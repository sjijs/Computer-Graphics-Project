#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"

class triangle : public hittable {
  public:
    // 兼容旧接口：Q, Q+u, Q+v
    triangle(const point3& Q, const vec3& u, const vec3& v, shared_ptr<material> mat)
      : triangle(Q, Q + u, Q + v,
                 vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0),
                 vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0),
                 false, false, mat) {}

    triangle(const point3& v0, const point3& v1, const point3& v2,
             const vec3& uv0, const vec3& uv1, const vec3& uv2,
             const vec3& n0, const vec3& n1, const vec3& n2,
             bool has_uv, bool has_normals,
             shared_ptr<material> mat)
      : v0(v0), v1(v1), v2(v2),
        uv0(uv0), uv1(uv1), uv2(uv2),
        n0(n0), n1(n1), n2(n2),
        has_uv(has_uv), has_normals(has_normals), mat(mat)
    {
        auto face = cross(v1 - v0, v2 - v0);
        face_normal = unit_vector(face);
        compute_tangent_space();
        set_bounding_box();
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        const double eps = 1e-8;
        vec3 e1 = v1 - v0;
        vec3 e2 = v2 - v0;

        vec3 pvec = cross(r.direction(), e2);
        double det = dot(e1, pvec);
        if (std::fabs(det) < eps) return false;

        double inv_det = 1.0 / det;
        vec3 tvec = r.origin() - v0;
        double b1 = dot(tvec, pvec) * inv_det;
        if (b1 < 0.0 || b1 > 1.0) return false;

        vec3 qvec = cross(tvec, e1);
        double b2 = dot(r.direction(), qvec) * inv_det;
        if (b2 < 0.0 || b1 + b2 > 1.0) return false;

        double t = dot(e2, qvec) * inv_det;
        if (!ray_t.surrounds(t)) return false;

        double b0 = 1.0 - b1 - b2;

        rec.t = t;
        rec.p = r.at(t);
        rec.mat = mat;

        vec3 outward_normal = face_normal;
        if (has_normals) {
            outward_normal = unit_vector(b0 * n0 + b1 * n1 + b2 * n2);
        }
        rec.set_face_normal(r, outward_normal);

        if (has_uv) {
            vec3 uv = b0 * uv0 + b1 * uv1 + b2 * uv2;
            rec.u = uv.x();
            rec.v = uv.y();
        } else {
            rec.u = b1;
            rec.v = b2;
        }

        rec.tangent = tangent;
        rec.bitangent = bitangent;
        rec.has_tangent_space = has_uv;

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

        return dist2 / (cosine * area());
    }

    vec3 random(const point3& origin) const override {
        // 均匀采样三角形面积
        double r1 = std::sqrt(random_double());
        double r2 = random_double();
        double a = 1.0 - r1;
        double b = r1 * (1.0 - r2);
        double c = r1 * r2;

        point3 p = a * v0 + b * v1 + c * v2;
        return p - origin;
    }

  private:
    point3 v0, v1, v2;
    vec3 uv0, uv1, uv2;
    vec3 n0, n1, n2;
    bool has_uv = false;
    bool has_normals = false;

    vec3 face_normal;
    vec3 tangent = vec3(1, 0, 0);
    vec3 bitangent = vec3(0, 1, 0);

    shared_ptr<material> mat;
    aabb bbox;

    void set_bounding_box() {
        point3 minp(
            std::fmin(v0.x(), std::fmin(v1.x(), v2.x())),
            std::fmin(v0.y(), std::fmin(v1.y(), v2.y())),
            std::fmin(v0.z(), std::fmin(v1.z(), v2.z()))
        );
        point3 maxp(
            std::fmax(v0.x(), std::fmax(v1.x(), v2.x())),
            std::fmax(v0.y(), std::fmax(v1.y(), v2.y())),
            std::fmax(v0.z(), std::fmax(v1.z(), v2.z()))
        );
        bbox = aabb(minp, maxp);
    }

    double area() const {
        return 0.5 * cross(v1 - v0, v2 - v0).length();
    }

    void compute_tangent_space() {
        if (!has_uv) {
            // 没有UV时构造一个稳定基
            auto up = std::fabs(face_normal.y()) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
            tangent = unit_vector(cross(up, face_normal));
            bitangent = cross(face_normal, tangent);
            return;
        }

        vec3 e1 = v1 - v0;
        vec3 e2 = v2 - v0;
        vec3 duv1 = uv1 - uv0;
        vec3 duv2 = uv2 - uv0;

        double det = duv1.x() * duv2.y() - duv2.x() * duv1.y();
        if (std::fabs(det) < 1e-10) {
            auto up = std::fabs(face_normal.y()) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
            tangent = unit_vector(cross(up, face_normal));
            bitangent = cross(face_normal, tangent);
            return;
        }

        double inv = 1.0 / det;
        tangent = unit_vector(inv * (duv2.y() * e1 - duv1.y() * e2));
        bitangent = unit_vector(inv * (-duv2.x() * e1 + duv1.x() * e2));
    }
};

#endif