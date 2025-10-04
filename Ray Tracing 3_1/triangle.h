#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"
#include "quad.h"

class triangle : public quad {
public:
    triangle(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat)
      : quad(Q, u, v, mat) {} // 显式调用父类构造函数（这都忘了，子类必须调用父类构造函数）
    
    void set_bounding_box() override {
        // 三角形只需要考虑三个顶点：Q, Q+u, Q+v
        auto bbox_diagonal1 = aabb(Q, Q + u);
        auto bbox_diagonal2 = aabb(Q, Q + v);
        bbox = aabb(bbox_diagonal1, bbox_diagonal2);
    }

    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);

        if (!(a > 0 && b > 0 && a + b < 1))
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }
};

#endif