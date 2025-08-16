#ifndef OCTAHEDRON_H
#define OCTAHEDRON_H

#include "hittable.h"
#include "vec3.h"
#include <cmath>

class octahedron : public hittable {
  public:
    octahedron(const point3& center, double size, shared_ptr<material> mat)
      : center(center), radius(size/2.0), mat(mat) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // 使用八面体的隐式方程：|x| + |y| + |z| = radius
        // 转换为射线参数方程求解
        
        vec3 oc = r.origin() - center;
        vec3 d = r.direction();
        
        // 我们需要求解 |oc.x + t*d.x| + |oc.y + t*d.y| + |oc.z + t*d.z| = radius
        // 这需要分情况讨论，根据射线方向和起点位置的符号
        
        double best_t = -1.0;
        vec3 best_normal;
        
        // 八面体有8个面，每个面对应不同的符号组合
        for (int sx = -1; sx <= 1; sx += 2) {
            for (int sy = -1; sy <= 1; sy += 2) {
                for (int sz = -1; sz <= 1; sz += 2) {
                    // 对于当前符号组合，线性方程为：
                    // sx*(oc.x + t*d.x) + sy*(oc.y + t*d.y) + sz*(oc.z + t*d.z) = radius
                    
                    double a = sx * d.x() + sy * d.y() + sz * d.z();
                    double b = sx * oc.x() + sy * oc.y() + sz * oc.z() - radius;
                    
                    if (std::abs(a) > 1e-8) {
                        double t = -b / a;
                        
                        if (ray_t.contains(t) && (best_t < 0 || t < best_t)) {
                            // 检查这个t值对应的点是否确实在对应的面上
                            point3 hit_point = r.at(t);
                            vec3 local_point = hit_point - center;
                            
                            // 检查符号是否匹配
                            bool valid = true;
                            if (sx > 0 && local_point.x() < -1e-6) valid = false;
                            if (sx < 0 && local_point.x() > 1e-6) valid = false;
                            if (sy > 0 && local_point.y() < -1e-6) valid = false;
                            if (sy < 0 && local_point.y() > 1e-6) valid = false;
                            if (sz > 0 && local_point.z() < -1e-6) valid = false;
                            if (sz < 0 && local_point.z() > 1e-6) valid = false;
                            
                            // 检查是否真的在八面体表面上
                            double surface_dist = std::abs(local_point.x()) + 
                                                 std::abs(local_point.y()) + 
                                                 std::abs(local_point.z());
                            
                            if (valid && std::abs(surface_dist - radius) < 1e-6) {
                                best_t = t;
                                // 计算该面的法向量
                                best_normal = vec3(sx, sy, sz);
                                best_normal = unit_vector(best_normal);
                            }
                        }
                    }
                }
            }
        }
        
        if (best_t > 0) {
            rec.t = best_t;
            rec.p = r.at(rec.t);
            rec.set_face_normal(r, best_normal);
            rec.mat = mat;
            return true;
        }
        
        return false;
    }

  private:
    point3 center;
    double radius;
    shared_ptr<material> mat;
};

#endif