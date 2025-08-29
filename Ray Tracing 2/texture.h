#ifndef TEXTURE_H
#define TEXTURE_H

#include "rtweekend.h"
#include "rtw_stb_image.h"
#include "perlin.h"

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

class image_texture : public texture {
  public:
    image_texture(const char* filename) : image(filename) {}

    color value(double u, double v, const point3& p) const override {
        // If we have no texture data, then return solid cyan as a debugging aid.
        if (image.height() <= 0) return color(0,1,1);

        // Clamp input texture coordinates to [0,1] x [1,0]
        u = interval(0,1).clamp(u);
        v = 1.0 - interval(0,1).clamp(v);  // Flip V to image coordinates

        auto i = int(u * image.width()); // 映射至图像像素位置(i,j)
        auto j = int(v * image.height());
        auto pixel = image.pixel_data(i,j);

        auto color_scale = 1.0 / 255.0; // 颜色值从 [0,255] 映射到 [0.0,1.0]
        return color(color_scale*pixel[0], color_scale*pixel[1], color_scale*pixel[2]);
    }

  private:
    rtw_image image;
};

class noise_texture : public texture {
  public:
    noise_texture(double scale) : scale(scale) {}

    color value(double u, double v, const point3& p) const override {
        // return color(1,1,1) * 0.5 * (1.0 + noise.noise(scale * p)); // 归一化噪声值到 [0,1],由于linear_to_gamma()颜色函数仅期望正输入
        // return color(1,1,1) * noise.turb(p, 7); // 纹理涡旋，depth越大细节越丰富
        return color(.5, .5, .5) * (1 + std::sin(scale * p.z() + 10 * noise.turb(p, 7))); // 结合噪声和正弦函数，增加纹理细节（使用湍流调整相位）
    }

  private:
    perlin noise;
    double scale; // 噪声缩放因子
};

#endif