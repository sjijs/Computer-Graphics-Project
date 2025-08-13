#include "rtweekend.h"

#include <fstream>
#include <cmath>
#include <limits>

#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "octahedron.h"

color ray_color(const ray& r, const hittable& world) {
    // 记录光线与物体的交点信息
    hit_record rec;
    if (world.hit(r, interval(0, infinity), rec)) {
        return 0.5 * (rec.normal + color(1,1,1));
    }

    // 计算背景颜色
    vec3 unit_direction = unit_vector(r.direction());
    // 计算光线方向的y分量，将其映射到[0,1]范围内。
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}

int main() {

    // Image

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    // Calculate the image height, and ensure that it's at least 1.
    // 计算图像高度，并确保至少为1。
    // 如果计算结果小于1，则将其设置为1。
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // World

    hittable_list world;
    world.add(make_shared<sphere>(point3(0.5,0,-1), 0.5));
    world.add(make_shared<octahedron>(point3(-0.5,0,-1), 0.5));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));

    // Camera

    // 视口是相机的可视区域，定义在实际物理空间中，与最终图像有映射关系
    // image定义了最终图像的分辨率与像素
    // 视口是实际物理空间中的一个矩形区域，而image是这个区域的采样结果。
    auto focal_length = 1.0;// 相机焦距
    auto viewport_height = 2.0;// 相机视口高度
    auto viewport_width = viewport_height * (double(image_width)/image_height);// 相机视口宽度
    auto camera_center = point3(0, 0, 0);// 相机中心位置

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    // 计算水平和垂直视口边缘的向量。
    // 这些向量定义了相机视口的大小和方向。
    auto viewport_u = vec3(viewport_width, 0, 0);
    auto viewport_v = vec3(0, -viewport_height, 0);

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    // 计算从一个像素到下一个像素的水平和垂直增量向量。
    // 这些向量用于在视口上定位每个像素的位置。
    auto pixel_delta_u = viewport_u / image_width;
    auto pixel_delta_v = viewport_v / image_height;

    // Calculate the location of the upper left pixel.
    // 计算左上角像素的位置。
    auto viewport_upper_left = camera_center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
    // 计算左上角像素的中心位置。
    // 这里的0.5是为了将像素中心对齐到视口的左上角。
    auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    // 创建输出文件流
    std::ofstream file("image_direct.ppm");
    
    // Render

    file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        std::clog << "\r渲染进度: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
            auto ray_direction = pixel_center - camera_center;
            ray r(camera_center, ray_direction);// 创建从相机中心到像素中心的光线

            color pixel_color = ray_color(r, world);// 获取光线颜色
            write_color(file, pixel_color);// 使用 write_color 函数将颜色写入文件
        }
    }
    
    file.close();
    std::clog << "\rDone.                 \n";
    std::cout << "PPM file generated successfully!\n";
    
    return 0;
}
