#ifndef HITTABLE_LIST_H
#define HITTABLE_LIST_H

#include "aabb.h"
#include "hittable.h"

#include <memory>
#include <vector>

using std::make_shared;
using std::shared_ptr;

class hittable_list : public hittable {
  public:
    std::vector<shared_ptr<hittable>> objects;

    hittable_list() {}
    hittable_list(shared_ptr<hittable> object) { add(object); }

    void clear() { objects.clear(); }

    void add(shared_ptr<hittable> object) {
        objects.push_back(object);
        bbox = aabb(bbox, object->bounding_box());// 更新包围盒
        // object是基类指针，实际指向派生类对象
        // 典型的运行时多态，调用时自动推断object类型，从而在虚函数表中查找相应的实现，从而调用具体的函数
    }

    // 重写父类的hit函数
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // const override表示该函数不会修改类的成员变量
        hit_record temp_rec;
        bool hit_anything = false;
        auto closest_so_far = ray_t.max; // 最近相交的物体

        // 遍历所有物体，检查光线是否与之相交
        for (const auto& object : objects) {
            if (object->hit(r, interval(ray_t.min, closest_so_far), temp_rec)) { // 进入具体物体的hit函数
                // closest_so_far作为递归传入，将其传入为最远距离限制，变相比较了新物体与上一个物体的远近
                // 如果第二个物体在 t2 处相交且 t2 < t1：才会返回 true，closest_so_far 更新为 t2
                hit_anything = true;
                closest_so_far = temp_rec.t; // 更新最近相交的物体
                rec = temp_rec; // 更新相交信息
            }
        }

        return hit_anything;
    }

    aabb bounding_box() const override {
        return bbox;
    }

    double pdf_value(const point3& origin, const vec3& direction) const override {
        if (objects.empty()) return 0.0;

        const auto weight = 1.0 / objects.size();
        double sum = 0.0;
        for (const auto& object : objects) {
            sum += weight * object->pdf_value(origin, direction);
        }

        return sum;
    }

    vec3 random(const point3& origin) const override {
        if (objects.empty()) return random_unit_vector();

        auto index = random_int(0, static_cast<int>(objects.size()) - 1);
        return objects[index]->random(origin);
    }

  private:
    aabb bbox;  // 包围盒
};

#endif