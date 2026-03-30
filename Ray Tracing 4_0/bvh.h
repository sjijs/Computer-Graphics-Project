#ifndef BVH_H
#define BVH_H

#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"
#include "rtweekend.h"

#include <algorithm>

class bvh_node : public hittable {
  public:
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {
        // There's a C++ subtlety here. This constructor (without span indices) creates an
        // implicit copy of the hittable list, which we will modify. The lifetime of the copied
        // list only extends until this constructor exits. That's OK, because we only need to
        // persist the resulting bounding volume hierarchy.
    }

    bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
        // Build the bounding box of the span of source objects.
        bbox = aabb::empty;
        for (size_t object_index=start; object_index < end; object_index++)
            bbox = aabb(bbox, objects[object_index]->bounding_box()); // 遍历所有物体，将物体的包围盒合并到一起，构造出最大的包围盒（根节点）

        int axis = bbox.longest_axis(); // 选择最长的轴进行划分

        auto comparator = (axis == 0) ? box_x_compare
                        : (axis == 1) ? box_y_compare
                                      : box_z_compare; // 选择比较函数

        size_t object_span = end - start; // 物体数量,

        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            left = objects[start];
            right = objects[start+1];
        } // 递归出口：如果只有一个物体：左右子树都指向它；如果有两个物体：左边一个，右边一个。
        else {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);
            // comparator的作用是根据选定轴对物体进行排序
            // start和end是索引，表示要排序的范围
            // comparator是一个函数，用于比较两个物体的包围盒在选定轴上的位置，从而决定它们的顺序


            auto mid = start + object_span/2; // mid只是数量值，表示该区间内的中点
            left = make_shared<bvh_node>(objects, start, mid); // objects在这里已经排好序了，所以只需要递归进入下一层子树即可
            right = make_shared<bvh_node>(objects, mid, end);
        }

    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!bbox.hit(r, ray_t)) // 进入aabb的hit函数
            return false;

        bool hit_left = left->hit(r, ray_t, rec); // 这里如果进入叶子节点，即多态转到具体物体的hit函数
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);
        // 这里如果进入叶子节点，即多态转到具体物体的hit函数

        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

    double pdf_value(const point3& origin, const vec3& direction) const override {
        return 0.5 * left->pdf_value(origin, direction) + 0.5 * right->pdf_value(origin, direction);
    }

    vec3 random(const point3& origin) const override {
        if (random_double() < 0.5) {
            return left->random(origin);
        }
        return right->random(origin);
    }

  private:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;

    static bool box_compare(
        const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index
    ) {
        // 传入参数：两个包围盒和一个轴索引
        auto a_axis_interval = a->bounding_box().axis_interval(axis_index); // 按照轴索引获取包围盒在该轴上的区间
        auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 2);
    }
};

#endif