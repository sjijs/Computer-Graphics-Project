#include "rtweekend.h"

#include <fstream>
#include <cmath>
#include <limits>
#include <chrono>

#include "camera.h"
#include "constant_medium.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "quad.h"
#include "triangle.h"
#include "cycle.h"
#include "sphere.h"
#include "octahedron.h"
#include "annulus.h"
#include "bvh.h"
#include "texture.h"
#include "model_loader.h"

void bouncing_spheres() {
    SphericalHarmonics sh_lighting(3);
    sh_lighting.generateFromEnvironmentMap("skybox.ppm");
    sh_lighting.saveCoefficients("skybox_sh.txt");

    hittable_list world;

    // 添加地面 - 大球体作为地面
    // auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    // world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));
    // 棋盘材质地面
    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));
    // 噪声材质地面
    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));

    // 随机生成小物体（球体和八面体混合）
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            auto choose_shape = random_double(); // 新增：选择形状
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> object_material;

                if (choose_mat < 0.8) {
                    // 漫反射材质
                    auto albedo = color::random() * color::random();
                    object_material = make_shared<lambertian>(albedo);
                    auto center2 = center + vec3(0, random_double(0,.5), 0);
                    world.add(make_shared<sphere>(center, center2, 0.2, object_material));
                } else if (choose_mat < 0.95) {
                    // 金属材质
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    object_material = make_shared<metal>(albedo, fuzz);
                } else {
                    // 玻璃材质
                    object_material = make_shared<dielectric>(1.5);
                }

                // 根据随机值选择形状：50%球体，50%八面体
                if (choose_shape < 0.5) {
                    world.add(make_shared<sphere>(center, 0.2, object_material));
                } else {
                    world.add(make_shared<octahedron>(center, 0.4, object_material)); // 八面体稍大一些
                }
            }
        }
    }

    // 主要展示物体 - 三个大物体
    
    // 中央：玻璃球体
    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    // 左侧：漫反射八面体
    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<octahedron>(point3(-4, 1, 0), 2.0, material2));

    // 右侧：金属八面体
    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<octahedron>(point3(4, 1, 0), 2.0, material3));

    // 额外添加一些有趣的八面体展示不同材质效果
    
    // 前方左：彩色玻璃八面体
    auto glass_material = make_shared<dielectric>(1.8);
    world.add(make_shared<octahedron>(point3(-2, 0.5, 2), 1.0, glass_material));

    // 前方右：有模糊的金属八面体
    auto fuzzy_metal = make_shared<metal>(color(0.8, 0.3, 0.3), 0.3);
    world.add(make_shared<octahedron>(point3(2, 0.5, 2), 1.0, fuzzy_metal));

    // 后方：玻璃八面体
    auto green_material = make_shared<dielectric>(1.5);
    world.add(make_shared<octahedron>(point3(0, 0.8, -3), 1.6, green_material));

    world = hittable_list(make_shared<bvh_node>(world)); // 使用BVH加速场景
    // 将所有的物体包裹在一个BVH节点中
    // world中的所有节点在此时已经不再是添加时的sphere或octahedron对象，而是BVH树的节点

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width  = 800;
    cam.samples_per_pixel = 1; // 每个像素的随机样本数，这里的随机采样不只可以实现抗锯齿，还可以实现漫反射材质表面的真实样子
    // 如果随机样本数足够多，最终渲染出来的图像会更加平滑和真实
    // 反之如果该样本数为1，渲染出来的图像会出现明显的锯齿和噪点
    cam.max_depth = 100;  // 最大反弹次数

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0.6;
    cam.focus_dist    = 10.0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;

    // 设置天空盒文件名
    cam.skybox_filename = "skybox.ppm";
    cam.enable_multithreading = true;
    cam.num_threads = 28;

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(world, "examples_1spp_1.ppm");
}

