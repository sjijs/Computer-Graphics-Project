#include "rtweekend.h"

#include <fstream>
#include <cmath>
#include <limits>

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"
#include "octahedron.h"

int main() {
    hittable_list world;

    world.add(make_shared<sphere>(point3(-0.5,0,-1), 0.5));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));
    world.add(make_shared<octahedron>(point3(0.5, 0, -1), 0.5));

    camera cam;

    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width  = 400;
    cam.samples_per_pixel = 200; // 每个像素的随机样本数，这里的随机采样不只可以实现抗锯齿，还可以实现漫反射材质表面的真是样子
    // 如果随机样本数足够多，最终渲染出来的图像会更加平滑和真实
    // 反之如果该样本数为1，渲染出来的图像会出现明显的锯齿和噪点
    cam.max_depth = 50;  // 最大反弹次数

    cam.render(world);
}
