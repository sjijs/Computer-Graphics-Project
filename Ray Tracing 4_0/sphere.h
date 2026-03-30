#ifndef SPHERE_H
#define SPHERE_H

#include "hittable.h"
#include "vec3.h"

class sphere : public hittable {
  public:
    // Stationary Sphere
    sphere(const point3& static_center, double radius, shared_ptr<material> mat)
      : center(static_center, vec3(0,0,0)), radius(std::fmax(0,radius)), mat(mat) {
        auto rvec = vec3(radius, radius, radius);
        bbox = aabb(static_center - rvec, static_center + rvec);
      }

    // Moving Sphere
    sphere(const point3& center1, const point3& center2, double radius,
           shared_ptr<material> mat)
        : center(center1, center2 - center1), radius(std::fmax(0,radius)), mat(mat)
    {
        auto rvec = vec3(radius, radius, radius);
        aabb box1(center.at(0) - rvec, center.at(0) + rvec);
        aabb box2(center.at(1) - rvec, center.at(1) + rvec);
        bbox = aabb(box1, box2);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        point3 current_center = center.at(r.time());// 这里的center变量是类的成员变量
        vec3 oc = current_center - r.origin();
        auto a = r.direction().length_squared();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius*radius;// 同理，radius也是类的成员变量

        auto discriminant = h*h - a*c;// 判别式
        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrt(discriminant);

        // 计算两个可能的交点
        // 如果第一个交点不在范围内，则计算第二个交点
        auto root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root))
                return false;
        }

        rec.t = root;// 记录交点的 t 值
        rec.p = r.at(rec.t);// 计算交点位置
        vec3 outward_normal = (rec.p - current_center) / radius;
        rec.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, rec.u, rec.v); // 计算纹理坐标
        // 传入法向量，利用了球体的便利性，直接就能用来算映射
        rec.mat = mat;

        return true;
    }

    aabb bounding_box() const override {
        return bbox;
    }

    double pdf_value(const point3& origin, const vec3& direction) const override {
      hit_record rec;
      if (!this->hit(ray(origin, direction), interval(0.001, infinity), rec))
        return 0.0;

      auto dist2 = rec.t * rec.t * direction.length_squared();
      auto cosine = std::fabs(dot(unit_vector(direction), rec.normal));
      if (cosine < 1e-8) return 0.0;

      auto area = 4.0 * pi * radius * radius;
      return dist2 / (cosine * area);
    }

    vec3 random(const point3& origin) const override {
      point3 sample_center = center.at(0.5);
      point3 point_on_surface = sample_center + radius * random_unit_vector();
      return point_on_surface - origin;
    }

  private:
    ray center;
    double radius;
    shared_ptr<material> mat;
    aabb bbox;

    static void get_sphere_uv(const point3& p, double& u, double& v) {
        // p: a given point on the sphere of radius one, centered at the origin.
        // u: returned value [0,1] of angle around the Y axis from X=-1.
        // v: returned value [0,1] of angle from Y=-1 to Y=+1.
        //     <1 0 0> yields <0.50 0.50>       <-1  0  0> yields <0.00 0.50>
        //     <0 1 0> yields <0.50 1.00>       < 0 -1  0> yields <0.50 0.00>
        //     <0 0 1> yields <0.25 0.50>       < 0  0 -1> yields <0.75 0.50>

        auto theta = std::acos(-p.y());
        auto phi = std::atan2(-p.z(), p.x()) + pi;

        u = phi / (2*pi);
        v = theta / pi;
    }
};

#endif