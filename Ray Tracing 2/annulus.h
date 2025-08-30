#ifndef ANNULUS_H
#define ANNULUS_H

#include "hittable.h"
#include "quad.h"

// class annulus : public quad {
//   public:
//     annulus(
//         const point3& center, const vec3& side_A, const vec3& side_B, double _inner,
//         shared_ptr<material> m)
//       : quad(center, side_A, side_B, m), inner(_inner)
//     {}

//     virtual void set_bounding_box() override {
//         bbox = aabb(plane_origin - axis_A - axis_B, plane_origin + axis_A + axis_B).pad();
//     }

//     virtual bool hit_ab(double a, double b, hit_record& rec) const override {
//         auto center_dist = sqrt(a*a + b*b);
//         if ((center_dist < inner) || (center_dist > 1))
//             return false;

//         rec.u = a/2 + 0.5;
//         rec.v = b/2 + 0.5;
//         return true;
//     }

//   private:
//     double inner;
// };

#endif