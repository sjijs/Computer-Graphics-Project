#include "rtweekend.h"

#include <fstream>
#include <cmath>
#include <limits>

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

int main() {
    hittable_list world;

    // 创建一个简单的场景，只有一个小球体，大部分视野显示天空盒
    auto material1 = make_shared<metal>(color(0.8, 0.8, 0.9), 0.0);
    world.add(make_shared<sphere>(point3(0, 0, -1), 0.5, material1));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width  = 1200;  // 增加分辨率以便更好观察
    cam.samples_per_pixel = 50;  // 减少采样数以加快渲染
    cam.max_depth = 50;

    cam.vfov     = 90;  // 更广的视角以显示更多天空盒
    cam.lookfrom = point3(0, 0, 0);  // 简单的相机位置
    cam.lookat   = point3(0, 0, -1);
    cam.vup      = vec3(0, 1, 0);

    cam.defocus_angle = 0.0;  // 关闭散焦
    cam.focus_dist    = 1.0;
    
    // 设置天空盒文件名和输出文件名
    cam.skybox_filename = "skybox.ppm";
    cam.output_filename = "skybox_test.ppm";

    cam.render(world);
}
