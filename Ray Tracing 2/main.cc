#include "rtweekend.h"

#include <fstream>
#include <cmath>
#include <limits>

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"
#include "octahedron.h"
#include "bvh.h"
#include "texture.h"

int main() {
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

    cam.render(world);
}
