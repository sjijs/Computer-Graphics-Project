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
#include <random>

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

    // 相机圆周运动参数 
    bool   enable_motion = false;       // 是否启用相机运动
    point3 center_orbit = point3(0,0,0); // 圆周运动的圆心
    double radius_orbit = 5.0;          // 圆周运动的半径
    double angular_velocity = 0.1;      // 角速度(弧度/帧)
    double current_angle = 0.0;         // 当前角度(弧度)

    std::string skybox_filename = "skybox.ppm";  // 天空盒贴图文件名

    std::string sh_coeffs_filename = "skybox_sh.txt";  // 球谐系数文件名
    bool use_spherical_harmonics = true;

    // === 多线程渲染控制 ===
    bool enable_multithreading = true;  // 是否启用多线程渲染
    int num_threads = std::thread::hardware_concurrency();  // 线程数量，默认为CPU核心数

    // === 降噪优化控制 ===
    bool enable_stratified_sampling = true;  // 是否启用分层采样
    bool enable_progressive_rendering = true; // 是否启用渐进式渲染(帧间累积)
    int samples_per_frame = 1;  // Progressive模式下每帧的采样数(建议1-4)

    // === 性能优化控制 ===
    int tile_size = 32;  // 块状渲染的tile大小(16/32/64),越大越能减少原子操作开销

    // === Temporal降噪控制 ===
    bool enable_temporal_denoising = true;   // 是否启用时间域降噪
    double temporal_blend_factor = 0.92;     // 历史帧混合权重(0.9-0.95),越大越平滑但运动响应越慢
    int temporal_accumulation_limit = 64;    // 静止时最大累积帧数

    // 天空盒贴图读取(贴图式全局光照)
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

        // 初始化 Progressive 渲染状态
        accumulated_samples.assign(image_width * image_height, 0);
        total_accumulated_samples = 0;

        // 初始化 Temporal 降噪状态
        previous_frame_buffer.assign(image_width * image_height, color(0, 0, 0));
        temporal_frame_count.assign(image_width * image_height, 0);
        global_frame_count = 0;
        camera_moved_last_frame = false;

        std::atomic<bool> cancel_render{false};
        // 持续渲染循环:仅当窗口关闭(或无窗口模式下完成一次)退出
        while (true) {
            // 处理窗口事件(优先判断关闭)
            if (window_ptr) {
                if (!window_ptr->process_events()) {
                    cancel_render.store(true);
                    break; // 退出整体持续渲染
                }
            }

            // 更新相机位置(如果启用运动)
            if (enable_motion) {
                update_camera_position();
                camera_moved_last_frame = true;
                // 重置累积缓冲区,因为相机位置已改变
                std::fill(accumulation_buffer.begin(), accumulation_buffer.end(), color(0, 0, 0));
                std::fill(accumulated_samples.begin(), accumulated_samples.end(), 0);
                total_accumulated_samples = 0;
                
                // Temporal降噪: 重置历史帧计数
                if (enable_temporal_denoising) {
                    std::fill(temporal_frame_count.begin(), temporal_frame_count.end(), 0);
                }
            } else {
                camera_moved_last_frame = false;
            }

            // 确定本帧使用的采样数
            int current_spp = enable_progressive_rendering ? samples_per_frame : samples_per_pixel;

            bool completed = false;
            if (enable_multithreading && num_threads > 1) {
                completed = render_multithreaded(world, window_ptr, accumulation_buffer, display_buffer, cancel_render);
            } else {
                completed = render_singlethreaded(world, window_ptr, accumulation_buffer, display_buffer, cancel_render);
            }

            if (cancel_render.load()) break; // 渲染过程中检测到窗口关闭

            // Temporal降噪: 混合当前帧和历史帧
            if (enable_temporal_denoising && global_frame_count > 0 && !camera_moved_last_frame) {
                apply_temporal_denoising(accumulation_buffer, display_buffer);
            } else {
                // 不启用Temporal或首帧: 直接复制到显示缓冲
                for (int i = 0; i < image_width * image_height; ++i) {
                    encode_pixel_to_bgra(accumulation_buffer[i], &display_buffer[i * 4]);
                }
            }

            // 保存当前帧为历史帧
            if (enable_temporal_denoising) {
                previous_frame_buffer = accumulation_buffer;
            }
            global_frame_count++;

            if (window_ptr) {
                window_ptr->present(display_buffer.data());
            }

            // 持续渲染：此处可以加入 progressive 策略（增量样本）占位
        }
    }

    // 重载版本：允许指定输出文件名(已废弃,仅保留窗口渲染)
    void render(const hittable& world, const std::string& filename) {
        // 忽略filename参数,直接调用窗口渲染
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

    // 球谐函数对象
    SphericalHarmonics sh_lighting{3};  // 3阶球谐函数

    // --- 天空盒数据 ---
    int skybox_width = 0;
    int skybox_height = 0;
    std::unique_ptr<rtw_image> skybox_image; // 使用rtw_image加载天空盒

    // --- Progressive 渲染状态 ---
    std::vector<int> accumulated_samples; // 每个像素的累积样本数
    int total_accumulated_samples = 0;    // 全局累积样本计数

    // --- Temporal降噪状态 ---
    std::vector<color> previous_frame_buffer;  // 上一帧的渲染结果
    std::vector<int> temporal_frame_count;     // 每个像素的累积帧数
    int global_frame_count = 0;                // 全局帧计数
    bool camera_moved_last_frame = false;      // 上一帧相机是否移动

    // === 渲染实现（窗口输出） ===

    // Temporal降噪: 混合当前帧和历史帧
    void apply_temporal_denoising(const std::vector<color>& current_frame,
                                  std::vector<uint8_t>& display_buffer) {
        for (int i = 0; i < image_width * image_height; ++i) {
            const color& current_color = current_frame[i];
            const color& previous_color = previous_frame_buffer[i];
            
            // 计算混合权重: 静止时累积更多历史帧
            int& frame_count = temporal_frame_count[i];
            frame_count = std::min(frame_count + 1, temporal_accumulation_limit);
            
            // 自适应混合: 累积的帧数越多,历史权重越大
            double history_weight = temporal_blend_factor * std::min(1.0, frame_count / 8.0);
            double current_weight = 1.0 - history_weight;
            
            // 指数移动平均 (Exponential Moving Average)
            color blended_color = current_weight * current_color + history_weight * previous_color;
            
            // 可选: 颜色夹紧防止拖影 (Neighborhood Clamping)
            // 这里简化版本,后续可以添加3x3邻域统计
            const double clamp_factor = 1.5;
            for (int c = 0; c < 3; ++c) {
                double diff = std::abs(blended_color[c] - current_color[c]);
                double max_diff = std::max(0.01, current_color[c] * clamp_factor);
                if (diff > max_diff) {
                    // 检测到突变(如新出现的高光),降低历史权重
                    blended_color[c] = 0.5 * current_color[c] + 0.5 * previous_color[c];
                    frame_count = std::max(1, frame_count / 2); // 重置累积
                }
            }
            
            encode_pixel_to_bgra(blended_color, &display_buffer[i * 4]);
        }
    }

    bool render_multithreaded(const hittable& world,
                              RenderWindow* window,
                              std::vector<color>& accumulation_buffer,
                              std::vector<uint8_t>& display_buffer,
                              std::atomic<bool>& cancel_render) {
        const int worker_count = std::max(1, std::min(num_threads, image_height));
        
        // 块状渲染: 计算tile数量
        const int tiles_x = (image_width + tile_size - 1) / tile_size;
        const int tiles_y = (image_height + tile_size - 1) / tile_size;
        const int total_tiles = tiles_x * tiles_y;
        
        std::atomic<int> next_tile{0};
        std::atomic<int> completed_tiles{0};

        // 确定采样参数
        const int current_spp = enable_progressive_rendering ? samples_per_frame : samples_per_pixel;
        const int sqrt_spp = static_cast<int>(std::sqrt(current_spp));
        const bool use_stratified = enable_stratified_sampling && (sqrt_spp * sqrt_spp == current_spp);

        std::vector<std::thread> threads;
        threads.reserve(worker_count);

        // 启动固定数量线程
        for (int t = 0; t < worker_count; ++t) {
            threads.emplace_back([&, t]() {
                // RNG - 每个线程独立的随机数生成器,避免锁竞争
                std::mt19937 rng(std::random_device{}() + t * 997);
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                
                // 局部lambda 使用线程私有的 rng 和 dist（不使用 thread_local，以避免捕获静态存储持续时间的变量）
                auto local_random = [&rng, &dist]() -> double { return dist(rng); };
                
                while (true) {
                    if (cancel_render.load()) break;
                    
                    int tile_idx = next_tile.fetch_add(1);
                    if (tile_idx >= total_tiles) break;

                    // 计算tile的起始坐标
                    int tile_x = tile_idx % tiles_x;
                    int tile_y = tile_idx / tiles_x;
                    int start_i = tile_x * tile_size;
                    int start_j = tile_y * tile_size;
                    int end_i = std::min(start_i + tile_size, image_width);
                    int end_j = std::min(start_j + tile_size, image_height);

                    // 渲染整个tile
                    for (int j = start_j; j < end_j; ++j) {
                        if (cancel_render.load()) break;
                        
                        for (int i = start_i; i < end_i; ++i) {
                            color pixel_color(0, 0, 0);
                            
                            // 采样循环
                            for (int s = 0; s < current_spp; ++s) {
                                ray r;
                                if (use_stratified) {
                                    r = get_ray_stratified(i, j, s, sqrt_spp);
                                } else {
                                    r = get_ray(i, j);
                                }
                                pixel_color += ray_color(r, max_depth, world);
                            }
                            
                            // Progressive 模式:累积而非替换
                            const int pixel_index = j * image_width + i;
                            if (enable_progressive_rendering) {
                                accumulation_buffer[pixel_index] += pixel_color;
                                accumulated_samples[pixel_index] += current_spp;
                                
                                // 计算平均颜色 (存到accumulation_buffer,后续由Temporal处理)
                                int total_samples = accumulated_samples[pixel_index];
                                accumulation_buffer[pixel_index] = (accumulation_buffer[pixel_index] / total_samples);
                            } else {
                                // 传统模式:直接平均本帧样本
                                color scaled_color = pixel_color / current_spp;
                                accumulation_buffer[pixel_index] = scaled_color;
                            }
                        }
                    }
                    
                    completed_tiles.fetch_add(1);
                }
            });
        }

        int last_reported = -1;
        auto last_present = std::chrono::steady_clock::now();
        auto start_time = std::chrono::steady_clock::now();

        // 主线程负责监控进度和窗口事件
        while (!cancel_render.load()) {
            int done = completed_tiles.load();
            if (done != last_reported) {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration<double>(now - start_time).count();
                double fps = (done > 0 && elapsed > 0) ? done / elapsed : 0;
                
                if (enable_progressive_rendering && total_accumulated_samples > 0) {
                    std::clog << "\rProgress: " << done << "/" << total_tiles
                              << " tiles (" << std::fixed << std::setprecision(1)
                              << (100.0 * done / std::max(1, total_tiles)) << "%) "
                              << "| Accumulated: " << total_accumulated_samples << " spp "
                              << "| " << std::setprecision(1) << fps << " tiles/s " << std::flush;
                } else {
                    std::clog << "\rProgress: " << done << "/" << total_tiles
                              << " tiles (" << std::fixed << std::setprecision(1)
                              << (100.0 * done / std::max(1, total_tiles)) << "%) "
                              << "| " << std::setprecision(1) << fps << " tiles/s " << std::flush;
                }
                last_reported = done;
            }

            if (done >= total_tiles) break;

            if (window) {
                if (!window->process_events()) { cancel_render.store(true); break; }
                auto now = std::chrono::steady_clock::now();
                if (now - last_present > std::chrono::milliseconds(16)) {
                    window->present(display_buffer.data());
                    last_present = now;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        }

        // 等待线程
        for (auto& th : threads) if (th.joinable()) th.join();

        // 更新全局累积计数
        if (enable_progressive_rendering) {
            total_accumulated_samples += current_spp;
        }

        if (window && !cancel_render.load()) {
            window->process_events();
            window->present(display_buffer.data());
        }

        auto end_time = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration<double>(end_time - start_time).count();
        std::clog << std::endl;
        std::clog << "Frame time: " << std::fixed << std::setprecision(3) 
                  << total_time << "s (" << (1.0/total_time) << " fps)" << std::endl;
        
        return !cancel_render.load();
    }

    bool render_singlethreaded(const hittable& world,
                               RenderWindow* window,
                               std::vector<color>& accumulation_buffer,
                               std::vector<uint8_t>& display_buffer,
                               std::atomic<bool>& cancel_render) {
        const int current_spp = enable_progressive_rendering ? samples_per_frame : samples_per_pixel;
        const int sqrt_spp = static_cast<int>(std::sqrt(current_spp));
        const bool use_stratified = enable_stratified_sampling && (sqrt_spp * sqrt_spp == current_spp);

        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;

            if (window && !window->process_events()) { cancel_render.store(true); return false; }
            if (cancel_render.load()) return false;

            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0, 0, 0);
                
                for (int sample = 0; sample < current_spp; ++sample) {
                    ray r;
                    if (use_stratified) {
                        r = get_ray_stratified(i, j, sample, sqrt_spp);
                    } else {
                        r = get_ray(i, j);
                    }
                    pixel_color += ray_color(r, max_depth, world);
                }

                const int pixel_index = j * image_width + i;
                if (enable_progressive_rendering) {
                    accumulation_buffer[pixel_index] += pixel_color;
                    accumulated_samples[pixel_index] += current_spp;
                    
                    int total_samples = accumulated_samples[pixel_index];
                    accumulation_buffer[pixel_index] = (accumulation_buffer[pixel_index] / total_samples);
                } else {
                    color scaled_color = pixel_color / current_spp;
                    accumulation_buffer[pixel_index] = scaled_color;
                }
            }
        }

        if (enable_progressive_rendering) {
            total_accumulated_samples += current_spp;
        }

        std::clog << std::endl;
        return !cancel_render.load();
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

    // 更新相机位置(用于圆周运动)
    void update_camera_position(double delta_time = 1.0) {
        if (!enable_motion) return;

        // 更新角度
        current_angle += angular_velocity * delta_time;
        
        // 保持角度在 [0, 2π) 范围内
        if (current_angle >= 2 * pi) {
            current_angle -= 2 * pi;
        }

        // 计算新的相机位置(在XZ平面上绕圆心旋转,Y坐标保持不变)
        // 这里假设相机在水平面上旋转,垂直方向由初始lookfrom的Y坐标决定
        double y_height = lookfrom.y(); // 保持初始的高度
        lookfrom = point3(
            center_orbit.x() + radius_orbit * std::cos(current_angle),
            y_height,
            center_orbit.z() + radius_orbit * std::sin(current_angle)
        );

        // 重新初始化相机参数(重新计算视口、像素增量等)
        initialize();
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

    // 分层采样版本的 get_ray
    ray get_ray_stratified(int i, int j, int sample_index, int sqrt_spp) const {
        auto offset = sample_square_stratified(sample_index, sqrt_spp);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
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

    // 分层采样 - 降低噪声的关键优化
    // 将像素划分为 sqrt(n) x sqrt(n) 的网格,每个子区域采样一次
    vec3 sample_square_stratified(int sample_index, int sqrt_spp) const {
        if (sqrt_spp <= 1) {
            return sample_square(); // 退化为普通随机采样
        }

        // 计算当前样本所在的网格位置
        int grid_x = sample_index % sqrt_spp;
        int grid_y = sample_index / sqrt_spp;

        // 网格大小
        double grid_size = 1.0 / sqrt_spp;

        // 在网格内随机偏移
        double offset_x = (grid_x + random_double()) * grid_size - 0.5;
        double offset_y = (grid_y + random_double()) * grid_size - 0.5;

        return vec3(offset_x, offset_y, 0);
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