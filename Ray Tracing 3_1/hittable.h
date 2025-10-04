#ifndef HITTABLE_H
#define HITTABLE_H

#include "rtweekend.h"
#include "aabb.h"

class material;// 防止循环引用的前向声明

// hit_record 用于存储光线与物体交点的信息
// 包括交点位置、法向量和交点的 t 值
class hit_record {
  public:
    point3 p;
    vec3 normal;
    shared_ptr<material> mat;  // 材质指针，在初始化物体时给予相对应材质属性，当 ray_color()函数获得hit_record时
                               // 它可以调用材质指针的成员函数来找出散射的光线（如果有）。
    double t;
    double u;
    double v;
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

    virtual aabb bounding_box() const = 0;
};

// translate 类用于对一个 hittable 对象进行平移变换
// 其实也可以看作是一个变换矩阵，如同渲染管线中的变换过程，只不过一般渲染管线的操作对象是三角形网格（或者说顶点）
// 1. 将光线从世界空间更改为对象空间
// 2. 确定对象空间中是否存在交集（如果存在，则在何处）
// 3. 将交点从对象空间更改为世界空间
class translate : public hittable {
  public:
    translate(shared_ptr<hittable> object, const vec3& offset)
      : object(object), offset(offset)
    {
        bbox = object->bounding_box() + offset;
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Move the ray backwards by the offset
        ray offset_r(r.origin() - offset, r.direction(), r.time());

        // Determine whether an intersection exists along the offset ray (and if so, where)
        if (!object->hit(offset_r, ray_t, rec))
            return false;

        // Move the intersection point forwards by the offset
        rec.p += offset;

        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    vec3 offset; // 位移量
    aabb bbox;
};

class rotate_y : public hittable {
  public:
    rotate_y(shared_ptr<hittable> object, double angle) : object(object) {
        auto radians = degrees_to_radians(angle);
        sin_theta = std::sin(radians);
        cos_theta = std::cos(radians);
        bbox = object->bounding_box();

        point3 min( infinity,  infinity,  infinity);
        point3 max(-infinity, -infinity, -infinity);

        // Find the new bounding box corners after rotation
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                for (int k = 0; k < 2; k++) {
                    auto x = i*bbox.x.max + (1-i)*bbox.x.min;
                    auto y = j*bbox.y.max + (1-j)*bbox.y.min;
                    auto z = k*bbox.z.max + (1-k)*bbox.z.min;

                    auto newx =  cos_theta*x + sin_theta*z;
                    auto newz = -sin_theta*x + cos_theta*z;

                    vec3 tester(newx, y, newz);

                    for (int c = 0; c < 3; c++) {
                        min[c] = std::fmin(min[c], tester[c]);
                        max[c] = std::fmax(max[c], tester[c]);
                    }
                }
            }
        }

        bbox = aabb(min, max);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {

        // Transform the ray from world space to object space.

        auto origin = point3(
            (cos_theta * r.origin().x()) - (sin_theta * r.origin().z()),
            r.origin().y(),
            (sin_theta * r.origin().x()) + (cos_theta * r.origin().z())
        );

        auto direction = vec3(
            (cos_theta * r.direction().x()) - (sin_theta * r.direction().z()),
            r.direction().y(),
            (sin_theta * r.direction().x()) + (cos_theta * r.direction().z())
        );

        ray rotated_r(origin, direction, r.time());

        // Determine whether an intersection exists in object space (and if so, where).

        if (!object->hit(rotated_r, ray_t, rec))
            return false;

        // Transform the intersection from object space back to world space.

        rec.p = point3(
            (cos_theta * rec.p.x()) + (sin_theta * rec.p.z()),
            rec.p.y(),
            (-sin_theta * rec.p.x()) + (cos_theta * rec.p.z())
        );

        rec.normal = vec3(
            (cos_theta * rec.normal.x()) + (sin_theta * rec.normal.z()),
            rec.normal.y(),
            (-sin_theta * rec.normal.x()) + (cos_theta * rec.normal.z())
        );

        return true;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> object;
    double sin_theta;
    double cos_theta;
    aabb bbox;
};

#endif