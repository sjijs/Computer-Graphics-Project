#include "rtweekend.h"
#include "spherical_harmonics.h"
#include "camera_sh.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

int main() {
    std::cout << "=== 球谐函数环境光照完整演示 ===" << std::endl;
    
    // 第一步：从环境贴图生成球谐系数
    std::cout << "\n1. 从环境贴图生成球谐函数系数..." << std::endl;
    SphericalHarmonics sh_generator(3);  // 使用3阶球谐函数（9个系数）
    
    if (!sh_generator.generateFromEnvironmentMap("skybox.ppm")) {
        std::cerr << "错误：无法生成球谐系数！" << std::endl;
        return 1;
    }
    
    // 保存系数到文件
    sh_generator.saveCoefficients("skybox_sh.txt");
    
    // 第二步：测试球谐函数的重建质量
    std::cout << "\n2. 验证球谐函数重建质量..." << std::endl;
    
    // 测试一些关键方向
    struct TestDirection {
        vec3 dir;
        std::string name;
    };
    
    std::vector<TestDirection> test_dirs = {
        {vec3(0, 1, 0), "天顶(+Y)"},
        {vec3(0, -1, 0), "地面(-Y)"},
        {vec3(1, 0, 0), "东方(+X)"},
        {vec3(-1, 0, 0), "西方(-X)"},
        {vec3(0, 0, 1), "南方(+Z)"},
        {vec3(0, 0, -1), "北方(-Z)"},
        {vec3(0.707, 0.707, 0), "东北上方"},
        {vec3(-0.707, -0.707, 0), "西南下方"}
    };
    
    std::cout << "球谐函数重建结果：" << std::endl;
    for (const auto& test : test_dirs) {
        color sh_color = sh_generator.evaluate(test.dir);
        std::cout << "  " << test.name << ": RGB(" 
                  << int(sh_color.x() * 255) << ", " 
                  << int(sh_color.y() * 255) << ", " 
                  << int(sh_color.z() * 255) << ")" << std::endl;
    }
    
    // 第三步：使用球谐函数渲染场景
    std::cout << "\n3. 使用球谐函数渲染测试场景..." << std::endl;
    
    hittable_list world;
    
    // 创建多个球体展示不同材质在球谐光照下的效果
    auto red_lambertian = make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto green_lambertian = make_shared<lambertian>(color(0.3, 0.7, 0.3));
    auto blue_lambertian = make_shared<lambertian>(color(0.3, 0.3, 0.7));
    auto metal_material = make_shared<metal>(color(0.8, 0.8, 0.9), 0.0);
    auto glass_material = make_shared<dielectric>(1.5);
    
    // 前排三个球
    world.add(make_shared<sphere>(point3(-2, 0, -1), 0.5, red_lambertian));
    world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, glass_material));
    world.add(make_shared<sphere>(point3(2, 0, -1), 0.5, blue_lambertian));
    
    // 后排两个球
    world.add(make_shared<sphere>(point3(-1, 0, -2), 0.5, metal_material));
    world.add(make_shared<sphere>(point3(1, 0, -2), 0.5, green_lambertian));
    
    // 地面
    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -100.5, -1), 100, ground_material));
    
    // 设置相机
    camera_sh cam;
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 600;
    cam.samples_per_pixel = 100;
    cam.max_depth = 50;
    
    cam.vfov = 20;
    cam.lookfrom = point3(-2, 2, 1);
    cam.lookat = point3(0, 0, -1);
    cam.vup = vec3(0, 1, 0);
    
    cam.defocus_angle = 0.0;
    cam.focus_dist = 3.0;
    
    // 渲染使用球谐函数的版本
    cam.output_filename = "sh_lighting_demo.ppm";
    cam.sh_coeffs_filename = "skybox_sh.txt";
    cam.use_spherical_harmonics = true;
    
    std::cout << "正在渲染球谐光照版本..." << std::endl;
    cam.render(world);
    
    // 第四步：对比渲染（可选）
    std::cout << "\n4. 生成对比版本..." << std::endl;
    
    // 禁用球谐函数，使用默认渐变背景
    cam.output_filename = "default_lighting_demo.ppm";
    cam.use_spherical_harmonics = false;
    
    std::cout << "正在渲染默认光照版本..." << std::endl;
    cam.render(world);
    
    // 总结
    std::cout << "\n=== 球谐函数演示完成 ===" << std::endl;
    std::cout << "\n生成的文件：" << std::endl;
    std::cout << "1. skybox_sh.txt - 球谐函数系数文件" << std::endl;
    std::cout << "2. sh_lighting_demo.ppm - 球谐光照渲染结果" << std::endl;
    std::cout << "3. default_lighting_demo.ppm - 默认光照对比结果" << std::endl;
    
    std::cout << "\n技术说明：" << std::endl;
    std::cout << "- 使用3阶球谐函数（9个基函数）" << std::endl;
    std::cout << "- 从1524x1024环境贴图投影生成系数" << std::endl;
    std::cout << "- 实时评估球谐函数获得方向性光照" << std::endl;
    std::cout << "- 相比传统纹理采样，计算更高效且平滑" << std::endl;
    
    return 0;
}
