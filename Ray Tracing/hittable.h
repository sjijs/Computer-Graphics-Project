#ifndef HITTABLE_H
#define HITTABLE_H

#include "rtweekend.h"

// hit_record 用于存储光线与物体交点的信息
// 包括交点位置、法向量和交点的 t 值
class hit_record {
  public:
    point3 p;
    vec3 normal;
    double t;
    bool front_face;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // 设置交点的法向量
        // front_face 用于判断光线是否从物体的外侧射入
        // 如果光线方向与法向量的点积小于0，则表示光线从物体外侧射入
        // 否则表示光线从物体内侧射入

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// hittable 是一个抽象基类，定义了所有可被光线检测的物体的接口
// 任何继承自 hittable 的类都需要实现 hit 方法
class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;// const表示该方法不会修改类的成员变量
};

#endif