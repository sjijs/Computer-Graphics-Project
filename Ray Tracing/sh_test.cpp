#include "rtweekend.h"
#include "spherical_harmonics.h"
#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

int main() {
    std::cout << "=== 球谐函数环境光照测试 ===" << std::endl;
    
    // 创建球谐函数对象（使用3阶，共9个系数）
    SphericalHarmonics sh_lighting(3);
    
    std::cout << "1. 从环境贴图生成球谐系数..." << std::endl;
    if (!sh_lighting.generateFromEnvironmentMap("skybox.ppm")) {
        std::cerr << "生成球谐系数失败！" << std::endl;
        return 1;
    }
    
    std::cout << "2. 保存球谐系数..." << std::endl;
    sh_lighting.saveCoefficients("skybox_sh.txt");
    
    std::cout << "3. 测试球谐函数评估..." << std::endl;
    
    // 测试几个方向的光照
    std::vector<vec3> test_directions = {
        vec3(0, 1, 0),      // 上方
        vec3(0, -1, 0),     // 下方
        vec3(1, 0, 0),      // 右方
        vec3(-1, 0, 0),     // 左方
        vec3(0, 0, 1),      // 前方
        vec3(0, 0, -1),     // 后方
        vec3(1, 1, 1),      // 对角
        vec3(-1, -1, -1)    // 对角
    };
    
    std::cout << "方向测试结果:" << std::endl;
    for (size_t i = 0; i < test_directions.size(); ++i) {
        vec3 dir = unit_vector(test_directions[i]);
        color lighting = sh_lighting.evaluate(dir);
        std::cout << "  方向(" << dir.x() << ", " << dir.y() << ", " << dir.z() 
                  << ") -> RGB(" << lighting.x() << ", " << lighting.y() << ", " << lighting.z() << ")" << std::endl;
    }
    
    std::cout << "\n4. 创建基于球谐函数的渲染场景..." << std::endl;
    
    // 创建简单场景
    hittable_list world;
    
    // 添加几个不同材质的球体来展示球谐光照效果
    auto lambertian_material = make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto metal_material = make_shared<metal>(color(0.8, 0.8, 0.9), 0.0);
    auto glass_material = make_shared<dielectric>(1.5);
    
    world.add(make_shared<sphere>(point3(-1, 0, -1), 0.5, lambertian_material));
    world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, glass_material));
    world.add(make_shared<sphere>(point3(1, 0, -1), 0.5, metal_material));
    
    // 地面
    auto ground_material = make_shared<lambertian>(color(0.8, 0.8, 0.0));
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, ground_material));
    
    // 创建相机
    camera cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 400;
    cam.samples_per_pixel = 20;
    cam.max_depth = 50;
    
    cam.vfov = 20;
    cam.lookfrom = point3(-2, 2, 1);
    cam.lookat = point3(0, 0, -1);
    cam.vup = vec3(0, 1, 0);
    
    cam.defocus_angle = 0.0;
    cam.focus_dist = 3.0;
    
    // 设置输出文件名
    cam.output_filename = "sh_lighting_test.ppm";
    
    // 为了演示，我们先使用原始的天空盒渲染
    cam.skybox_filename = "skybox.ppm";
    
    std::cout << "5. 渲染参考图像..." << std::endl;
    cam.render(world);
    
    std::cout << "\n=== 球谐函数测试完成 ===" << std::endl;
    std::cout << "生成的文件:" << std::endl;
    std::cout << "- skybox_sh.txt: 球谐系数文件" << std::endl;
    std::cout << "- sh_lighting_test.ppm: 渲染测试图像" << std::endl;
    
    return 0;
}
