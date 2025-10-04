#ifndef ANNULUS_H
#define ANNULUS_H

#include "hittable.h"
#include "quad.h"

/**
 * 环形类 - 用于创建土星环等环状结构
 * 继承自quad类，通过内外半径控制环形形状
 */
class annulus : public quad {
  public:
    annulus(
        const point3& center, const vec3& u, const vec3& v, 
        double inner_radius, double outer_radius, shared_ptr<material> m)
      : quad(center, u, v, m), inner_radius(inner_radius), outer_radius(outer_radius)
    {
        // 确保内半径小于外半径
        if (inner_radius >= outer_radius) {
            std::swap(inner_radius, outer_radius);
        }
    }

    virtual void set_bounding_box() override {
        // 环形的包围盒就是完整四边形的包围盒
        auto bbox_diagonal1 = aabb(Q, Q + u + v);
        auto bbox_diagonal2 = aabb(Q + u, Q + v);
        bbox = aabb(bbox_diagonal1, bbox_diagonal2);
    }

    virtual bool is_interior(double a, double b, hit_record& rec) const override {
        // 计算从环心到撞击点的距离
        auto hit_distance_squared = a*a + b*b;
        auto hit_distance = sqrt(hit_distance_squared);
        
        // 检查是否在环形内（大于内半径，小于外半径）
        if (hit_distance < inner_radius || hit_distance > outer_radius)
            return false;

        // 设置纹理坐标
        // u坐标：基于距离环心的距离，内环心至外环心
        rec.u = (hit_distance - inner_radius) / (outer_radius - inner_radius);

        // v坐标：始终为1.0
        rec.v = 1.0;
        
        return true;
    }

  private:
    double inner_radius;  // 内半径
    double outer_radius;  // 外半径
};

#endif