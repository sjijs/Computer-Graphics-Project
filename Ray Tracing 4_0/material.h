#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "texture.h"
#include <algorithm>

struct bsdf_sample {
  ray scattered;
  color f = color(0, 0, 0);
  color attenuation = color(1, 1, 1);
  double pdf = 0.0;
  bool is_delta = false;
};

inline double pow5(double x) {
  auto x2 = x * x;
  return x2 * x2 * x;
}

inline void build_onb(const vec3& n, vec3& t, vec3& b) {
  vec3 up = std::fabs(n.y()) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
  t = unit_vector(cross(up, n));
  b = cross(n, t);
}

inline vec3 to_world(const vec3& local, const vec3& n) {
  vec3 t, b;
  build_onb(n, t, b);
  return local.x() * t + local.y() * b + local.z() * n;
}

inline vec3 sample_cosine_hemisphere() {
  auto r1 = random_double();
  auto r2 = random_double();
  auto phi = 2.0 * pi * r1;
  auto r = std::sqrt(r2);

  auto x = r * std::cos(phi);
  auto y = r * std::sin(phi);
  auto z = std::sqrt(std::max(0.0, 1.0 - r2));
  return vec3(x, y, z);
}

inline double luminance(const color& c) {
  return 0.2126 * c.x() + 0.7152 * c.y() + 0.0722 * c.z();
}

inline color fresnel_schlick(double cos_theta, const color& F0) {
  return F0 + (color(1, 1, 1) - F0) * pow5(1.0 - std::clamp(cos_theta, 0.0, 1.0));
}

inline double ggx_D(double alpha, double NoH) {
  auto a2 = alpha * alpha;
  auto d = NoH * NoH * (a2 - 1.0) + 1.0;
  return a2 / (pi * d * d + 1e-10);
}

inline double smith_G1(double alpha, double NoX) {
  auto a = alpha;
  auto b = std::sqrt(a * a + (1.0 - a * a) * NoX * NoX);
  return (2.0 * NoX) / (NoX + b + 1e-10);
}

inline double smith_G2(double alpha, double NoV, double NoL) {
  return smith_G1(alpha, NoV) * smith_G1(alpha, NoL);
}

inline vec3 sample_ggx_half_vector(double alpha, const vec3& n) {
  auto u1 = random_double();
  auto u2 = random_double();

  auto a2 = alpha * alpha;
  auto phi = 2.0 * pi * u1;
  auto cos_theta = std::sqrt((1.0 - u2) / (1.0 + (a2 - 1.0) * u2));
  auto sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));

  vec3 local_h(std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta);
  return unit_vector(to_world(local_h, n));
}

/*
材质抽象类
1. 产生散射射线（或者说它吸收了入射射线）。
2. 如果散射，请说出光线应该衰减多少。
*/

class material {
  public:
    virtual ~material() = default;

    virtual color emitted(double u, double v, const point3& p) const {
        return color(0,0,0);
    }

    virtual bool sample_bsdf(const ray& r_in, const hit_record& rec, bsdf_sample& sample) const {
      return false;
    }

    virtual color eval_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const {
      return color(0, 0, 0);
    }

    virtual double pdf_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const {
      return 0.0;
    }

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const {
      bsdf_sample sample;
      if (!sample_bsdf(r_in, rec, sample))
        return false;

      scattered = sample.scattered;
      attenuation = sample.is_delta ? sample.attenuation : sample.f;
      return true;
    }
};

