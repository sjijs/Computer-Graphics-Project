#include "rtweekend.h"

#include <fstream>
#include <cmath>
#include <limits>

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
#include "bvh.h"
#include "texture.h"

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
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, make_shared<lambertian>(checker)));

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
    cam.image_width  = 400;
    cam.samples_per_pixel = 100; // 每个像素的随机样本数，这里的随机采样不只可以实现抗锯齿，还可以实现漫反射材质表面的真实样子
    // 如果随机样本数足够多，最终渲染出来的图像会更加平滑和真实
    // 反之如果该样本数为1，渲染出来的图像会出现明显的锯齿和噪点
    cam.max_depth = 50;  // 最大反弹次数

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

    cam.background = color(0.70, 0.80, 1.00);

    cam.render(world);
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

    auto emat = make_shared<lambertian>(make_shared<image_texture>("earthmap.jpg"));
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

    cam.render(world);
}

int main() {
    switch (8) {
        case 1: bouncing_spheres();  break;
        case 2: checkered_spheres(); break;
        case 3: earth();             break;
        case 4: perlin_spheres();    break;
        case 5: quads();             break;
        case 6: simple_light();      break;
        case 7: cornell_box();       break;
        case 8: cornell_smoke();     break;
        case 9:  final_scene(800, 10000, 40); break;
        default: final_scene(400,   250,  4); break;
    }
}
