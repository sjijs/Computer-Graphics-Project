#ifndef TRIANGLE_MESH_H
#define TRIANGLE_MESH_H

#include "hittable.h"
#include "hittable_list.h"
#include "triangle.h"
#include "bvh.h"
#include <memory>
#include <vector>

class triangle_mesh : public hittable {
public:
    triangle_mesh() = default;

    void add_triangle(const point3& a, const point3& b, const point3& c, std::shared_ptr<material> mat) {
        // triangle 继承自 quad，构造参数为 (Q, u, v)，其中 u = B - A，v = C - A
        // 先前直接传 (a,b,c) 会被解释为 Q=a, u=b, v=c，导致几何错误（针状/扁平问题）。
        tris.push_back(std::make_shared<triangle>(a, b - a, c - a, mat));
    }

    void add_triangle(
        const point3& a, const point3& b, const point3& c,
        const vec3& uv_a, const vec3& uv_b, const vec3& uv_c,
        const vec3& n_a, const vec3& n_b, const vec3& n_c,
        bool has_uv, bool has_normals,
        std::shared_ptr<material> mat
    ) {
        tris.push_back(std::make_shared<triangle>(
            a, b, c,
            uv_a, uv_b, uv_c,
            n_a, n_b, n_c,
            has_uv, has_normals,
            mat
        ));
    }

    void build_bvh() {
        auto list = std::make_shared<hittable_list>();
        for (auto& t : tris) list->add(t);
        accel = std::make_shared<bvh_node>(*list);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!accel) return false;
        return accel->hit(r, ray_t, rec);
    }

    aabb bounding_box() const override {
        if (!accel) return aabb();
        return accel->bounding_box();
    }

private:
    std::vector<std::shared_ptr<hittable>> tris;
    std::shared_ptr<hittable> accel; // bvh_node
};

#endif