#ifndef CYCLE_H
#define CYCLE_H

#include "hittable.h"
#include "quad.h"

class cycle : public quad {
public:
    cycle(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat)
      : quad(Q, u, v, mat) {
        auto r = Q + u + Q + v;
        radius = r.length();
      } // 显式调用父类构造函数（这都忘了，子类必须调用父类构造函数）

    bool is_interior(double a, double b, hit_record& rec) const override {
        interval unit_interval = interval(0, 1);

        if (!(sqrt(a*a + b*b) < radius))
            return false;

        rec.u = a;
        rec.v = b;
        return true;
    }

private:
    double radius;
};

#endif