class lambertian : public material {
  public:
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}  // ***albedo作为反射率传入，同时也为漫反射材质最初的颜色***
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool sample_bsdf(const ray& r_in, const hit_record& rec, bsdf_sample& sample) const override {
      auto n = unit_vector(rec.shading_normal);
      auto local_dir = sample_cosine_hemisphere();
      auto wi = unit_vector(to_world(local_dir, n));
      auto cos_theta = std::max(0.0, dot(n, wi));
      if (cos_theta <= 0.0)
        return false;

      auto albedo = tex->value(rec.u, rec.v, rec.p);

      sample.scattered = ray(rec.p, wi, r_in.time());
      sample.f = albedo / pi;
      sample.attenuation = albedo;
      sample.pdf = cos_theta / pi;
      sample.is_delta = false;
        return true;
    }

    color eval_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const override {
      auto n = unit_vector(rec.shading_normal);
      if (dot(n, wi) <= 0.0) return color(0, 0, 0);
      return tex->value(rec.u, rec.v, rec.p) / pi;
    }

    double pdf_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const override {
      auto n = unit_vector(rec.shading_normal);
      auto cos_theta = std::max(0.0, dot(n, wi));
      return cos_theta / pi;
    }

  private:
    shared_ptr<texture> tex;
};

class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool sample_bsdf(const ray& r_in, const hit_record& rec, bsdf_sample& sample) const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());// 添加模糊效果
        // 同时这里在加入模糊前对反射光线先进行了一步归一化处理
        // 归一化把反射向量变成长度为1的方向向量，使“fuzz”只控制角度扰动的幅度，而不会被入射向量的长度放大或缩小。
        // 如果不归一化，入射向量的长度会线性影响反射向量的长度，加入 fuzz 后的偏移量会以非一致的方式改变方向（和物理上把 fuzz 当做“角度噪声”时不一致）。
        // 试验后结果确实出现不同，fuzz偏移量会随着入射向量长度变化而变化，导致反射效果不一致。
        if (dot(reflected, rec.normal) <= 0)
          return false;

        sample.scattered = ray(rec.p, reflected, r_in.time());
        sample.attenuation = albedo;
        sample.is_delta = true;
        sample.pdf = 1.0;
        return true;
    }

  private:
    color albedo;
    double fuzz;
};

class dielectric : public material {
  public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool sample_bsdf(const ray& r_in, const hit_record& rec, bsdf_sample& sample) const override {
        double ri = rec.front_face ? (1.0/refraction_index) : refraction_index;

        vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta*cos_theta);

        bool cannot_refract = ri * sin_theta > 1.0;
        vec3 direction;

         if (cannot_refract || reflectance(cos_theta, ri) > random_double())  // 判断是否无法折射（全反射），判断是否反射率极高产生近似反射
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, ri);

        sample.scattered = ray(rec.p, direction, r_in.time());
        sample.attenuation = color(1.0, 1.0, 1.0);
        sample.is_delta = true;
        sample.pdf = 1.0;
        return true;
    }

  private:
    // Refractive index in vacuum or air, or the ratio of the material's refractive index over
    // the refractive index of the enclosing media
    // 真空或空气中的折射率，**或材料的折射率与包围介质的折射率之比**
    // 周围介质的折射率
    double refraction_index;

    static double reflectance(double cosine, double refraction_index) {
        // Use Schlick's approximation for reflectance.
        auto r0 = (1 - refraction_index) / (1 + refraction_index);
        r0 = r0*r0;
        return r0 + (1-r0)*std::pow((1 - cosine),5);
    }
};

class diffuse_light : public material {
  public:
    diffuse_light(shared_ptr<texture> tex) : tex(tex) {}
    diffuse_light(const color& emit) : tex(make_shared<solid_color>(emit)) {}

    color emitted(double u, double v, const point3& p) const override {
        return tex->value(u, v, p); // 发光材质的颜色由纹理决定
        // 发光材质返回的值即为材质本身的颜色，不同与其余材质，返回值为数值等类型，影响光线方向等
        // 在光线追踪中，“光”就是RGB值，摄像机是按照光路可逆去追踪影响hit点的光线，最终累积这些光线的RGB值
        // 其他材质的scatter函数返回的是一个布尔值，表示光线是否被散射，如果散射就会被ray_color函数继续追踪传递下去
        // 同时通过内部算法修改相应的attenuation和scattered参数输出光线的衰减和散射方向
    }

  private:
    shared_ptr<texture> tex;
};