void checkered_spheres() {
    hittable_list world;

    auto checker = make_shared<checker_texture>(0.32, color(.2, .3, .1), color(.9, .9, .9));

    world.add(make_shared<sphere>(point3(0,-10, 0), 10, make_shared<lambertian>(checker)));
    world.add(make_shared<sphere>(point3(0, 10, 0), 10, make_shared<lambertian>(checker)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;

    // 设置天空盒文件名
    cam.skybox_filename = "skybox.ppm";

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(world);
}

void earth() {
    auto earth_texture = make_shared<image_texture>("solar system/earth_day_8k.jpg");
    auto earth_surface = make_shared<lambertian>(earth_texture);
    auto globe = make_shared<sphere>(point3(0,0,0), 2, earth_surface);

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(0,0,12);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;

    // 设置天空盒文件名
    cam.skybox_filename = "galaxy.jpg";

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(hittable_list(globe));
}

void perlin_spheres() {
    hittable_list world;

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 20;
    cam.lookfrom = point3(13,2,3);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;

    // 设置天空盒文件名
    cam.skybox_filename = "skybox.ppm";

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(world);
}

void quads() {
    hittable_list world;

    // Materials
    auto left_red     = make_shared<metal>(color(1.0, 0.2, 0.2), 0.0);
    auto back_green   = make_shared<metal>(color(0.2, 1.0, 0.2), 0.0);
    auto right_blue   = make_shared<metal>(color(0.2, 0.2, 1.0), 0.0);
    auto upper_orange = make_shared<metal>(color(1.0, 0.5, 0.0), 0.0);
    auto lower_teal   = make_shared<metal>(color(0.2, 0.8, 0.8), 0.0);

    // Quads
    world.add(make_shared<cycle>(point3(-3,-2, 5), vec3(0, 0,-4), vec3(0, 4, 0), left_red));
    world.add(make_shared<cycle>(point3(-2,-2, 0), vec3(4, 0, 0), vec3(0, 4, 0), back_green));
    world.add(make_shared<cycle>(point3( 3,-2, 1), vec3(0, 0, 4), vec3(0, 4, 0), right_blue));
    world.add(make_shared<cycle>(point3(-2, 3, 1), vec3(4, 0, 0), vec3(0, 0, 4), upper_orange));
    world.add(make_shared<cycle>(point3(-2,-3, 5), vec3(4, 0, 0), vec3(0, 0,-4), lower_teal));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 100;
    cam.max_depth         = 50;

    cam.vfov     = 80;
    cam.lookfrom = point3(0,0,9);
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;

    // 设置天空盒文件名
    cam.skybox_filename = "skybox.ppm";

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(world);
}

void simple_light() {
    hittable_list world;

    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));
    world.add(make_shared<sphere>(point3(0,2,0), 2, make_shared<lambertian>(pertext)));

    auto difflight = make_shared<diffuse_light>(color(4,4,4));
    // 请注意，该灯比 (1,1,1)亮。这使得它足够亮，可以照亮事物。
    // 颜色值不仅可以表示光的颜色，还可以表示光的强度
    // 例如，(4,4,4) 表示该灯的光强是基准的 4 倍
    // 在漫反射材质中，颜色值会乘上漫反射参数，导致能量损失，所以基准值越高，“照亮能力”越强
    // 因为光反弹多次后仍能保持高基准值，就能将更大的范围照亮
    // 这就是为什么现代渲染器使用 HDR（High Dynamic Range）
      /*
        // 传统 LDR：颜色值限制在 [0,1]
        color ldr_color = clamp(final_color, 0.0, 1.0);

        // HDR：允许超过 1.0 的值
        color hdr_color = final_color; // 可能是 (4.5, 2.1, 8.3)

        // 最后通过色调映射转换为显示范围
        color display_color = tone_mapping(hdr_color);
      */
    // 在该程序实际输出图像时，超过 1.0 的值会被截断，仍输出（255，255，255）表示最高值
    world.add(make_shared<sphere>(point3(0,7,0), 2, difflight));
    world.add(make_shared<quad>(point3(3,1,-2), vec3(2,0,0), vec3(0,2,0), difflight));

    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 400;
    cam.samples_per_pixel = 1000;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 20;
    cam.lookfrom = point3(26,3,6);
    cam.lookat   = point3(0,2,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;
    cam.enable_skybox = false; // 该场景不需要天空盒

    cam.render(world);
}

void cornell_box() {
    hittable_list world;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(15, 15, 15));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(343, 554, 332), vec3(-130,0,0), vec3(0,0,-105), light));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(555,555,555), vec3(-555,0,0), vec3(0,0,-555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));
    world.add(box1);

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));
    world.add(box2);

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;
    cam.enable_skybox = false; // 该场景不需要天空盒

    cam.render(world);
}

