#ifndef CAMERA_H
#define CAMERA_H

#include "rtweekend.h"
#include "rtw_stb_image.h"
#include "hittable.h"
#include "material.h"
#include "color.h"
#include "spherical_harmonics.h"
#include "render_window.h"
#include <fstream>
#include <string>
#include <algorithm>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <iomanip>

class camera {
  public:
    double aspect_ratio = 1.0;  // Ratio of image width over height
    int    image_width  = 100;  // Rendered image width in pixel count
    int    samples_per_pixel = 10;   // Count of random samples for each pixel
    int    max_depth = 10;   // **Maximum number of ray bounces into scene**
    color  background;   // Scene background color
    bool enable_skybox = true; // 是否启用天空盒
    bool enable_window_output = true; // 是否以窗口实时显示渲染

    double vfov = 90;  // Vertical view angle (field of view)
    point3 lookfrom = point3(0,0,0);   // Point camera is looking from
    point3 lookat   = point3(0,0,-1);  // Point camera is looking at
    vec3   vup      = vec3(0,1,0);     // Camera-relative "up" direction，(0,1,0)该向量使得相机的上方向与世界坐标系的Y轴对齐

    double defocus_angle = 0;  // Variation angle of rays through each pixel
    double focus_dist = 10;    // Distance from camera lookfrom point to plane of perfect focus


    std::string output_filename = "output.ppm";  // Output PPM filename
    std::string skybox_filename = "skybox.ppm";  // 天空盒贴图文件名

    std::string sh_coeffs_filename = "skybox_sh.txt";  // 球谐系数文件名
    bool use_spherical_harmonics = true;

    // === 多线程渲染控制 ===
    bool enable_multithreading = true;  // 是否启用多线程渲染
    int num_threads = std::thread::hardware_concurrency();  // 线程数量，默认为CPU核心数

    // 天空盒贴图读取（贴图式全局光照）
    bool load_skybox(const std::string& filename) {
        // 使用rtw_image加载天空盒，支持多种格式
        skybox_image = std::make_unique<rtw_image>(filename.c_str());
        
        if (skybox_image->width() == 0 || skybox_image->height() == 0) {
            std::cerr << "Warning: Cannot load skybox file: " << filename << std::endl;
            skybox_image.reset();
            return false;
        }
        
        skybox_width = skybox_image->width();
        skybox_height = skybox_image->height();
        
        std::cout << "Loaded skybox: " << skybox_width << "x" << skybox_height << " (" << filename << ")" << std::endl;
        return true;
    }


    void render(const hittable& world) {
        initialize();

        // 加载天空盒一次
        if (!load_skybox(skybox_filename)) {
            std::cerr << "Warning: Skybox not loaded, fallback to gradient.\n";
        }

        // 初始化球谐函数
        if (use_spherical_harmonics) {
            if (!sh_lighting.loadCoefficients(sh_coeffs_filename)) {
                std::cerr << "警告: 无法加载球谐系数，将使用默认天空盒贴图" << std::endl;
                use_spherical_harmonics = false;
            } else {
                std::cout << "已加载球谐函数环境光照" << std::endl;
            }
        }

        RenderWindow window;
        RenderWindow* window_ptr = nullptr;
        if (enable_window_output) {
            if (window.create(image_width, image_height, L"Ray Tracing Renderer")) {
                window_ptr = &window;
            } else {
                std::cerr << "Warning: 无法创建渲染窗口，将继续以无窗口模式渲染。" << std::endl;
            }
        }

        std::vector<color> accumulation_buffer(image_width * image_height, color(0, 0, 0));
        std::vector<uint8_t> display_buffer(static_cast<size_t>(image_width) * image_height * 4, 0);

        bool completed = false;
        if (enable_multithreading && num_threads > 1) {
            std::clog << "启动多线程渲染模式，使用 " << num_threads << " 个线程..." << std::endl;
            completed = render_multithreaded(world, window_ptr, accumulation_buffer, display_buffer);
        } else {
            std::clog << "使用单线程渲染模式..." << std::endl;
            completed = render_singlethreaded(world, window_ptr, accumulation_buffer, display_buffer);
        }

        if (window_ptr) {
            window_ptr->process_events();
            window_ptr->present(display_buffer.data());
        }

        if (completed && save_to_file_) {
            write_buffer_to_ppm(accumulation_buffer, output_filename);
            std::clog << "帧已保存到 " << output_filename << std::endl;
        }

        save_to_file_ = false;
    }

