#ifndef MATERIAL_H
#define MATERIAL_H

#include "hittable.h"

/*
材质抽象类
1. 产生散射射线（或者说它吸收了入射射线）。
2. 如果散射，请说出光线应该衰减多少。
*/

class material {
  public:
    virtual ~material() = default;

    virtual bool scatter(
        const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered
    ) const {
        return false;
    }
};

class lambertian : public material {
  public:
    lambertian(const color& albedo) : albedo(albedo) {}  // albedo作为反射率传入，同时也为漫反射材质的颜色

    bool scatter(const ray& r_in, const hit_record& rec, color& attenuation, ray& scattered)
    const override {
        auto scatter_direction = rec.normal + random_unit_vector();// 生成随机散射方向

        // 防止“假向量”情况
        if (scatter_direction.near_zero())
            scatter_direction = rec.normal;
        
        scattered = ray(rec.p, scatter_direction, r_in.time());// 生成散射光线
        attenuation = albedo;// 反射光线衰减系数
        return true;
    }

  private:
    color albedo;
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

#endif