void cornell_smoke() {
    hittable_list world;

    auto red   = make_shared<lambertian>(color(.65, .05, .05));
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    auto green = make_shared<lambertian>(color(.12, .45, .15));
    auto light = make_shared<diffuse_light>(color(7, 7, 7));

    world.add(make_shared<quad>(point3(555,0,0), vec3(0,555,0), vec3(0,0,555), green));
    world.add(make_shared<quad>(point3(0,0,0), vec3(0,555,0), vec3(0,0,555), red));
    world.add(make_shared<quad>(point3(113,554,127), vec3(330,0,0), vec3(0,0,305), light));
    world.add(make_shared<quad>(point3(0,555,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,0), vec3(555,0,0), vec3(0,0,555), white));
    world.add(make_shared<quad>(point3(0,0,555), vec3(555,0,0), vec3(0,555,0), white));

    shared_ptr<hittable> box1 = box(point3(0,0,0), point3(165,330,165), white);
    box1 = make_shared<rotate_y>(box1, 15);
    box1 = make_shared<translate>(box1, vec3(265,0,295));

    shared_ptr<hittable> box2 = box(point3(0,0,0), point3(165,165,165), white);
    box2 = make_shared<rotate_y>(box2, -18);
    box2 = make_shared<translate>(box2, vec3(130,0,65));

    world.add(make_shared<constant_medium>(box1, 0.01, color(0,0,0)));
    world.add(make_shared<constant_medium>(box2, 0.01, color(1,1,1)));

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = 600;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(278, 278, -800);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;
    cam.enable_skybox = false; // 该场景不需要天空盒

    cam.render(world);
}

/**
 * 太阳系场景函数 - 重新设计构图版本
 * 特点：
 * - 太阳在右侧，巨大且只露出一部分
 * - 8大行星依次排列在图像中
 * - 使用新的annulus类显示土星环
 * - 银河系背景环境光
 */