    // 重载版本：允许指定输出文件名
    void render(const hittable& world, const std::string& filename) {
        auto previous_filename = output_filename;
        bool previous_save_flag = save_to_file_;
        output_filename = filename;
        save_to_file_ = true;
        render(world);
        save_to_file_ = previous_save_flag;
        output_filename = previous_filename;
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

    // 球谐函数对象
    SphericalHarmonics sh_lighting{3};  // 3阶球谐函数

    // --- 天空盒数据 ---
    int skybox_width = 0;
    int skybox_height = 0;
    std::unique_ptr<rtw_image> skybox_image; // 使用rtw_image加载天空盒

    bool save_to_file_ = false;

    // === 渲染实现（窗口输出） ===

    bool render_multithreaded(const hittable& world,
                              RenderWindow* window,
                              std::vector<color>& accumulation_buffer,
                              std::vector<uint8_t>& display_buffer) {
        const int worker_count = std::max(1, std::min(num_threads, image_height));
        std::atomic<int> completed_rows{0};
        std::atomic<bool> cancel_render{false};

        std::vector<std::thread> threads;
        threads.reserve(worker_count);

        int base_rows = image_height / worker_count;
        int extra_rows = image_height % worker_count;
        int current_row = 0;

        for (int thread_id = 0; thread_id < worker_count; ++thread_id) {
            int row_count = base_rows + (thread_id < extra_rows ? 1 : 0);
            if (row_count <= 0) {
                continue;
            }

            int start_row = current_row;
            int end_row = start_row + row_count;
            current_row = end_row;

            threads.emplace_back([=, &world, &accumulation_buffer, &display_buffer, &completed_rows, &cancel_render]() {
                for (int j = start_row; j < end_row; ++j) {
                    if (cancel_render.load(std::memory_order_relaxed)) {
                        break;
                    }

                    for (int i = 0; i < image_width; ++i) {
                        if (cancel_render.load(std::memory_order_relaxed)) {
                            break;
                        }

                        color pixel_color(0, 0, 0);
                        for (int sample = 0; sample < samples_per_pixel; ++sample) {
                            ray r = get_ray(i, j);
                            pixel_color += ray_color(r, max_depth, world);
                        }

                        color scaled_color = pixel_samples_scale * pixel_color;
                        store_pixel(i, j, scaled_color, accumulation_buffer, display_buffer);
                    }

                    completed_rows.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        int last_reported = -1;
        while (!cancel_render.load(std::memory_order_relaxed)) {
            int current_completed = completed_rows.load(std::memory_order_relaxed);
            if (current_completed != last_reported) {
                std::clog << "\rProgress: " << current_completed << "/" << image_height
                          << " rows (" << std::fixed << std::setprecision(1)
                          << (100.0 * current_completed / std::max(1, image_height))
                          << "%) " << std::flush;
                last_reported = current_completed;
            }

            if (current_completed >= image_height) {
                break;
            }

            if (window) {
                if (!window->process_events()) {
                    cancel_render.store(true, std::memory_order_relaxed);
                    break;
                }
                window->present(display_buffer.data());
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(window ? 16 : 50));
        }

        if (cancel_render.load(std::memory_order_relaxed)) {
            std::clog << "\n检测到窗口关闭，中止渲染。" << std::endl;
        }

        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        if (window && !cancel_render.load(std::memory_order_relaxed)) {
            window->process_events();
            window->present(display_buffer.data());
        }

        std::clog << std::endl;
        return !cancel_render.load(std::memory_order_relaxed);
    }

    bool render_singlethreaded(const hittable& world,
                               RenderWindow* window,
                               std::vector<color>& accumulation_buffer,
                               std::vector<uint8_t>& display_buffer) {
        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;

            if (window && !window->process_events()) {
                std::clog << "\n窗口关闭，中止渲染。" << std::endl;
                return false;
            }

            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }

                color scaled_color = pixel_samples_scale * pixel_color;
                store_pixel(i, j, scaled_color, accumulation_buffer, display_buffer);
            }

            if (window) {
                window->present(display_buffer.data());
            }
        }

        std::clog << std::endl;
        return true;
    }

    void store_pixel(int i, int j,
                     const color& scaled_color,
                     std::vector<color>& accumulation_buffer,
                     std::vector<uint8_t>& display_buffer) const {
        const int index = j * image_width + i;
        accumulation_buffer[index] = scaled_color;
        encode_pixel_to_bgra(scaled_color, &display_buffer[static_cast<size_t>(index) * 4]);
    }

    void encode_pixel_to_bgra(const color& linear_color, uint8_t* dest) const {
        double r = linear_to_gamma(linear_color.x());
        double g = linear_to_gamma(linear_color.y());
        double b = linear_to_gamma(linear_color.z());

        static const interval intensity(0.000, 0.999);
        const auto rbyte = static_cast<uint8_t>(256 * intensity.clamp(r));
        const auto gbyte = static_cast<uint8_t>(256 * intensity.clamp(g));
        const auto bbyte = static_cast<uint8_t>(256 * intensity.clamp(b));

        dest[0] = bbyte;
        dest[1] = gbyte;
        dest[2] = rbyte;
        dest[3] = 255;
    }

    void write_buffer_to_ppm(const std::vector<color>& accumulation_buffer,
                             const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create output file " << filename << std::endl;
            return;
        }

        file << "P3\n" << image_width << ' ' << image_height << "\n255\n";
        for (const auto& pixel : accumulation_buffer) {
            write_color(file, pixel);
        }
    }

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
        auto ray_time = random_double();

        return ray(ray_origin, ray_direction, ray_time);
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

    color sample_skybox(vec3 direction) const {
        if (!skybox_image || skybox_image->width() == 0 || skybox_image->height() == 0) {
            // fallback：蓝白渐变
            vec3 unit_direction = unit_vector(direction);
            auto a = 0.5*(unit_direction.y() + 1.0);
            return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
        }

        // 归一化方向向量
        vec3 unit_direction = unit_vector(direction);
        double x = unit_direction.x();
        double y = unit_direction.y();
        double z = unit_direction.z();

        // 球面坐标映射 - 修正版本
        // theta: 从+Y轴测量的极角 (0到π)
        // phi: 从+X轴测量的方位角 (-π到π)
        double theta = acos(std::clamp(y, -1.0, 1.0));
        double phi = atan2(z, x);
        
        // 将phi从[-π,π]映射到[0,1]
        double u = (phi + pi) / (2.0 * pi);
        // 将theta从[0,π]映射到[0,1] 
        double v = theta / pi;
        
        // 边界检查和环绕处理
        u = u - std::floor(u);  // 确保u在[0,1)范围内，等价于fmod(u + 1.0, 1.0)
        v = std::clamp(v, 0.0, 1.0);  // 确保v在[0,1]范围内
        
        // 转换为像素坐标
        int i = static_cast<int>(u * skybox_width);
        int j = static_cast<int>(v * skybox_height);
        
        // 边界保护
        i = std::clamp(i, 0, skybox_width - 1);
        j = std::clamp(j, 0, skybox_height - 1);

        // 使用rtw_image采样像素
        const unsigned char* pixel = skybox_image->pixel_data(i, j);
        return color(
            pixel[0] / 255.0,
            pixel[1] / 255.0,
            pixel[2] / 255.0
        );
    }

    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        // 如果深度小于等于0，表示光线已经经过了最大次数的反弹，此时返回黑色
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        // 计算光线与场景的交点
        if (world.hit(r, interval(0.001, infinity), rec)) {
            // ***这里的传入的world列表是BVH节点化之后的，所以这里的hit函数会更高效***
            // ***所以BVH在该项目的代码逻辑中核心作用是在这里，简化了光线与物体的相交测试***
            // ***这里用到大量多态，先进入hittable_list的hit函数，在hittable_list的hit函数中递归进入BVH节点的hit函数，最后进入具体物体的hit函数***
            // ray_color → hittable_list::hit → bvh_node::hit → [递归到物体]
            // 如果光线与物体相交，设置交点的法向量
            ray scattered;// 反弹光线
            color attenuation;// 反弹光线衰减系数
            /*
            attenuation 在代码里是每次散射（bounce）带走/保留的光的分量，通常用 RGB 三通道表示。
            把它乘到 ray_color(...) 上就是“把上一跳返回的光按照这个材质的反射/吸收特性缩放”。
            从物理意义上讲，attenuation 更精确的叫法是“谱反射率 / albedo / reflectance”（按通道的反射率），
            所以它既表现为材质的颜色，也就是每个波段保留多少光；在简单模型里这也就是“反射率”。
            */
            color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p); // 计算自发光颜色

            if (!rec.mat->scatter(r, rec, attenuation, scattered)) // 如果材质没有散射光线
                return color_from_emission; // 直接返回自发光颜色（没有自发光颜色时则为黑色）
            
            // 否则，材质有散射光线
            // 这里前方点乘的attenuation为main函数中设置的材质反射率，实际也表现为材质的颜色
            color color_from_scatter = attenuation * ray_color(scattered, depth-1, world);// 光线递归进入下一层，方向变为材质约定的方向
            // 第一层递归中attenuation表现为材质的颜色
            return color_from_emission + color_from_scatter; // 返回自发光颜色和散射光颜色的叠加
            // depth-1 表示光线已经反弹了一次，进入下一层的递归
            // 最终像素颜色是所有光线路径贡献的积分
        }

        // // 背景色渐变
        // vec3 unit_direction = unit_vector(r.direction());
        // auto a = 0.5*(unit_direction.y() + 1.0);
        // return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);

        // 使用球谐函数计算环境光照
        if (use_spherical_harmonics && enable_skybox) {
            // 采样天空盒*球谐函数*
            vec3 unit_direction = unit_vector(r.direction());
            return sh_lighting.evaluate(unit_direction);
        } else if (enable_skybox) {
            // 采样天空盒*环境光贴图*
            vec3 unit_direction = unit_vector(r.direction());
            return sample_skybox(unit_direction);
        } else {
            // 返回背景色
            return background;
        }
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