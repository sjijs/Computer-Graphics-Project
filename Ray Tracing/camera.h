#ifndef CAMERA_H
#define CAMERA_H

#include "hittable.h"
#include "material.h"
#include <fstream>
#include <string>

class camera {
  public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width  = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth = 10;   // **Maximum number of ray bounces into scene**

    double vfov = 90;  // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,0);   // Point camera is looking from
    point3 lookat   = point3(0,0,-1);  // Point camera is looking at
    vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction，(0,1,0)该向量使得相机的上方向与世界坐标系的Y轴对齐

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus


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
    vec3   u, v, w;              // Camera frame basis vectors
    vec3   defocus_disk_u;       // Defocus disk horizontal radius
    vec3   defocus_disk_v;       // Defocus disk vertical radius

    void initialize() {
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;  // 对每个像素的颜色值进行平均化处理

        center = lookfrom;

        // Determine viewport dimensions.
        auto theta = degrees_to_radians(vfov);  //  视场可调，可由vfov控制
        auto h = std::tan(theta/2);
        auto viewport_height = 2 * h * focus_dist;
        auto viewport_width = viewport_height * (double(image_width)/image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

        // Calculate the camera defocus disk basis vectors.
        auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
        defocus_disk_u = u * defocus_radius;
        defocus_disk_v = v * defocus_radius;
    }

    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the defocus disk and directed at a randomly
        // sampled point around the pixel location i, j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
        // center变量是相机的中心位置.defocus_angle是相机的散焦角度
        // 如果散焦角度小于等于0，则光线起始位置为相机中心，此时相当于完全小孔成像，所有图像显示都是清晰的（之前不考虑薄透镜模型的相机即为小孔相机）
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

    point3 defocus_disk_sample() const {
        // Returns a random point in the camera defocus disk.
        auto p = random_in_unit_disk();
        return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
    }


    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        // 如果深度小于等于0，表示光线已经经过了最大次数的反弹，此时返回黑色
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        // 计算光线与场景的交点
        if (world.hit(r, interval(0.001, infinity), rec)) {
            // 如果光线与物体相交，设置交点的法向量
            ray scattered;// 反弹光线
            color attenuation;// 反弹光线衰减系数
            if (rec.mat->scatter(r, rec, attenuation, scattered))// 如果材质有散射光线
            // 这里前方点乘的attenuation为main函数中设置的材质反射率，实际也表现为材质的颜色
                return attenuation * ray_color(scattered, depth-1, world);// 光线递归进入下一层，方向变为材质约定的方向
            return color(0,0,0);// 如果材质没有散射光线，则返回黑色
            // depth-1 表示光线已经反弹了一次，进入下一层的递归
            // 最终像素颜色是所有光线路径贡献的积分
        }

        // 背景色渐变
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
    /*
    这段程序的逻辑是实现光线追踪的基本步骤：
    1. 检查光线与场景中的物体是否相交。
    2. 如果相交，计算光线在交点的反射方向，并递归调用 ray_color 函数进行光线反弹，直到达到最大反弹次数（防止光线无限传播）。
    3. 如果没有相交，返回背景色（在逐层递归的条件中尤为重要，
        比如没有击中物体时，则会直接运行至背景色渐变的绘制代码，
        当击中n次(n<10)次物体时，此时光线传播路径上再不会击中物体，此时就会绘制该光线颜色为背景色渐变）。
    **最终像素颜色是所有光线路径贡献的积分**
    **所以最后图像自然出现的环境光遮蔽现象则为：在两个物体接近的地方光线反弹多次最终达到最高反弹次数返回了黑色**
    */
};

#endif