void solar_system() {
    // === 首先生成银河系贴图的球谐系数 ===
    std::clog << "正在生成银河系环境光球谐系数..." << std::endl;
    SphericalHarmonics sh_lighting(3);
    sh_lighting.generateFromEnvironmentMap("solar system/sky_8k.jpg");
    sh_lighting.saveCoefficients("galaxy_sh.txt");
    std::clog << "银河系球谐系数生成完成！" << std::endl;

    hittable_list world;

    // === 太阳：巨大且位于右侧，只露出一小部分 ===
    // 太阳作为强发光体，位置调整到右侧画面外
    auto sun_material = make_shared<diffuse_light>(color(6.0, 4.5, 2.0));  // 更强的发光
    auto sun = make_shared<sphere>(point3(25, 0, 0), 12.0, sun_material);  // 巨大的太阳
    world.add(sun);

    // === 太阳日冕层：更大的体积介质效果 ===
    auto corona_boundary = make_shared<sphere>(point3(25, 0, 0), 16.0, make_shared<dielectric>(1.0));
    world.add(corona_boundary);
    // 日冕介质：橙黄色，低密度
    world.add(make_shared<constant_medium>(corona_boundary, 0.02, color(1.0, 0.7, 0.3), 16.0, point3(25, 0, 0)));

    // === 行星依次排列（从左到右，距离适中便于观察） ===
    
    // 海王星 - 最左侧，最远的行星
    auto neptune_texture = make_shared<image_texture>("solar system/neptune_2k.jpg");
    auto neptune_material = make_shared<lambertian>(neptune_texture);
    world.add(make_shared<sphere>(point3(-15, 0, 2), 1.2, neptune_material));

    // 天王星 - 倾斜的冰巨星
    auto uranus_texture = make_shared<image_texture>("solar system/uranus_2k.jpg");
    auto uranus_material = make_shared<lambertian>(uranus_texture);
    world.add(make_shared<sphere>(point3(-12, 0, -1), 1.4, uranus_material));

    // 土星 - 带美丽光环的气态巨星
    auto saturn_texture = make_shared<image_texture>("solar system/saturn_8k.jpg");
    auto saturn_material = make_shared<lambertian>(saturn_texture);
    auto saturn_center = point3(-8, 0, 3);
    world.add(make_shared<sphere>(saturn_center, 2.8, saturn_material));
    
    // 土星环 - 使用新的annulus类
    auto ring_texture = make_shared<image_texture>("solar system/saturn_ring_8k.png");
    auto ring_material = make_shared<lambertian>(ring_texture);
    // 创建水平环状结构，内径3.5，外径6.0
    world.add(make_shared<annulus>(
        saturn_center, 
        vec3(6.0, 0, 0),     // u方向向量
        vec3(0, 0, 6.0),     // v方向向量
        3.5,                 // 内半径
        6.0,                 // 外半径
        ring_material
    ));

    // 木星 - 气态巨行星
    auto jupiter_texture = make_shared<image_texture>("solar system/jupiter_8k.jpg");
    auto jupiter_material = make_shared<lambertian>(jupiter_texture);
    world.add(make_shared<sphere>(point3(-4, 0, -2), 3.2, jupiter_material));

    // 火星 - 红色行星
    auto mars_texture = make_shared<image_texture>("solar system/mars_8k.jpg");
    auto mars_material = make_shared<lambertian>(mars_texture);
    world.add(make_shared<sphere>(point3(0, 0, 1), 0.6, mars_material));

    // 地球 - 蓝色家园
    auto earth_texture = make_shared<image_texture>("solar system/earth_day_8k.jpg");
    auto earth_material = make_shared<lambertian>(earth_texture);
    world.add(make_shared<sphere>(point3(3, 0, -1), 1.0, earth_material));

    // 月球 - 地球的卫星
    auto moon_texture = make_shared<image_texture>("solar system/moon_8k.jpg");
    auto moon_material = make_shared<lambertian>(moon_texture);
    world.add(make_shared<sphere>(point3(4.5, 0, -0.5), 0.27, moon_material));

    // 金星 - 有厚大气层效果
    auto venus_surface_texture = make_shared<image_texture>("solar system/venus_surface_8k.jpg");
    // 金星表面
    auto venus_surface = make_shared<sphere>(point3(6, 0, 2), 0.9, make_shared<lambertian>(venus_surface_texture));
    world.add(venus_surface);
    // 金星大气层（体积介质）
    auto venus_atmosphere = make_shared<sphere>(point3(6, 0, 2), 1.1, make_shared<dielectric>(1.0));
    world.add(venus_atmosphere);
    world.add(make_shared<constant_medium>(venus_atmosphere, 0.15, color(0.9, 0.8, 0.4), 1.1, point3(6, 0, 2)));

    // 水星 - 距离太阳最近，小而暗
    auto mercury_texture = make_shared<image_texture>("solar system/mercury_8k.jpg");
    auto mercury_material = make_shared<lambertian>(mercury_texture);
    world.add(make_shared<sphere>(point3(9, 0, -1), 0.4, mercury_material));

    // === 小行星带装饰（在火星和木星之间） ===
    auto asteroid_material = make_shared<lambertian>(color(0.4, 0.4, 0.4));
    for (int i = 0; i < 15; i++) {
        double angle = 2 * pi * i / 15.0;
        double radius = 2.0 + random_double(-0.8, 0.8);
        double x = -2 + radius * cos(angle) * 0.5;  // 椭圆分布
        double z = radius * sin(angle);
        double y = random_double(-1, 1);
        double size = random_double(0.03, 0.1);
        world.add(make_shared<sphere>(point3(x, y, z), size, asteroid_material));
    }

    // === 使用BVH优化场景 ===
    world = hittable_list(make_shared<bvh_node>(world));

    // === 相机设置：从侧面观察整个太阳系排列 ===
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 600;  // 更高分辨率展现细节
    cam.samples_per_pixel = 1000;   // 充足采样确保质量
    cam.max_depth         = 50;
    cam.background        = color(0, 0, 0);  // 太空黑暗背景

    // 相机位置：从侧上方观察整个行星排列，太阳在右侧
    cam.vfov     = 45;  // 适中视角，能看到完整行星序列
    cam.lookfrom = point3(-5, 8, -12);  // 从左前上方观察
    cam.lookat   = point3(0, 0, 0);     // 看向行星序列中心
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0;  // 清晰对焦

    // === 银河系环境光设置 ===
    cam.skybox_filename = "solar system/sky_8k.jpg";  // 银河系背景
    cam.sh_coeffs_filename = "galaxy_sh.txt";         // 球谐系数文件
    cam.use_spherical_harmonics = false;              // 暂时关闭，使用天空盒
    cam.enable_skybox = true;                          // 启用天空盒

    // === 多线程渲染优化 ===
    cam.enable_multithreading = true;
    cam.num_threads = 32;
    
    auto start = std::chrono::high_resolution_clock::now();
    cam.render(world, "solar_system_redesigned.ppm");
    auto render_time = std::chrono::high_resolution_clock::now() - start;
    
    std::clog << "渲染时间: " << std::chrono::duration_cast<std::chrono::seconds>(render_time).count() 
              << " 秒" << std::endl;
}

