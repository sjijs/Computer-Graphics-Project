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
    bool use_spherical_harmonics = false;

    // === 多线程渲染控制 ===
    bool enable_multithreading = true;  // 是否启用多线程渲染
    int num_threads = std::thread::hardware_concurrency();  // 线程数量，默认为CPU核心数

    // === 降噪优化控制 ===
    bool enable_stratified_sampling = false;  // 是否启用分层采样
    bool enable_progressive_rendering = false; // 是否启用渐进式渲染(帧间累积)
    int samples_per_frame = 1;  // Progressive模式下每帧的采样数(建议1-4)

    // === 性能优化控制 ===
    int tile_size = 32;  // 块状渲染的tile大小(16/32/64),越大越能减少原子操作开销

    // === Temporal降噪控制 ===
    bool enable_temporal_denoising = false;   // 是否启用时间域降噪
    double temporal_blend_factor = 0.9;     // 历史帧混合权重(0.9-0.95),越大越平滑但运动响应越慢
    int temporal_accumulation_limit = 64;    // 静止时最大累积帧数

    // === 空间降噪控制 (Joint Bilateral Filter) ===
    bool enable_spatial_denoising = false;   // 是否启用空间域降噪
    int spatial_filter_radius = 3;          // 滤波半径(1=3x3, 2=5x5)
    double spatial_sigma_color = 0.3;       // 颜色相似度阈值
    double spatial_sigma_normal = 0.5;      // 法线相似度阈值(弧度)
    double spatial_sigma_depth = 0.1;       // 深度相似度阈值

    // === Back Projection控制 ===
    bool enable_back_projection = false;    // 是否启用反向投影
    double reprojection_threshold = 2.0;    // 重投影失败阈值(像素)

    // === G-Buffer可视化 ===
    int debug_gbuffer_mode = 0;  // 0=关闭, 1=深度, 2=法线, 3=世界坐标, 4=双边滤波权重

    // === Russian Roulette控制 ===
    bool enable_russian_roulette = false;   // 是否启用俄罗斯轮盘赌(避免黑点)
    int rr_start_depth = 3;                // 从第几次弹射开始应用RR(建议3-5)
    double rr_survival_prob = 0.95;        // 基础存活概率(0.9-0.95)

    // === Outlier Removal控制 ===
    bool enable_outlier_removal = false;   // 是否启用异常值移除
    double outlier_threshold = 0.3;        // 异常值阈值(相对于邻域均值,0.3-0.7)
    int outlier_filter_radius = 3;         // 异常检测半径(1=3x3, 2=5x5)

    // === 单帧渲染控制 ===
    bool single_frame_mode = false;        // 单帧离线渲染模式
    std::string output_filename = "output.ppm"; // 输出文件名
    bool show_progress_window = true;      // 渲染时是否显示进度窗口

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
        
        // 单帧模式: 总是显示窗口(除非用户明确关闭)
        if (single_frame_mode) {
            enable_window_output = show_progress_window;
        }
        
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
        raw_accumulation.assign(image_width * image_height, color(0, 0, 0));
        accumulated_samples.assign(image_width * image_height, 0);
        total_accumulated_samples = 0;

        // 初始化 Temporal 降噪状态
        previous_frame_buffer.assign(image_width * image_height, color(0, 0, 0));
        temporal_frame_count.assign(image_width * image_height, 0);
        global_frame_count = 0;
        camera_moved_last_frame = false;

        // 初始化 G-Buffer
        current_gbuffer.assign(image_width * image_height, GBufferData());
        previous_gbuffer.assign(image_width * image_height, GBufferData());
        
        // 保存初始相机状态
        prev_camera_pos = center;
        prev_camera_u = u;
        prev_camera_v = v;
        prev_camera_w = w;

        std::atomic<bool> cancel_render{false};
        
        // 单帧模式: 只渲染一次然后保存
        if (single_frame_mode) {
            std::clog << "\n=== 单帧离线渲染模式 ===" << std::endl;
            std::clog << "分辨率: " << image_width << "x" << image_height << std::endl;
            std::clog << "采样数: " << samples_per_pixel << " spp" << std::endl;
            std::clog << "最大深度: " << max_depth << std::endl;
            std::clog << "Russian Roulette: " << (enable_russian_roulette ? "启用" : "关闭") << std::endl;
            if (enable_russian_roulette) {
                std::clog << "  - 起始深度: " << rr_start_depth << std::endl;
                std::clog << "  - 存活概率: " << rr_survival_prob << std::endl;
            }
            std::clog << "输出文件: " << output_filename << std::endl;
            std::clog << "============================\n" << std::endl;
        }
        
        // 持续渲染循环:仅当窗口关闭(或单帧模式完成)退出
        bool single_frame_done = false;
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
                
                // Progressive渲染: 清除累积缓冲区(视角变化,旧样本无效)
                std::fill(accumulation_buffer.begin(), accumulation_buffer.end(), color(0, 0, 0));
                std::fill(raw_accumulation.begin(), raw_accumulation.end(), color(0, 0, 0));
                std::fill(accumulated_samples.begin(), accumulated_samples.end(), 0);
                total_accumulated_samples = 0;
                
                // Temporal降噪: 不清除历史帧，让Back Projection处理运动
                // (如果Back Projection失败,apply_temporal_denoising会自动降低历史权重)
            } else {
                camera_moved_last_frame = false;
            }

            // 确定本帧使用的采样数
            int current_spp = enable_progressive_rendering ? samples_per_frame : samples_per_pixel;

            // G-Buffer Pass: 收集几何信息(用于降噪)
            if (enable_back_projection || enable_spatial_denoising) {
                collect_gbuffer(world);
            }

            bool completed = false;
            if (enable_multithreading && num_threads > 1) {
                completed = render_multithreaded(world, window_ptr, accumulation_buffer, display_buffer, cancel_render);
            } else {
                completed = render_singlethreaded(world, window_ptr, accumulation_buffer, display_buffer, cancel_render);
            }

            if (cancel_render.load()) break; // 渲染过程中检测到窗口关闭

            // G-Buffer调试可视化(优先级最高)
            if (debug_gbuffer_mode > 0) {
                visualize_gbuffer(display_buffer, debug_gbuffer_mode);
            } else {
                // 应用降噪管线 (线性空间 → 线性空间 → BGRA)
                std::vector<color> denoised_buffer = accumulation_buffer;
            
                // 0. Outlier Removal (异常值移除 - 最先应用)
                if (enable_outlier_removal) {
                    denoised_buffer = apply_outlier_removal(denoised_buffer);
                }
            
                // 1. Temporal降噪 (线性空间输入/输出)
                if (enable_temporal_denoising && global_frame_count > 0) {
                    denoised_buffer = apply_temporal_denoising_linear(denoised_buffer);
                }
                
                // 2. 空间降噪 (线性空间输入/输出)
                if (enable_spatial_denoising) {
                    std::vector<color> spatial_filtered(image_width * image_height);
                    for (int j = 0; j < image_height; ++j) {
                        for (int i = 0; i < image_width; ++i) {
                            spatial_filtered[j * image_width + i] = 
                                apply_bilateral_filter(denoised_buffer, i, j);
                        }
                    }
                    denoised_buffer = spatial_filtered;
                }
                
                // 3. 最终编码到显示缓冲 (线性空间 → BGRA)
                for (int i = 0; i < image_width * image_height; ++i) {
                    encode_pixel_to_bgra(denoised_buffer[i], &display_buffer[i * 4]);
                }
            } // 结束 debug_gbuffer_mode == 0 的else分支

            // 保存当前帧为历史帧 (保存降噪后的结果用于下一帧)
            if (enable_temporal_denoising) {
                // 注意: 这里应该保存经过降噪的buffer,而不是原始accumulation_buffer
                if (enable_spatial_denoising) {
                    // 如果同时开启Temporal+Spatial,保存Spatial的输出
                    // 但为了避免过度平滑,这里保存Temporal的输出
                    previous_frame_buffer = accumulation_buffer; // 保持原样,避免历史累积过度平滑
                } else {
                    previous_frame_buffer = accumulation_buffer;
                }
            }
            
            // 保存G-Buffer和相机状态
            if (enable_back_projection || enable_spatial_denoising) {
                previous_gbuffer = current_gbuffer;
                prev_camera_pos = center;
                prev_camera_u = u;
                prev_camera_v = v;
                prev_camera_w = w;
            }
            
            global_frame_count++;

            if (window_ptr) {
                window_ptr->present(display_buffer.data());
            }

            // 单帧模式: 渲染完成后保存并退出
            if (single_frame_mode && !single_frame_done) {
                single_frame_done = true;
                
                std::clog << "\n=== 渲染完成,正在保存... ===" << std::endl;
                
                // 保存到文件
                write_buffer_to_ppm(accumulation_buffer, output_filename);
                
                std::clog << "已保存到: " << output_filename << std::endl;
                std::clog << "按任意键关闭窗口或直接关闭窗口退出..." << std::endl;
                
                // 如果有窗口,保持显示直到用户关闭
                if (window_ptr) {
                    while (window_ptr->process_events()) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    }
                }
                
                break; // 退出渲染循环
            }
            
            // 实时模式: 继续下一帧
            if (!single_frame_mode) {
                // 持续渲染
            } else {
                // 单帧模式已完成,退出
                break;
            }
        }
    }

    // 重载版本：单帧离线渲染(带文件名)
    // 注意: 该版本会强制启用single_frame_mode,如需实时模式请使用无参数版本
    void render(const hittable& world, const std::string& filename) {
        single_frame_mode = true;  // 强制单帧模式
        output_filename = filename;
        show_progress_window = true;
        render(world);
    }
    
    // 重载版本：单帧离线渲染(带文件名和窗口控制)
    void render(const hittable& world, const std::string& filename, bool show_window) {
        single_frame_mode = true;  // 强制单帧模式
        output_filename = filename;
        show_progress_window = show_window;
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
    std::vector<color> raw_accumulation;   // 原始累积值(未平均)
    std::vector<int> accumulated_samples;  // 每个像素的累积样本数
    int total_accumulated_samples = 0;     // 全局累积样本计数

    // --- Temporal降噪状态 ---
    std::vector<color> previous_frame_buffer;  // 上一帧的渲染结果
    std::vector<int> temporal_frame_count;     // 每个像素的累积帧数
    int global_frame_count = 0;                // 全局帧计数
    bool camera_moved_last_frame = false;      // 上一帧相机是否移动

    // --- G-Buffer数据 (用于降噪) ---
    struct GBufferData {
        float depth;           // 深度值(到相机距离)
        vec3 world_position;   // 世界坐标
        vec3 normal;           // 世界空间法线
        bool valid;            // 是否有效(是否击中物体)
        
        GBufferData() : depth(1e10f), world_position(0,0,0), normal(0,0,0), valid(false) {}
    };
    
    std::vector<GBufferData> current_gbuffer;   // 当前帧G-Buffer
    std::vector<GBufferData> previous_gbuffer;  // 上一帧G-Buffer

    // --- 相机矩阵(用于Back Projection) ---
    point3 prev_camera_pos;       // 上一帧相机位置
    vec3 prev_camera_u, prev_camera_v, prev_camera_w; // 上一帧相机坐标系

    // 渲染实现（窗口输出）

    // G-Buffer Pass: 快速收集深度、法线等几何信息
    void collect_gbuffer(const hittable& world) {
        for (int j = 0; j < image_height; ++j) {
            for (int i = 0; i < image_width; ++i) {
                const int pixel_index = j * image_width + i;
                
                // 发射主光线(像素中心,不做抗锯齿采样)
                auto offset = vec3(0, 0, 0); // 中心采样
                auto pixel_sample = pixel00_loc
                                  + (i * pixel_delta_u)
                                  + (j * pixel_delta_v);
                
                auto ray_origin = center; // 不考虑散焦
                auto ray_direction = pixel_sample - ray_origin;
                ray r(ray_origin, ray_direction, 0.0);
                
                // 测试首次命中
                hit_record rec;
                if (world.hit(r, interval(0.001, infinity), rec)) {
                    current_gbuffer[pixel_index].valid = true;
                    current_gbuffer[pixel_index].depth = (rec.p - center).length();
                    current_gbuffer[pixel_index].world_position = rec.p;
                    current_gbuffer[pixel_index].normal = rec.normal;
                } else {
                    current_gbuffer[pixel_index].valid = false;
                    current_gbuffer[pixel_index].depth = 1e10f;
                }
            }
        }
    }

    // 计算双边滤波权重总和(用于可视化)
    double compute_bilateral_filter_weight_sum(int center_x, int center_y) const {
        const int pixel_index = center_y * image_width + center_x;
        const GBufferData& center_gb = current_gbuffer[pixel_index];
        
        if (!center_gb.valid) {
            return 0.0; // 背景像素
        }
        
        double sum_weight = 0.0;
        const int radius = spatial_filter_radius;
        
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int nx = center_x + dx;
                int ny = center_y + dy;
                
                // 边界检查
                if (nx < 0 || nx >= image_width || ny < 0 || ny >= image_height)
                    continue;
                
                const int neighbor_index = ny * image_width + nx;
                const GBufferData& neighbor_gb = current_gbuffer[neighbor_index];
                
                if (!neighbor_gb.valid) continue;
                
                // 空间距离权重
                double spatial_dist = std::sqrt(dx*dx + dy*dy);
                double spatial_weight = std::exp(-spatial_dist * spatial_dist / (2.0 * radius * radius));
                
                // 深度相似度权重
                double depth_diff = std::abs(neighbor_gb.depth - center_gb.depth) / (center_gb.depth + 1e-6);
                double depth_weight = std::exp(-depth_diff * depth_diff / (2.0 * spatial_sigma_depth * spatial_sigma_depth));
                
                // 法线相似度权重
                double normal_sim = dot(neighbor_gb.normal, center_gb.normal);
                normal_sim = std::max(0.0, normal_sim);
                double normal_weight = std::pow(normal_sim, 1.0 / spatial_sigma_normal);
                
                // 综合权重(不含颜色权重,因为这里只是可视化几何信息)
                double weight = spatial_weight * depth_weight * normal_weight;
                
                sum_weight += weight;
            }
        }
        
        return sum_weight;
    }

    // G-Buffer可视化 (调试用)
    void visualize_gbuffer(std::vector<uint8_t>& display_buffer, int mode) {
        double max_depth = 0.0;
        
        // 统计权重信息(仅mode 4需要)
        if (mode == 4) {
            double min_weight = 1e10, max_weight = 0.0, avg_weight = 0.0;
            int valid_pixels = 0;
            
            for (int i = 0; i < image_width * image_height; ++i) {
                if (current_gbuffer[i].valid) {
                    int x = i % image_width;
                    int y = i / image_width;
                    double w = compute_bilateral_filter_weight_sum(x, y);
                    min_weight = std::min(min_weight, w);
                    max_weight = std::max(max_weight, w);
                    avg_weight += w;
                    valid_pixels++;
                }
            }
            
            if (valid_pixels > 0) {
                avg_weight /= valid_pixels;
                std::clog << "\n[Bilateral Filter Weight Stats]" << std::endl;
                std::clog << "  Min: " << std::fixed << std::setprecision(2) << min_weight << std::endl;
                std::clog << "  Max: " << max_weight << std::endl;
                std::clog << "  Avg: " << avg_weight << std::endl;
                std::clog << "  Filter area: " << (2*spatial_filter_radius+1) << "x" << (2*spatial_filter_radius+1) 
                          << " = " << (2*spatial_filter_radius+1)*(2*spatial_filter_radius+1) << " pixels" << std::endl;
                std::clog << "  理论最大权重(无边缘): ~" << (2*spatial_filter_radius+1)*(2*spatial_filter_radius+1) << std::endl;
            }
        }
        
        // 第一遍:找到最大深度用于归一化
        if (mode == 1) {
            for (int i = 0; i < image_width * image_height; ++i) {
                if (current_gbuffer[i].valid) {
                    max_depth = std::max(max_depth, static_cast<double>(current_gbuffer[i].depth));
                }
            }
        }
        
        for (int i = 0; i < image_width * image_height; ++i) {
            color vis_color(0, 0, 0);
            
            if (!current_gbuffer[i].valid) {
                vis_color = color(0, 0, 0); // 背景黑色
            } else {
                switch (mode) {
                    case 1: // 深度
                        {
                            double normalized_depth = current_gbuffer[i].depth / max_depth;
                            vis_color = color(normalized_depth, normalized_depth, normalized_depth);
                        }
                        break;
                    
                    case 2: // 法线 (映射到[0,1])
                        vis_color = (current_gbuffer[i].normal + vec3(1,1,1)) * 0.5;
                        break;
                    
                    case 3: // 世界坐标 (归一化显示)
                        {
                            auto wp = current_gbuffer[i].world_position;
                            vis_color = color(
                                std::fmod(std::abs(wp.x()), 1.0),
                                std::fmod(std::abs(wp.y()), 1.0),
                                std::fmod(std::abs(wp.z()), 1.0)
                            );
                        }
                        break;
                    
                    case 4: // 双边滤波权重可视化
                        {
                            int x = i % image_width;
                            int y = i / image_width;
                            double sum_weight = compute_bilateral_filter_weight_sum(x, y);
                            
                            // 归一化权重到可见范围
                            // sum_weight通常在[0, (2*radius+1)^2]范围内
                            int filter_area = (2 * spatial_filter_radius + 1) * (2 * spatial_filter_radius + 1);
                            double normalized_weight = sum_weight / filter_area;
                            
                            // 使用热力图颜色: 蓝(低) -> 绿 -> 黄 -> 红(高)
                            if (normalized_weight < 0.25) {
                                // 蓝 -> 青
                                double t = normalized_weight / 0.25;
                                vis_color = color(0, t, 1);
                            } else if (normalized_weight < 0.5) {
                                // 青 -> 绿
                                double t = (normalized_weight - 0.25) / 0.25;
                                vis_color = color(0, 1, 1 - t);
                            } else if (normalized_weight < 0.75) {
                                // 绿 -> 黄
                                double t = (normalized_weight - 0.5) / 0.25;
                                vis_color = color(t, 1, 0);
                            } else {
                                // 黄 -> 红
                                double t = (normalized_weight - 0.75) / 0.25;
                                vis_color = color(1, 1 - t, 0);
                            }
                        }
                        break;
                    
                    default:
                        vis_color = color(1, 0, 1); // 错误:洋红色
                        break;
                }
            }
            
            encode_pixel_to_bgra(vis_color, &display_buffer[i * 4]);
        }
    }

    // 收集G-Buffer信息(修改ray_color以返回首次击中信息)
    color ray_color_with_gbuffer(const ray& r, int depth, const hittable& world, 
                                  GBufferData& gbuffer) const {
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        if (world.hit(r, interval(0.001, infinity), rec)) {
            // 首次击中时记录G-Buffer
            if (depth == max_depth) {
                gbuffer.valid = true;
                gbuffer.depth = (rec.p - center).length();
                gbuffer.world_position = rec.p;
                gbuffer.normal = rec.normal;
            }

            ray scattered;
            color attenuation;
            color color_from_emission = rec.mat->emitted(rec.u, rec.v, rec.p);

            if (!rec.mat->scatter(r, rec, attenuation, scattered))
                return color_from_emission;
            
            GBufferData dummy_gbuffer; // 递归时忽略G-Buffer
            color color_from_scatter = attenuation * ray_color_with_gbuffer(scattered, depth-1, world, dummy_gbuffer);
            return color_from_emission + color_from_scatter;
        }

        // 未击中任何物体
        if (depth == max_depth) {
            gbuffer.valid = false;
        }

        if (use_spherical_harmonics && enable_skybox) {
            vec3 unit_direction = unit_vector(r.direction());
            return sh_lighting.evaluate(unit_direction);
        } else if (enable_skybox) {
            vec3 unit_direction = unit_vector(r.direction());
            return sample_skybox(unit_direction);
        } else {
            return background;
        }
    }

    // Back Projection: 将当前帧世界坐标投影到上一帧屏幕空间
    bool back_project(const vec3& world_pos, int& prev_x, int& prev_y) const {
        // 将世界坐标转换到上一帧相机空间
        vec3 view_pos = world_pos - prev_camera_pos;
        
        // 投影到上一帧视平面
        double view_z = dot(view_pos, -prev_camera_w);
        if (view_z <= 0) return false; // 在相机后面
        
        double view_x = dot(view_pos, prev_camera_u);
        double view_y = dot(view_pos, prev_camera_v);
        
        // 转换到NDC坐标 [-1, 1]
        double ndc_x = view_x / (view_z * std::tan(degrees_to_radians(vfov/2)) * aspect_ratio);
        double ndc_y = view_y / (view_z * std::tan(degrees_to_radians(vfov/2)));
        
        // 转换到屏幕坐标
        prev_x = static_cast<int>((ndc_x + 1.0) * 0.5 * image_width);
        prev_y = static_cast<int>((1.0 - ndc_y) * 0.5 * image_height);
        
        // 检查是否在屏幕内
        return (prev_x >= 0 && prev_x < image_width && 
                prev_y >= 0 && prev_y < image_height);
    }

    // Joint Bilateral Filter: 边缘保持的空间滤波
    color apply_bilateral_filter(const std::vector<color>& input_buffer,
                                 int center_x, int center_y) const {
        const int pixel_index = center_y * image_width + center_x;
        const GBufferData& center_gb = current_gbuffer[pixel_index];
        
        if (!center_gb.valid) {
            return input_buffer[pixel_index]; // 背景像素不滤波
        }
        
        color sum_color(0, 0, 0);
        double sum_weight = 0.0;
        
        const int radius = spatial_filter_radius;
        
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                int nx = center_x + dx;
                int ny = center_y + dy;
                
                // 边界检查
                if (nx < 0 || nx >= image_width || ny < 0 || ny >= image_height)
                    continue;
                
                const int neighbor_index = ny * image_width + nx;
                const GBufferData& neighbor_gb = current_gbuffer[neighbor_index];
                
                if (!neighbor_gb.valid) continue;
                
                // 空间距离权重 (高斯)
                double spatial_dist = std::sqrt(dx*dx + dy*dy);
                double spatial_weight = std::exp(-spatial_dist * spatial_dist / (2.0 * radius * radius));
                
                // 深度相似度权重
                double depth_diff = std::abs(neighbor_gb.depth - center_gb.depth) / center_gb.depth;
                double depth_weight = std::exp(-depth_diff * depth_diff / (2.0 * spatial_sigma_depth * spatial_sigma_depth));
                
                // 法线相似度权重
                double normal_sim = dot(neighbor_gb.normal, center_gb.normal);
                normal_sim = std::max(0.0, normal_sim); // clamp to [0,1]
                double normal_weight = std::pow(normal_sim, 1.0 / spatial_sigma_normal);
                
                // 颜色相似度权重(可选,防止过度平滑)
                color color_diff = input_buffer[neighbor_index] - input_buffer[pixel_index];
                double color_dist = color_diff.length();
                double color_weight = std::exp(-color_dist * color_dist / (2.0 * spatial_sigma_color * spatial_sigma_color));
                
                // 综合权重
                double weight = spatial_weight * depth_weight * normal_weight * color_weight;

                sum_color += weight * input_buffer[neighbor_index];
                sum_weight += weight;
            }
        }
        
        if (sum_weight > 1e-6) {
            return sum_color / sum_weight;
        } else {
            return input_buffer[pixel_index];
        }
    }

    // Outlier Removal: 移除异常暗点(黑点)
    std::vector<color> apply_outlier_removal(const std::vector<color>& input_buffer) const {
        std::vector<color> output(image_width * image_height);
        
        const int radius = outlier_filter_radius;
        
        for (int y = 0; y < image_height; ++y) {
            for (int x = 0; x < image_width; ++x) {
                const int pixel_index = y * image_width + x;
                const color& center_color = input_buffer[pixel_index];
                
                // 收集邻域像素
                std::vector<double> neighbor_luminance;
                color neighbor_sum(0, 0, 0);
                int neighbor_count = 0;
                
                for (int dy = -radius; dy <= radius; ++dy) {
                    for (int dx = -radius; dx <= radius; ++dx) {
                        if (dx == 0 && dy == 0) continue; // 跳过中心像素
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        // 边界检查
                        if (nx < 0 || nx >= image_width || ny < 0 || ny >= image_height)
                            continue;
                        
                        const int neighbor_index = ny * image_width + nx;
                        const color& neighbor_color = input_buffer[neighbor_index];
                        
                        // 计算亮度(相对亮度公式: 0.299R + 0.587G + 0.114B)
                        double lum = 0.299 * neighbor_color.x() + 
                                    0.587 * neighbor_color.y() + 
                                    0.114 * neighbor_color.z();
                        
                        neighbor_luminance.push_back(lum);
                        neighbor_sum += neighbor_color;
                        neighbor_count++;
                    }
                }
                
                if (neighbor_count == 0) {
                    output[pixel_index] = center_color;
                    continue;
                }
                
                // 计算中心像素亮度
                double center_lum = 0.299 * center_color.x() + 
                                   0.587 * center_color.y() + 
                                   0.114 * center_color.z();
                
                // 计算邻域平均亮度
                double avg_neighbor_lum = 0.0;
                for (double lum : neighbor_luminance) {
                    avg_neighbor_lum += lum;
                }
                avg_neighbor_lum /= neighbor_count;
                
                // 计算邻域亮度标准差
                double variance = 0.0;
                for (double lum : neighbor_luminance) {
                    double diff = lum - avg_neighbor_lum;
                    variance += diff * diff;
                }
                double std_dev = std::sqrt(variance / neighbor_count);
                
                // 检测异常值:如果中心像素比邻域平均值暗很多
                bool is_outlier = false;
                
                if (avg_neighbor_lum > 1e-6) {
                    // 相对差异检测
                    double relative_diff = (avg_neighbor_lum - center_lum) / avg_neighbor_lum;
                    
                    // 如果中心像素比邻域暗超过阈值,且邻域有足够亮度
                    if (relative_diff > outlier_threshold && avg_neighbor_lum > 0.05) {
                        is_outlier = true;
                    }
                    
                    // 额外检测:中心像素是否是明显的黑点(接近黑色但邻域不黑)
                    if (center_lum < 0.01 && avg_neighbor_lum > 0.1) {
                        is_outlier = true;
                    }
                }
                
                if (is_outlier) {
                    // 用邻域中值替换(更稳定)
                    std::sort(neighbor_luminance.begin(), neighbor_luminance.end());
                    double median_lum = neighbor_luminance[neighbor_luminance.size() / 2];
                    
                    // 找到亮度最接近中值的邻域像素
                    color replacement_color = neighbor_sum / neighbor_count; // 默认用平均值
                    
                    // 或者使用智能插值:保持色调,只调整亮度
                    if (center_lum > 1e-6) {
                        // 保持原色调,放大亮度
                        double scale = median_lum / center_lum;
                        scale = std::min(scale, 10.0); // 限制最大放大倍数
                        replacement_color = center_color * scale;
                    } else {
                        // 完全黑色,用邻域平均色
                        replacement_color = neighbor_sum / neighbor_count;
                    }
                    
                    output[pixel_index] = replacement_color;
                } else {
                    output[pixel_index] = center_color;
                }
            }
        }
        
        return output;
    }

    // Temporal降噪: 混合当前帧和历史帧 (支持Back Projection) - 线性空间版本
    std::vector<color> apply_temporal_denoising_linear(const std::vector<color>& current_frame) {
        std::vector<color> output(image_width * image_height);
        
        for (int y = 0; y < image_height; ++y) {
            for (int x = 0; x < image_width; ++x) {
                const int i = y * image_width + x;
                const color& current_color = current_frame[i];
                const GBufferData& current_gb = current_gbuffer[i];
                
                color history_color = previous_frame_buffer[i];
                bool reprojection_valid = true;
                
                // Back Projection: 尝试找到上一帧对应像素
                if (enable_back_projection && current_gb.valid) {
                    int prev_x, prev_y;
                    if (back_project(current_gb.world_position, prev_x, prev_y)) {
                        const int prev_i = prev_y * image_width + prev_x;
                        const GBufferData& prev_gb = previous_gbuffer[prev_i];
                        
                        if (prev_gb.valid) {
                            // 验证重投影质量(深度和法线一致性)
                            double depth_diff = std::abs(current_gb.depth - prev_gb.depth) / (current_gb.depth + 1e-6);
                            double normal_sim = dot(current_gb.normal, prev_gb.normal);
                            
                            // 放宽重投影验证阈值,提高运动场景的降噪效果
                            if (depth_diff < 0.2 && normal_sim > 0.9) {
                                // 重投影成功,使用重投影位置的历史颜色
                                history_color = previous_frame_buffer[prev_i];
                            } else {
                                reprojection_valid = false; // 重投影失败,降低历史权重
                            }
                        } else {
                            reprojection_valid = false;
                        }
                    } else {
                        reprojection_valid = false; // 投影到屏幕外
                    }
                }
                
                // 计算混合权重
                int& frame_count = temporal_frame_count[i];
                if (!reprojection_valid && camera_moved_last_frame) {
                    // 相机移动且重投影失败: 降低历史权重但不完全清零
                    frame_count = std::max(1, frame_count / 4); // 保留部分历史
                } else if (!reprojection_valid) {
                    frame_count = 0; // 静止场景下重投影失败才完全重置
                } else {
                    frame_count = std::min(frame_count + 1, temporal_accumulation_limit);
                }
                
                double history_weight = temporal_blend_factor * std::min(1.0, frame_count / 8.0);
                if (!reprojection_valid) {
                    history_weight *= 0.5; // 降低不可靠历史的权重
                }
                double current_weight = 1.0 - history_weight;
                
                // 指数移动平均
                color blended_color = current_weight * current_color + history_weight * history_color;
                
                // 颜色夹紧(防止拖影) - 相机移动时放宽限制
                const double clamp_factor = camera_moved_last_frame ? 2.0 : 1.5;
                for (int c = 0; c < 3; ++c) {
                    double diff = std::abs(blended_color[c] - current_color[c]);
                    double max_diff = std::max(0.01, current_color[c] * clamp_factor);
                    if (diff > max_diff) {
                        blended_color[c] = 0.5 * current_color[c] + 0.5 * history_color[c];
                        frame_count = std::max(1, frame_count / 2);
                    }
                }
                
                output[i] = blended_color;
            }
        }
        
        return output;
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
        const int current_spp = enable_progressive_rendering ? samples_per_frame : samples_per_pixel; // 每像素采样数
        const int sqrt_spp = static_cast<int>(std::sqrt(current_spp)); // 分层采样的行列数
        const bool use_stratified = enable_stratified_sampling && (sqrt_spp * sqrt_spp == current_spp); // 是否使用分层采样

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
                                // 累积原始值(不平均)
                                raw_accumulation[pixel_index] += pixel_color;
                                accumulated_samples[pixel_index] += current_spp;
                                
                                // 计算平均颜色存到accumulation_buffer
                                int total_samples = accumulated_samples[pixel_index];
                                accumulation_buffer[pixel_index] = raw_accumulation[pixel_index] / total_samples;
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
                    // 累积原始值(不平均)
                    raw_accumulation[pixel_index] += pixel_color;
                    accumulated_samples[pixel_index] += current_spp;
                    
                    // 计算平均颜色
                    int total_samples = accumulated_samples[pixel_index];
                    accumulation_buffer[pixel_index] = raw_accumulation[pixel_index] / total_samples;
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

    // 将线性空间颜色编码为BGRA格式
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

    // 将累积缓冲区写入PPM文件
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

    // 初始化相机参数
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
        // Russian Roulette: 用概率方法替代硬深度限制,避免黑点
        if (enable_russian_roulette && depth < max_depth - rr_start_depth) {
            // 计算存活概率(基于能量守恒,越深概率越低)
            double survival_prob = rr_survival_prob * std::pow(0.95, max_depth - depth - rr_start_depth);
            
            if (random_double() > survival_prob) {
                // 路径终止,但不返回黑色,而是返回环境光
                if (use_spherical_harmonics && enable_skybox) {
                    vec3 unit_direction = unit_vector(r.direction());
                    return sh_lighting.evaluate(unit_direction);
                } else if (enable_skybox) {
                    vec3 unit_direction = unit_vector(r.direction());
                    return sample_skybox(unit_direction);
                } else {
                    return background;
                }
            }
        }
        
        // 传统深度限制(作为保险,防止无限递归)
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
            
            // 递归计算散射光
            color color_from_scatter = attenuation * ray_color(scattered, depth-1, world);
            
            // Russian Roulette概率补偿(保持无偏估计)
            if (enable_russian_roulette && depth < max_depth - rr_start_depth) {
                double survival_prob = rr_survival_prob * std::pow(0.95, max_depth - depth - rr_start_depth);
                color_from_scatter = color_from_scatter / survival_prob;
            }
            
            return color_from_emission + color_from_scatter;
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