/**
 * 各向同性散射材质类 - 用于体积渲染中的粒子散射
 * 
 * 各向同性(Isotropic)的含义：
 * - 光线在任何方向上的散射概率都相等
 * - 这模拟了雾、烟雾、云朵等介质中小颗粒的散射行为
 * - 与表面材质不同，这种材质没有"表面法向量"的概念
 */
class isotropic : public material {
  public:
    /**
     * 构造函数 - 使用单一颜色创建各向同性材质
     * @param albedo 散射颜色，决定介质的整体颜色外观
     * 
     * 使用场景：
     * - 单色雾气：isotropic(color(0.7, 0.7, 0.7))  // 灰色雾
     * - 彩色烟雾：isotropic(color(0.2, 0.4, 0.9))  // 蓝色烟雾
     */
    isotropic(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}
    
    /**
     * 构造函数重载 - 使用纹理创建各向同性材质
     * @param tex 纹理对象，可以是图像纹理、噪声纹理等
     * 
     * 使用场景：
     * - 云朵纹理：使用Perlin噪声模拟云的密度变化
     * - 火焰效果：使用渐变纹理模拟火焰的颜色变化
     * - 复杂介质：使用图像纹理定义介质的空间分布
     */
    isotropic(shared_ptr<texture> tex) : tex(tex) {}

    /**
     * 散射函数 - 处理光线在介质中的散射行为
     * 
     * @param r_in 入射光线
     * @param rec 散射点的记录信息（位置、法向量等）
     * @param attenuation 散射后的颜色衰减系数
     * @param scattered 散射后的新光线
     * @return 始终返回true，表示散射总是发生
     * 
     * 散射过程详解：
     * 1. 生成随机的散射方向（均匀分布在单位球面上）
     * 2. 从散射点发出新的光线
     * 3. 根据纹理计算散射颜色
     */
    bool sample_bsdf(const ray& r_in, const hit_record& rec, bsdf_sample& sample) const override {
      sample.scattered = ray(rec.p, random_unit_vector(), r_in.time());
      sample.attenuation = tex->value(rec.u, rec.v, rec.p);
      sample.f = sample.attenuation * (1.0 / (4.0 * pi));
      sample.pdf = 1.0 / (4.0 * pi);
      sample.is_delta = false;
        return true;
    }

    color eval_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const override {
      return tex->value(rec.u, rec.v, rec.p) * (1.0 / (4.0 * pi));
    }

    double pdf_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const override {
      return 1.0 / (4.0 * pi);
    }