void final_scene(int image_width, int samples_per_pixel, int max_depth) {
    hittable_list boxes1;
    auto ground = make_shared<lambertian>(color(0.48, 0.83, 0.53));

    int boxes_per_side = 20;
    for (int i = 0; i < boxes_per_side; i++) {
        for (int j = 0; j < boxes_per_side; j++) {
            auto w = 100.0;
            auto x0 = -1000.0 + i*w;
            auto z0 = -1000.0 + j*w;
            auto y0 = 0.0;
            auto x1 = x0 + w;
            auto y1 = random_double(1,101);
            auto z1 = z0 + w;

            boxes1.add(box(point3(x0,y0,z0), point3(x1,y1,z1), ground));
        }
    }

    hittable_list world;

    world.add(make_shared<bvh_node>(boxes1));

    auto light = make_shared<diffuse_light>(color(7, 7, 7));
    world.add(make_shared<quad>(point3(123,554,147), vec3(300,0,0), vec3(0,0,265), light));

    auto center1 = point3(400, 400, 200);
    auto center2 = center1 + vec3(30,0,0);
    auto sphere_material = make_shared<lambertian>(color(0.7, 0.3, 0.1));
    world.add(make_shared<sphere>(center1, center2, 50, sphere_material));

    world.add(make_shared<sphere>(point3(260, 150, 45), 50, make_shared<dielectric>(1.5)));
    world.add(make_shared<sphere>(
        point3(0, 150, 145), 50, make_shared<metal>(color(0.8, 0.8, 0.9), 1.0)
    ));

    auto boundary = make_shared<sphere>(point3(360,150,145), 70, make_shared<dielectric>(1.5));
    world.add(boundary);
    world.add(make_shared<constant_medium>(boundary, 0.2, color(0.2, 0.4, 0.9)));
    boundary = make_shared<sphere>(point3(0,0,0), 5000, make_shared<dielectric>(1.5));
    world.add(make_shared<constant_medium>(boundary, .0001, color(1,1,1)));

    auto emat = make_shared<lambertian>(make_shared<image_texture>("solar system/earth_day_8k.jpg"));
    world.add(make_shared<sphere>(point3(400,200,400), 100, emat));
    auto pertext = make_shared<noise_texture>(0.2);
    world.add(make_shared<sphere>(point3(220,280,300), 80, make_shared<lambertian>(pertext)));

    hittable_list boxes2;
    auto white = make_shared<lambertian>(color(.73, .73, .73));
    int ns = 1000;
    for (int j = 0; j < ns; j++) {
        boxes2.add(make_shared<sphere>(point3::random(0,165), 10, white));
    }

    world.add(make_shared<translate>(
        make_shared<rotate_y>(
            make_shared<bvh_node>(boxes2), 15),
            vec3(-100,270,395)
        )
    );

    camera cam;

    cam.aspect_ratio      = 1.0;
    cam.image_width       = image_width;
    cam.samples_per_pixel = samples_per_pixel;
    cam.max_depth         = max_depth;
    cam.background        = color(0,0,0);

    cam.vfov     = 40;
    cam.lookfrom = point3(478, 278, -600);
    cam.lookat   = point3(278, 278, 0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0;

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;
    cam.enable_skybox = false; // 该场景不需要天空盒

    cam.enable_multithreading = true;
    // 设置线程数量
    // cam.num_threads = 32;               // 手动指定线程数
    // 或者使用默认值（自动检测CPU核心数）
    cam.num_threads = std::thread::hardware_concurrency();

    auto start = std::chrono::high_resolution_clock::now();
    cam.render(world);
    auto multi_time = std::chrono::high_resolution_clock::now() - start; // 记录多线程渲染时间
    std::cout << "多线程渲染时间: " << std::chrono::duration_cast<std::chrono::milliseconds>(multi_time).count() << " 毫秒" << std::endl;
}

void test_annulus() {
    hittable_list world;

    // === 创建简单的环形测试场景 ===
    
    // 背景：简单的渐变天空
    
    // 测试环形1：水平放置的大环形（类似土星环）
    auto ring_texture = make_shared<image_texture>("solar system/saturn_ring_8k.png");
    auto ring_material1 = make_shared<lambertian>(ring_texture); // 金黄色
    world.add(make_shared<annulus>(
        point3(0, 0, 0),        // 中心位置
        vec3(4.0, 0, 0),        // u方向向量（X轴方向）
        vec3(0, 0, 4.0),        // v方向向量（Z轴方向）
        1.5,                    // 内半径
        3.0,                    // 外半径
        ring_material1
    ));

    // 测试环形2：垂直放置的环形
    auto ring_material2 = make_shared<metal>(color(0.7, 0.3, 0.9), 0.1); // 紫色金属
    world.add(make_shared<annulus>(
        point3(6, 0, 0),        // 中心位置（右侧）
        vec3(0, 3.0, 0),        // u方向向量（Y轴方向）
        vec3(0, 0, 3.0),        // v方向向量（Z轴方向）
        0.8,                    // 内半径
        2.0,                    // 外半径
        ring_material2
    ));

    // 测试环形3：倾斜的环形
    auto ring_material3 = make_shared<dielectric>(1.5); // 玻璃材质
    world.add(make_shared<annulus>(
        point3(-4, 1, 2),       // 中心位置（左侧稍高）
        vec3(2.5, 1.0, 0),      // u方向向量（倾斜）
        vec3(0, 1.0, 2.5),      // v方向向量（倾斜）
        0.5,                    // 内半径
        1.8,                    // 外半径
        ring_material3
    ));

    // 添加一些参考球体来对比环形效果
    auto sphere_material = make_shared<lambertian>(color(0.4, 0.7, 0.4));
    world.add(make_shared<sphere>(point3(0, 0, 0), 0.8, sphere_material)); // 中心球体
    world.add(make_shared<sphere>(point3(6, 0, 0), 0.5, sphere_material)); // 右侧参考球
    world.add(make_shared<sphere>(point3(-4, 1, 2), 0.3, sphere_material)); // 左侧参考球

    // 添加地面
    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    // 添加光源
    auto light_material = make_shared<diffuse_light>(color(4, 4, 4));
    world.add(make_shared<sphere>(point3(-2, 8, -3), 1.5, light_material));

    // === 相机设置 ===
    camera cam;

    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 800;
    cam.samples_per_pixel = 200;
    cam.max_depth         = 50;
    cam.background        = color(0.1, 0.1, 0.2); // 深蓝色背景

    // 相机位置：从斜上方观察所有环形
    cam.vfov     = 50;
    cam.lookfrom = point3(2, 6, -8);  // 观察位置
    cam.lookat   = point3(0, 0, 0);   // 看向中心
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0;

    // 禁用天空盒和球谐函数
    cam.use_spherical_harmonics = false;
    cam.enable_skybox = false;

    // 启用多线程
    cam.enable_multithreading = true;
    cam.num_threads = 16;

    std::clog << "=== 开始渲染环形测试场景 ===" << std::endl;
    std::clog << "场景包含：" << std::endl;
    std::clog << "- 水平金黄色环形（模拟土星环）" << std::endl;
    std::clog << "- 垂直紫色金属环形" << std::endl;
    std::clog << "- 倾斜玻璃环形" << std::endl;
    std::clog << "- 参考球体和光源" << std::endl;

    cam.render(world, "annulus_test.ppm");
    
    std::clog << "=== 环形测试场景渲染完成 ===" << std::endl;
    std::clog << "输出文件：annulus_test.ppm" << std::endl;
}

void test_model() {
    hittable_list world;

    // 地面
    auto ground = make_shared<lambertian>(color(0.5, 0.5, 0.5));

    // 模型材质
    auto matte = make_shared<lambertian>(color(0.7, 0.7, 0.7));
    ModelLoadOptions opt;
    opt.scale = vec3(1.0, 1.0, 1.0);
    opt.translate = vec3(0, 0, 0);
    opt.center_model = true; // 模型居中
    opt.flip_winding = false; // 不翻转法线

    // 加载模型
    auto model = load_model_as_hittable("models/bunny.obj", matte, opt);
    world.add(model);

    // 光源（用发光球）
    auto light = make_shared<diffuse_light>(color(6,6,6));
    world.add(make_shared<sphere>(point3(0, 5, 4), 1.2, light));

    // 相机
    camera cam;
    cam.aspect_ratio      = 16.0 / 9.0;
    cam.image_width       = 800;
    cam.samples_per_pixel = 500;
    cam.max_depth         = 200;
    cam.background        = color(0.05, 0.05, 0.1);

    cam.vfov     = 35;
    cam.lookfrom = point3(1.5, 1.0, -3);
    cam.lookat   = point3(0, 0.5, 0);
    cam.vup      = vec3(0,1,0);
    cam.defocus_angle = 0;

    cam.skybox_filename = "skybox.ppm";
    cam.use_spherical_harmonics = false;
    cam.enable_skybox = true;

    cam.enable_multithreading = true;
    cam.num_threads = 28;

    cam.render(world, "model_test.ppm");
}

void mix_scene() {
    SphericalHarmonics sh_lighting(3);
    sh_lighting.generateFromEnvironmentMap("skybox.ppm");
    sh_lighting.saveCoefficients("skybox_sh.txt");

    hittable_list world;

    // 添加地面 - 大球体作为地面
    // 噪声材质地面
    auto pertext = make_shared<noise_texture>(4);
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(pertext)));

    // 随机生成小物体（球体和八面体混合）
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double(); // 选择材质
            auto choose_shape = random_double(); // 选择形状
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> object_material;

                if (choose_mat < 0.8) {
                    // 漫反射材质
                    auto albedo = color::random() * color::random();
                    object_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, object_material));
                } else if (choose_mat < 0.95) {
                    // 金属材质
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    object_material = make_shared<metal>(albedo, fuzz);
                } else {
                    // 玻璃材质
                    object_material = make_shared<dielectric>(1.5);
                }

                // 根据随机值选择形状：50%球体，50%八面体
                if (choose_shape < 0.5) {
                    world.add(make_shared<sphere>(center, 0.2, object_material));
                } else {
                    world.add(make_shared<octahedron>(center, 0.4, object_material)); // 八面体稍大一些
                }
            }
        }
    }

    // 主要展示物体 - 三个大物体
    
    // 中央：玻璃球体
    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    // 左侧：漫反射八面体
    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<octahedron>(point3(-4, 1, 0), 2.0, material2));

    // 模型材质
    auto matte = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    ModelLoadOptions opt;
    opt.scale = vec3(2.0, 2.0, 2.0);
    opt.translate = vec3(4, 0, 0);
    opt.center_model = false; // 不居中
    opt.flip_winding = false; // 不翻转法线

    // 加载模型
    auto model = load_model_as_hittable("models/bunny.obj", matte, opt);
    world.add(model);

    // 额外添加一些有趣的八面体展示不同材质效果
    
    // 前方左：彩色玻璃八面体
    auto glass_material = make_shared<dielectric>(1.8);
    world.add(make_shared<octahedron>(point3(-2, 0.5, 2), 1.0, glass_material));

    // 前方右：有模糊的金属八面体
    auto fuzzy_metal = make_shared<metal>(color(0.8, 0.3, 0.3), 0.3);
    world.add(make_shared<octahedron>(point3(2, 0.5, 2), 1.0, fuzzy_metal));

    // 后方：玻璃八面体
    auto green_material = make_shared<dielectric>(1.5);
    world.add(make_shared<octahedron>(point3(0, 0.8, -3), 1.6, green_material));

    world = hittable_list(make_shared<bvh_node>(world)); // 使用BVH加速场景
    // 将所有的物体包裹在一个BVH节点中
    // world中的所有节点在此时已经不再是添加时的sphere或octahedron对象，而是BVH树的节点

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width  = 720;
    cam.samples_per_pixel = 1; // 每个像素的随机样本数，这里的随机采样不只可以实现抗锯齿，还可以实现漫反射材质表面的真实样子
    // 如果随机样本数足够多，最终渲染出来的图像会更加平滑和真实
    // 反之如果该样本数为1，渲染出来的图像会出现明显的锯齿和噪点
    cam.max_depth = 10;  // 最大反弹次数

    // 降噪参数
    cam.enable_stratified_sampling = true;  // 是否启用分层采样
    cam.enable_progressive_rendering = true; // 是否启用渐进式渲染(帧间累积)
    cam.samples_per_frame = 1; 
    cam.temporal_blend_factor = 0.9;
    cam.temporal_accumulation_limit = 128;
    cam.enable_temporal_denoising = true;
    cam.enable_spatial_denoising = true;
    cam.enable_back_projection = true;
    cam.temporal_blend_factor = 0.85;
    cam.debug_gbuffer_mode = 0;
    cam.spatial_sigma_color = 0.5;       // 颜色相似度阈值
    cam.spatial_sigma_normal = 0.5;      // 法线相似度阈值(弧度)
    cam.spatial_sigma_depth = 0.1; 
    cam.enable_russian_roulette = true; // 启用俄罗斯轮盘赌
    cam.enable_outlier_removal = true; // 启用异常值移除
    cam.outlier_threshold = 0.5;

    // 单帧模式
    cam.single_frame_mode = false;
    cam.output_filename = "single_frame_3.ppm";

    // 相机参数
    cam.vfov     = 20;
    cam.enable_motion = true;
    cam.lookfrom = point3(13, 2, 3);
    cam.center_orbit = point3(0, 2, 0); // 相机围绕中心点旋转
    cam.radius_orbit = 13; // 相机围绕中心点的半径
    cam.angular_velocity = 0.01; // 相机围绕中心点的旋转速度
    cam.lookat   = point3(0,0,0);
    cam.vup      = vec3(0,1,0);

    cam.defocus_angle = 0; // 景深效果
    cam.focus_dist    = 10.0; // 聚焦距离

    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = false;

    // 设置天空盒文件名
    cam.skybox_filename = "skybox.ppm";
    cam.enable_multithreading = true;
    cam.num_threads = 28;

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(world);
}

int main() {
    switch (13) {
        case 1: bouncing_spheres();  break;
        case 2: checkered_spheres(); break;
        case 3: earth();             break;
        case 4: perlin_spheres();    break;
        case 5: quads();             break;
        case 6: simple_light();      break;
        case 7: cornell_box();       break;
        case 8: cornell_smoke();     break;
        case 9: final_scene(800, 5000, 40); break;
        case 10: solar_system();     break;
        case 11: test_annulus();     break;
        case 12: test_model();       break;
        case 13: mix_scene();        break;
        default: final_scene(400,   250,  4); break;
    }
}
