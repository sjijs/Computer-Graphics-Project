#ifndef TEXTURE_H
#define TEXTURE_H

#include "rtweekend.h"

class texture {
  public:
    virtual ~texture() = default;

    virtual color value(double u, double v, const point3& p) const = 0;
};

class solid_color : public texture {
  public:
    solid_color(const color& albedo) : albedo(albedo) {}

    solid_color(double red, double green, double blue) : solid_color(color(red,green,blue)) {}

    color value(double u, double v, const point3& p) const override {
        return albedo;
    }

  private:
    color albedo;
};

class checker_texture : public texture {
  public:
    checker_texture(double scale, shared_ptr<texture> even, shared_ptr<texture> odd)
      : inv_scale(1.0 / scale), even(even), odd(odd) {}
      // 通过 scale 控制棋盘格大小：构造函数保存 inv_scale = 1.0/scale，计算时用 inv_scale * p.x() 相当于 p.x() / scale。

    checker_texture(double scale, const color& c1, const color& c2)
      : checker_texture(scale, make_shared<solid_color>(c1), make_shared<solid_color>(c2)) {}

    color value(double u, double v, const point3& p) const override {
        auto xInteger = int(std::floor(inv_scale * p.x())); // floor 向下取整
        auto yInteger = int(std::floor(inv_scale * p.y()));
        auto zInteger = int(std::floor(inv_scale * p.z()));

        bool isEven = (xInteger + yInteger + zInteger) % 2 == 0;
        // 将 xIndex + yIndex + zIndex 的和取奇偶：偶数用 even 纹理，奇数用 odd 纹理。这样在三维上形成交替的立方体格子（3D checker）。

        return isEven ? even->value(u, v, p) : odd->value(u, v, p); // 运行时多态（还是依靠虚函数表），纹理的具体实现由子类决定（传入数据类型选择）
    }

  private:
    double inv_scale;
    shared_ptr<texture> even;
    shared_ptr<texture> odd;
};

#endif