  private:
    shared_ptr<texture> tex;  // 纹理对象，定义介质的颜色分布
};

  class disney_material : public material {
    public:
    disney_material(const color& base_color,
            double metallic = 0.0,
            double roughness = 0.5,
            double specular = 0.5,
            double clearcoat = 0.0,
            double sheen = 0.0)
      : base_color_factor(base_color),
      metallic_factor(metallic),
      roughness_factor(roughness),
      specular(specular),
      clearcoat(clearcoat),
      sheen(sheen) {}

    disney_material(shared_ptr<texture> base_color_tex,
            double metallic = 0.0,
            double roughness = 0.5,
            double specular = 0.5,
            double clearcoat = 0.0,
            double sheen = 0.0)
      : base_color_tex(base_color_tex),
      metallic_factor(metallic),
      roughness_factor(roughness),
      specular(specular),
      clearcoat(clearcoat),
      sheen(sheen) {}

    void set_normal_texture(shared_ptr<texture> tex) { normal_tex = tex; }
    void set_metallic_roughness_texture(shared_ptr<texture> tex) { metallic_roughness_tex = tex; }
    void set_emissive_texture(shared_ptr<texture> tex) { emissive_tex = tex; }
    void set_emissive_factor(const color& e) { emissive_factor = e; }

    color emitted(double u, double v, const point3& p) const override {
      if (!emissive_tex && emissive_factor.near_zero()) return color(0, 0, 0);

      color tex_e = emissive_tex ? emissive_tex->value(u, v, p) : color(1, 1, 1);
      return emissive_factor * tex_e;
    }

    bool sample_bsdf(const ray& r_in, const hit_record& rec, bsdf_sample& sample) const override {
      SurfaceParams sp = surface_params(rec);
      vec3 n = shading_normal(rec, sp);
      vec3 wo = unit_vector(-r_in.direction());
      if (dot(wo, n) <= 0.0)
        return false;

      double wd = std::max(0.02, 1.0 - sp.metallic);
      double ws = 1.0;
      double wc = 0.25 * clearcoat;
      double sum = wd + ws + wc;
      wd /= sum;
      ws /= sum;
      wc /= sum;

      vec3 wi;
      double pick = random_double();
      if (pick < wd) {
        wi = unit_vector(to_world(sample_cosine_hemisphere(), n));
      } else if (pick < wd + ws) {
        double alpha = std::max(0.02, sp.roughness * sp.roughness);
        vec3 h = sample_ggx_half_vector(alpha, n);
        wi = reflect(-wo, h);
      } else {
        double alpha_cc = 0.1;
        vec3 h = sample_ggx_half_vector(alpha_cc, n);
        wi = reflect(-wo, h);
      }

      if (dot(wi, n) <= 0.0)
        return false;

      sample.scattered = ray(rec.p, wi, r_in.time());
      sample.f = eval_bsdf(r_in, rec, wi);
      sample.pdf = pdf_bsdf(r_in, rec, wi);
      sample.attenuation = color(1, 1, 1);
      sample.is_delta = false;

      return sample.pdf > 1e-8 && sample.f.length_squared() > 0.0;
    }

    color eval_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const override {
      SurfaceParams sp = surface_params(rec);
      vec3 n = shading_normal(rec, sp);
      vec3 wo = unit_vector(-r_in.direction());
      vec3 l = unit_vector(wi);

      double NoV = std::max(0.0, dot(n, wo));
      double NoL = std::max(0.0, dot(n, l));
      if (NoV <= 0.0 || NoL <= 0.0)
        return color(0, 0, 0);

      vec3 h = unit_vector(wo + l);
      double NoH = std::max(0.0, dot(n, h));
      double VoH = std::max(0.0, dot(wo, h));

      color diffuse = (1.0 - sp.metallic) * sp.base_color / pi;

      double alpha = std::max(0.02, sp.roughness * sp.roughness);
      double D = ggx_D(alpha, NoH);
      double G = smith_G2(alpha, NoV, NoL);
      color F0 = (1.0 - sp.metallic) * color(0.08 * specular, 0.08 * specular, 0.08 * specular)
           + sp.metallic * sp.base_color;
      color F = fresnel_schlick(VoH, F0);
      color spec_term = (D * G / std::max(1e-6, 4.0 * NoV * NoL)) * F;

      color Ctint = color(1, 1, 1);
      double lum = luminance(sp.base_color);
      if (lum > 1e-6) Ctint = sp.base_color / lum;
      color sheen_term = (1.0 - sp.metallic) * sheen * pow5(1.0 - VoH) * Ctint;

      double alpha_cc = 0.1;
      double Dcc = ggx_D(alpha_cc, NoH);
      double Gcc = smith_G2(0.25, NoV, NoL);
      double Fcc = 0.04 + 0.96 * pow5(1.0 - VoH);
      color clear_term = color(1, 1, 1) * (clearcoat * 0.25 * Dcc * Gcc * Fcc / std::max(1e-6, 4.0 * NoV * NoL));

      return diffuse + spec_term + sheen_term + clear_term;
    }

    double pdf_bsdf(const ray& r_in, const hit_record& rec, const vec3& wi) const override {
      SurfaceParams sp = surface_params(rec);
      vec3 n = shading_normal(rec, sp);
      vec3 wo = unit_vector(-r_in.direction());
      vec3 l = unit_vector(wi);

      double NoL = std::max(0.0, dot(n, l));
      double NoV = std::max(0.0, dot(n, wo));
      if (NoL <= 0.0 || NoV <= 0.0)
        return 0.0;

      vec3 h = unit_vector(wo + l);
      double NoH = std::max(0.0, dot(n, h));
      double VoH = std::max(1e-8, dot(wo, h));

      double wd = std::max(0.02, 1.0 - sp.metallic);
      double ws = 1.0;
      double wc = 0.25 * clearcoat;
      double sum = wd + ws + wc;
      wd /= sum;
      ws /= sum;
      wc /= sum;

      double pdf_diff = NoL / pi;

      double alpha = std::max(0.02, sp.roughness * sp.roughness);
      double D = ggx_D(alpha, NoH);
      double pdf_spec = D * NoH / (4.0 * VoH + 1e-10);

      double Dcc = ggx_D(0.1, NoH);
      double pdf_clear = Dcc * NoH / (4.0 * VoH + 1e-10);

      return wd * pdf_diff + ws * pdf_spec + wc * pdf_clear;
    }

    private:
    struct SurfaceParams {
      color base_color = color(0.8, 0.8, 0.8);
      double metallic = 0.0;
      double roughness = 0.5;
    };

    shared_ptr<texture> base_color_tex;
    shared_ptr<texture> normal_tex;
    shared_ptr<texture> metallic_roughness_tex;
    shared_ptr<texture> emissive_tex;

    color base_color_factor = color(0.8, 0.8, 0.8);
    color emissive_factor = color(0, 0, 0);
    double metallic_factor = 0.0;
    double roughness_factor = 0.5;

    double specular = 0.5;
    double clearcoat = 0.0;
    double sheen = 0.0;

    SurfaceParams surface_params(const hit_record& rec) const {
      SurfaceParams p;
      p.base_color = base_color_tex ? base_color_tex->value(rec.u, rec.v, rec.p) : base_color_factor;
      p.metallic = std::clamp(metallic_factor, 0.0, 1.0);
      p.roughness = std::clamp(roughness_factor, 0.02, 1.0);

      if (metallic_roughness_tex) {
        color mr = metallic_roughness_tex->value(rec.u, rec.v, rec.p);
        p.roughness = std::clamp(p.roughness * mr.y(), 0.02, 1.0);
        p.metallic = std::clamp(p.metallic * mr.z(), 0.0, 1.0);
      }

      return p;
    }

    vec3 shading_normal(const hit_record& rec, const SurfaceParams& params) const {
      vec3 n = unit_vector(rec.shading_normal);
      if (!normal_tex || !rec.has_tangent_space)
        return n;

      color nt = normal_tex->value(rec.u, rec.v, rec.p);
      vec3 tangent_n(2.0 * nt.x() - 1.0, 2.0 * nt.y() - 1.0, 2.0 * nt.z() - 1.0);

      vec3 t = unit_vector(rec.tangent);
      vec3 b = unit_vector(rec.bitangent);

      vec3 mapped = unit_vector(tangent_n.x() * t + tangent_n.y() * b + tangent_n.z() * n);
      if (dot(mapped, rec.normal) < 0.0)
        mapped = -mapped;

      return mapped;
    }
  };

/**
 * 物理背景知识：
 * 
 * 1. **Mie散射 vs Rayleigh散射**：
 *    - 当粒子尺寸接近光波长时，发生Mie散射（各向同性）
 *    - 当粒子远小于光波长时，发生Rayleigh散射（方向相关）
 *    - 云朵、雾气主要表现为Mie散射
 * 
 * 2. **相位函数**：
 *    - 描述散射方向的概率分布
 *    - 各向同性相位函数：P(θ) = 1/(4π)，所有方向概率相等
 *    - 更复杂的相位函数：Henyey-Greenstein函数，可以模拟前向或后向散射偏好
 * 
 * 3. **体积渲染方程**：
 *    - L(x,ω) = ∫ σₛ(x) * P(ω,ω') * L(x,ω') dω' + σₐ(x) * Lₑ(x,ω)
 *    - 其中σₛ是散射系数，P是相位函数，σₐ是吸收系数
 */

#endif