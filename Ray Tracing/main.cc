#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

#include "color.h"
#include "ray.h"
#include "vec3.h"

double hit_sphere(const point3& center, double radius, const ray& r) {
    vec3 oc = center - r.origin();
    auto a = r.direction().length_squared();
    auto h = dot(r.direction(), oc);
    auto c = oc.length_squared() - radius*radius;
    auto discriminant = h*h - a*c;
    
    if (discriminant < 0) {
        return -1.0;
    } else {
        return (h - std::sqrt(discriminant)) / a;// 返回找到的 t 值
    }
}

double hit_octahedron(const point3& center, double length, const ray& r) {
    // 计算 a_i = C_i - Q_i (八面体中心 - 射线起点)
    vec3 a = center - r.origin();
    vec3 d = r.direction();
    
    // 存储零点候选值
    double candidates[6];
    int candidate_count = 0;
    
    // 计算三个分量的零点 t_i = a_i / d_i (当 d_i != 0 时)
    for (int i = 0; i < 3; i++) {
        if (std::abs(d[i]) > 1e-8) { // 避免除零
            candidates[candidate_count++] = a[i] / d[i];
        }
    }
    
    // 添加两个远端点进行比较
    candidates[candidate_count++] = -1000.0; // 负无穷的近似
    candidates[candidate_count++] = 1000.0;  // 正无穷的近似
    
    // 计算 F = length/2 (立方体的半边长)
    double F = length / 2.0;
    
    // 寻找 g(t) 的最小值
    double g_min = std::numeric_limits<double>::max();
    
    for (int i = 0; i < candidate_count; i++) {
        double t = candidates[i];
        
        // 计算 g(t) = |a_x - t*d_x| + |a_y - t*d_y| + |a_z - t*d_z|
        double g_t = std::abs(a.x() - t * d.x()) + 
                     std::abs(a.y() - t * d.y()) + 
                     std::abs(a.z() - t * d.z());
        
        if (g_t < g_min) {
            g_min = g_t;
        }
    }
    
    // 判断是否有解：F >= g_min
    if (F >= g_min) {
        // 找到使 g(t) = F 的 t 值
        // 我们需要在候选点附近搜索实际的交点
        double best_t = -1.0;
        
        for (int i = 0; i < candidate_count; i++) {
            double t = candidates[i];
            if (t <= 0.001) continue; // 忽略负值和过小的值，避免自相交
            
            // 计算该点的 g(t) 值
            double g_t = std::abs(a.x() - t * d.x()) + 
                         std::abs(a.y() - t * d.y()) + 
                         std::abs(a.z() - t * d.z());
            
            // 如果这个点在立方体表面上或内部
            if (g_t <= F + 1e-6) {
                if (best_t < 0 || t < best_t) {
                    best_t = t;
                }
            }
        }
        
        // 如果没有找到合适的候选点，进行二分搜索
        if (best_t < 0) {
            // 在 [0, 1000] 范围内二分搜索找到 g(t) = F 的点
            double left = 0.001, right = 1000.0;
            for (int iter = 0; iter < 50; iter++) {
                double mid = (left + right) / 2.0;
                double g_mid = std::abs(a.x() - mid * d.x()) + 
                               std::abs(a.y() - mid * d.y()) + 
                               std::abs(a.z() - mid * d.z());
                
                if (std::abs(g_mid - F) < 1e-6) {
                    best_t = mid;
                    break;
                }
                
                if (g_mid > F) {
                    right = mid;
                } else {
                    left = mid;
                }
            }
            if (best_t < 0) best_t = (left + right) / 2.0;
        }
        
        return best_t;// 返回找到的 t 值
    } else {
        return -1.0;
    }
}

// 基于原方法计算八面体法向量
vec3 get_octahedron_normal_original_method(const point3& center, double length, const point3& hit_point) {
    vec3 local_point = hit_point - center;
    
    // 对于八面体，法向量是每个分量的符号向量，并进行归一化
    // 八面体的每个面的法向量都是 (±1, ±1, ±1) 的某种组合
    
    vec3 normal;
    normal.e[0] = (local_point.x() >= 0) ? 1.0 : -1.0;
    normal.e[1] = (local_point.y() >= 0) ? 1.0 : -1.0;
    normal.e[2] = (local_point.z() >= 0) ? 1.0 : -1.0;
    
    // 归一化法向量
    return unit_vector(normal);
}

color ray_color(const ray& r) {
    auto t1 = hit_sphere(point3(0.5,0,-1), 0.5, r);
    if (t1 > 0.0) {
        vec3 N = unit_vector(r.at(t1) - vec3(0.5,0,-1)); // 计算法向量
        return 0.5*color(N.x()+1, N.y()+1, N.z()+1);
    }

    auto t2 = hit_octahedron(point3(-0.5,0,-1), 0.5, r); // 移动八面体位置避免重叠
    if (t2 > 0.0) {
        point3 hit_point = r.at(t2);
        vec3 N = get_octahedron_normal_original_method(point3(-0.5,0,-1), 0.5, hit_point);
        return 0.5*color(N.x()+1, N.y()+1, N.z()+1);
    }

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

            color pixel_color = ray_color(r);// 获取光线颜色
            write_color(file, pixel_color);// 使用 write_color 函数将颜色写入文件
        }
    }
    
    file.close();
    std::clog << "\rDone.                 \n";
    std::cout << "PPM file generated successfully!\n";
    
    return 0;
}
