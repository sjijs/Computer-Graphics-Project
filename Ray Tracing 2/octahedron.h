#ifndef OCTAHEDRON_H
#define OCTAHEDRON_H

#include "hittable.h"
#include "vec3.h"
#include <cmath>

class octahedron : public hittable {
  public:
    // 静态八面体
    octahedron(const point3& center, double size, shared_ptr<material> mat)
      : center(center, vec3(0,0,0)), radius(size/2.0), mat(mat) {}

    // 运动八面体
    octahedron(const point3& center1, const point3& center2, double radius, shared_ptr<material> mat)
      : center(center1, center2 - center1), radius(radius), mat(mat) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        point3 current_center = center.at(r.time());
        
        // 将射线转换到以八面体中心为原点的坐标系
        vec3 ro = r.origin() - current_center;
        vec3 rd = r.direction();
        
        double t_min = ray_t.max + 1;
        bool found_hit = false;
        vec3 hit_normal;
        
        // 八面体有8个面，每个面的法向量是 (±1, ±1, ±1) / sqrt(3)
        // 对应的平面方程是 ±x ± y ± z = radius / sqrt(3)
        
        for (int i = 0; i < 8; i++) {
            // 8种符号组合
            int sx = (i & 1) ? 1 : -1;
            int sy = (i & 2) ? 1 : -1; 
            int sz = (i & 4) ? 1 : -1;
            
            vec3 normal(sx, sy, sz);
            normal = unit_vector(normal);
            
            // 平面方程: normal · (point - center) = radius / sqrt(3)
            double plane_dist = radius / std::sqrt(3.0);
            
            // 射线与平面的交点
            double denom = dot(rd, normal);
            if (std::abs(denom) < 1e-10) continue; // 平行
            
            double t = (plane_dist - dot(ro, normal)) / denom;
            
            if (!ray_t.contains(t) || t >= t_min) continue;
            
            // 计算交点
            vec3 hit_point = ro + t * rd;
            
            // 检查交点是否在八面体表面上
            double surface_dist = std::abs(hit_point.x()) + std::abs(hit_point.y()) + std::abs(hit_point.z());
            
            if (std::abs(surface_dist - radius) > 1e-6) continue;
            
            // 检查交点是否在正确的面上（符号检查）
            bool on_correct_face = true;
            if (sx > 0 && hit_point.x() < -1e-8) on_correct_face = false;
            if (sx < 0 && hit_point.x() > 1e-8) on_correct_face = false;
            if (sy > 0 && hit_point.y() < -1e-8) on_correct_face = false;
            if (sy < 0 && hit_point.y() > 1e-8) on_correct_face = false;
            if (sz > 0 && hit_point.z() < -1e-8) on_correct_face = false;
            if (sz < 0 && hit_point.z() > 1e-8) on_correct_face = false;
            
            if (on_correct_face) {
                t_min = t;
                found_hit = true;
                hit_normal = normal;
            }
        }
        
        if (found_hit) {
            rec.t = t_min;
            rec.p = r.at(t_min);
            rec.set_face_normal(r, hit_normal);
            rec.mat = mat;
            return true;
        }
        
        return false;
    }

  private:
    ray center;
    double radius;
    shared_ptr<material> mat;
};

#endif