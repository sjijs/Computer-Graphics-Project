#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include <fstream>
#include <string>

class camera {
  public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width  = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth = 10;   // ***Maximum number of ray bounces into scene
    std::string output_filename = "output.ppm";  // Output PPM filename

    void render(const hittable& world) {
        initialize();

        // 创建输出文件流
        std::ofstream file(output_filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create output file " << output_filename << std::endl;
            return;
        }

        // 同时输出到标准输出和文件
        file << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; j++) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; i++) {
                color pixel_color(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                // 同时写入文件和标准输出
                write_color(file, pixel_samples_scale * pixel_color);
                write_color(std::cout, pixel_samples_scale * pixel_color);
            }
        }

        file.close();
        std::clog << "\rDone. Output saved to " << output_filename << "                 \n";
    }

    // 重载版本：允许指定输出文件名
    void render(const hittable& world, const std::string& filename) {
        output_filename = filename;
        render(world);
    }

  private:
    int    image_height;   // Rendered image height
    double pixel_samples_scale;  // Color scale factor for a sum of pixel samples
    point3 center;         // Camera center
    point3 pixel00_loc;    // Location of pixel 0, 0
    vec3   pixel_delta_u;  // Offset to pixel to the right
    vec3   pixel_delta_v;  // Offset to pixel below

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;  // 对每个像素的颜色值进行平均化处理

        center = point3(0, 0, 0);

        // Determine viewport dimensions.
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left =
            center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = center;  // center变量是相机的中心位置
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        // 盒式滤波，随机偏移量 0.5 指的是将随机样本的选取范围限定在以像素中心为中心，边长为 1 的正方形区域内。
        // random_double() 生成一个 [0, 1) 之间的随机数。
        // random_double() - 0.5 将随机数的范围平移到 [-0.5, 0.5) 之间。
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        // 如果深度小于等于0，表示光线已经经过了最大次数的反弹，此时返回黑色
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        // 计算光线与场景的交点
        if (world.hit(r, interval(0.001, infinity), rec)) {
            vec3 direction = random_on_hemisphere(rec.normal);
            return 0.5 * ray_color(ray(rec.p, direction), depth-1, world);  // 光线递归进入下一层，方向变为随机生成的方向
            // depth-1 表示光线已经反弹了一次，进入下一层的递归
        }

        // 背景色渐变
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif