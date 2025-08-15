#ifndef OCTAHEDRON_H
#define OCTAHEDRON_H

#include "hittable.h"
#include "vec3.h"
#include <cmath>
#include <limits>

class octahedron : public hittable {
  public:
    octahedron(const point3& center, double size, shared_ptr<material> mat)
      : center(center), size(std::fmax(0,size)), mat(mat) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // 计算 a_i = C_i - Q_i (八面体中心 - 射线起点)
        vec3 a = center - r.origin();
        vec3 d = r.direction();
        
        // 存储零点候选值
        double candidates[6];
        int candidate_count = 0;
        
        // 计算三个分量的零点 t_i = a_i / d_i (当 d_i != 0 时)
        for (int i = 0; i < 3; i++) {
            if (std::abs(d[i]) > 1e-8) { // 避免除零
                candidates[candidate_count++] = a[i] / d[i];
            }
        }
        
        // 添加两个远端点进行比较
        candidates[candidate_count++] = -1000.0; // 负无穷的近似
        candidates[candidate_count++] = 1000.0;  // 正无穷的近似
        
        // 计算 F = size/2 (八面体的半边长)
        double F = size / 2.0;
        
        // 寻找 g(t) 的最小值
        double g_min = std::numeric_limits<double>::max();
        
        for (int i = 0; i < candidate_count; i++) {
            double t = candidates[i];
            
            // 计算 g(t) = |a_x - t*d_x| + |a_y - t*d_y| + |a_z - t*d_z|
            double g_t = std::abs(a.x() - t * d.x()) + 
                         std::abs(a.y() - t * d.y()) + 
                         std::abs(a.z() - t * d.z());
            
            if (g_t < g_min) {
                g_min = g_t;
            }
        }
        
        // 判断是否有解：F >= g_min
        if (F >= g_min) {
            // 找到使 g(t) = F 的 t 值
            double best_t = -1.0;
            
            // 先检查候选点
            for (int i = 0; i < candidate_count; i++) {
                double t = candidates[i];
                if (!ray_t.surrounds(t)) continue; // 检查 t 值范围
                
                // 计算该点的 g(t) 值
                double g_t = std::abs(a.x() - t * d.x()) + 
                             std::abs(a.y() - t * d.y()) + 
                             std::abs(a.z() - t * d.z());
                
                // 如果这个点在八面体表面上或内部
                if (g_t <= F + 1e-6) {
                    if (best_t < 0 || t < best_t) {
                        best_t = t;
                    }
                }
            }
            
            // 如果没有找到合适的候选点，进行二分搜索
            if (best_t < 0) {
                double left = ray_t.left(), right = ray_t.right();
                for (int iter = 0; iter < 50; iter++) {
                    double mid = (left + right) / 2.0;
                    double g_mid = std::abs(a.x() - mid * d.x()) + 
                                   std::abs(a.y() - mid * d.y()) + 
                                   std::abs(a.z() - mid * d.z());
                    
                    if (std::abs(g_mid - F) < 1e-6) {
                        best_t = mid;
                        break;
                    }
                    
                    if (g_mid > F) {
                        right = mid;
                    } else {
                        left = mid;
                    }
                }
                if (best_t < 0 && left >= ray_t.left() && right <= ray_t.right()) {
                    best_t = (left + right) / 2.0;
                }
            }
            
            // 如果找到了有效的交点
            if (ray_t.surrounds(best_t)) {
                rec.t = best_t;
                rec.p = r.at(rec.t);
                
                // 计算八面体的外向法向量
                vec3 local_point = rec.p - center;
                vec3 outward_normal;
                outward_normal.e[0] = (local_point.x() >= 0) ? 1.0 : -1.0;
                outward_normal.e[1] = (local_point.y() >= 0) ? 1.0 : -1.0;
                outward_normal.e[2] = (local_point.z() >= 0) ? 1.0 : -1.0;
                outward_normal = unit_vector(outward_normal);
                
                // 使用 set_face_normal 方法设置正确的法向量方向
                rec.set_face_normal(r, outward_normal);
                rec.mat = mat; // 设置材质
                
                return true;
            }
        }
        
        return false;
    }

  private:
    point3 center;
    double size;
    shared_ptr<material> mat;
};

#endif