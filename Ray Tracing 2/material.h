#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"
#include "texture.h"

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

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const {
        return false;
    }
};

class lambertian : public material {
  public:
    lambertian(const color& albedo) : tex(make_shared<solid_color>(albedo)) {}  // ***albedo作为反射率传入，同时也为漫反射材质最初的颜色***
    lambertian(shared_ptr<texture> tex) : tex(tex) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();// 生成随机散射方向

        // 防止“假向量”情况
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;
        
        scattered = ray(rec.p, scatter_direction, r_in.time());// 生成散射光线
        attenuation = tex->value(rec.u, rec.v, rec.p);// 反射光线衰减系数
        return true;
    }

  private:
    shared_ptr<texture> tex;
};

class metal : public material {
  public:
    metal(const color& albedo, double fuzz) : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        reflected = unit_vector(reflected) + (fuzz * random_unit_vector());// 添加模糊效果
        // 同时这里在加入模糊前对反射光线先进行了一步归一化处理
        // 归一化把反射向量变成长度为1的方向向量，使“fuzz”只控制角度扰动的幅度，而不会被入射向量的长度放大或缩小。
        // 如果不归一化，入射向量的长度会线性影响反射向量的长度，加入 fuzz 后的偏移量会以非一致的方式改变方向（和物理上把 fuzz 当做“角度噪声”时不一致）。
        // 试验后结果确实出现不同，fuzz偏移量会随着入射向量长度变化而变化，导致反射效果不一致。
        scattered = ray(rec.p, reflected, r_in.time());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);// 反射光线必须朝向物体表面
    }

  private:
    color albedo;
    double fuzz;
};

class dielectric : public material {
  public:
    dielectric(double refraction_index) : refraction_index(refraction_index) {}

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        attenuation = color(1.0, 1.0, 1.0);
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

        scattered = ray(rec.p, direction, r_in.time());
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
    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        // 步骤1：生成各向同性的随机散射方向
        // random_unit_vector() 在单位球面上均匀采样
        // 这确保了散射在所有方向上的概率相等
        scattered = ray(rec.p, random_unit_vector(), r_in.time());
        
        // 步骤2：计算散射点的颜色
        // 使用纹理坐标(u,v)和3D位置(p)来采样纹理
        // 对于体积渲染，纹理坐标可能基于3D位置计算
        attenuation = tex->value(rec.u, rec.v, rec.p); // 指向对应的材质采样函数
        
        // 步骤3：散射总是发生
        // 在真实的体积渲染中，散射概率取决于介质密度
        // 但在这个简化模型中，我们假设散射总是发生
        return true;
    }

  private:
    shared_ptr<texture> tex;  // 纹理对象，定义介质的颜色分布
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