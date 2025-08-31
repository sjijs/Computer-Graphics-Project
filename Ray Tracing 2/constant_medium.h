#ifndef CONSTANT_MEDIUM_H
#define CONSTANT_MEDIUM_H

#include "hittable.h"
#include "material.h"
#include "texture.h"
#include <vector>
#include <algorithm>

/**
 * 恒定密度介质类 - 用于渲染雾、烟雾、云朵等体积效果
 * 
 * 工作原理：
 * 1. 光线在介质中传播时会被粒子散射
 * 2. 散射发生的概率基于介质密度和传播距离
 * 3. 使用蒙特卡洛方法随机采样散射点
 */
class constant_medium : public hittable {
  public:
    /**
     * 构造函数 - 使用纹理定义介质外观
     * @param boundary 介质的边界几何体（如球体、盒子等）
     * @param density 介质密度，密度越高散射越频繁
     * @param tex 介质的纹理，决定散射后的颜色
     */
    constant_medium(shared_ptr<hittable> boundary, double density, shared_ptr<texture> tex)
      : boundary(boundary), 
        neg_inv_density(-1/density),  // 预计算负逆密度，用于指数分布采样
        phase_function(make_shared<isotropic>(tex))  // 各向同性散射材质
    {}

    /**
     * 构造函数重载 - 使用单一颜色定义介质
     * @param boundary 介质边界
     * @param density 介质密度
     * @param albedo 介质的反射颜色（散射后的颜色）
     */
    constant_medium(shared_ptr<hittable> boundary, double density, const color& albedo)
      : boundary(boundary), 
        neg_inv_density(-1/density),
        phase_function(make_shared<isotropic>(albedo))
    {}

    /**
     * 构造函数重载 - 球形非均匀密度介质
     * @param boundary 介质边界（应该是球体）
     * @param max_density 球心的最大密度值
     * @param albedo 介质的反射颜色
     * @param sphere_radius 球体半径
     * @param sphere_center 球心位置
     */
    constant_medium(shared_ptr<hittable> boundary, double max_density, const color& albedo, 
                   double sphere_radius, point3 sphere_center)
      : boundary(boundary),
        max_density(max_density),
        phase_function(make_shared<isotropic>(albedo)),
        sphere_radius(sphere_radius),
        sphere_center(sphere_center),
        use_variable_density(true)
    {}

    /**
     * 光线-介质交点检测
     * 
     * 算法流程：
     * 1. 计算光线与边界的两个交点（进入点和离开点）
     * 2. 在介质内部随机采样散射点
     * 3. 如果散射点在有效范围内，返回交点信息
     */
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        hit_record rec1, rec2;

        // 步骤1：找到光线与边界的第一个交点（进入点）
        if (!boundary->hit(r, interval::universe, rec1)) // hit函数被调用时即完成了相关rec点的计算
            return false;  // 光线完全错过介质

        // 步骤2：找到光线与边界的第二个交点（离开点）
        // 注意：从第一个交点稍后开始搜索，避免数值精度问题
        if (!boundary->hit(r, interval(rec1.t+0.0001, infinity), rec2)) 
            return false;  // 光线只有一个交点（切线相交）

        // 步骤3：将交点限制在有效的光线参数范围内
        if (rec1.t < ray_t.min) rec1.t = ray_t.min;  // 裁剪到光线起始点
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;  // 裁剪到光线结束点

        // 步骤4：检查是否有有效的介质区间
        if (rec1.t >= rec2.t)
            return false;  // 无效区间

        // 步骤5：确保不在光线起点之前
        if (rec1.t < 0)
            rec1.t = 0;

        // 步骤6：计算光线在介质内的传播距离
        auto ray_length = r.direction().length();  // 光线方向的长度
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;  // 实际物理距离

        // 步骤7：根据密度类型采样散射距离
        auto hit_distance = 0.0;
        
        if (use_variable_density) {
            // 对于球形非均匀密度：使用数值积分方法
            hit_distance = sample_variable_density_distance(r, rec1.t, distance_inside_boundary, ray_length);
        } else {
            // 对于恒定密度：使用指数分布随机采样
            // 基于Beer-Lambert定律：I = I₀ * e^(-σt)
            hit_distance = neg_inv_density * std::log(random_double());
        }

        // 步骤8：检查散射是否发生在介质内部
        if (hit_distance > distance_inside_boundary)
            return false;  // 光线穿过介质但未发生散射

        // 步骤9：计算散射点的位置和属性
        rec.t = rec1.t + hit_distance / ray_length;  // 散射点的光线参数
        rec.p = r.at(rec.t);  // 散射点的3D坐标

        // 步骤10：设置散射点的表面属性
        // 注意：这些值是任意的，因为体积渲染不依赖于表面法向量
        rec.normal = vec3(1,0,0);  // 任意法向量
        rec.front_face = true;     // 任意朝向
        rec.mat = phase_function;  // 使用各向同性散射材质

        return true;  // 成功找到散射点
    }

    /**
     * 返回介质的包围盒
     * 介质的包围盒就是其边界几何体的包围盒
     */
    aabb bounding_box() const override { 
        return boundary->bounding_box(); 
    }

  private:
    shared_ptr<hittable> boundary;      // 介质的边界几何体
    double neg_inv_density;             // 负逆密度 (-1/density)，用于指数分布采样
    shared_ptr<material> phase_function; // 相位函数材质（各向同性散射）
    
    // 球形非均匀密度参数
    double max_density = 0;             // 球心的最大密度
    double sphere_radius = 0;           // 球体半径
    point3 sphere_center;               // 球心位置
    bool use_variable_density = false;  // 是否使用变密度

    /**
     * 球形非均匀密度的散射距离采样
     * 使用逆变换采样法处理线性密度变化
     */
    double sample_variable_density_distance(const ray& r, double t_start, 
                                           double max_distance, double ray_length) const {
        // 使用分段线性近似和逆变换采样
        const int num_samples = 100;
        double step_size = max_distance / num_samples;
        
        // 计算累积密度积分（光学深度）
        std::vector<double> optical_depth(num_samples + 1, 0.0);
        
        for (int i = 1; i <= num_samples; ++i) {
            double current_distance = i * step_size;
            double current_t = t_start + current_distance / ray_length;
            point3 current_point = r.at(current_t);
            
            // 计算当前点的密度
            double distance_to_center = (current_point - sphere_center).length();
            double normalized_distance = std::min(distance_to_center / sphere_radius, 1.0);
            double local_density = max_density * (1.0 - normalized_distance);
            
            // 累积光学深度（梯形积分）
            if (i == 1) {
                optical_depth[i] = local_density * step_size;
            } else {
                double prev_distance = (i - 1) * step_size;
                double prev_t = t_start + prev_distance / ray_length;
                point3 prev_point = r.at(prev_t);
                double prev_dist_to_center = (prev_point - sphere_center).length();
                double prev_normalized = std::min(prev_dist_to_center / sphere_radius, 1.0);
                double prev_density = max_density * (1.0 - prev_normalized);
                
                optical_depth[i] = optical_depth[i-1] + 0.5 * (prev_density + local_density) * step_size;
            }
        }
        
        // 随机选择一个光学深度值
        double total_optical_depth = optical_depth[num_samples];
        if (total_optical_depth <= 0) {
            return max_distance + 1.0; // 无散射
        }
        
        double target_optical_depth = -std::log(random_double()) * total_optical_depth / total_optical_depth;
        
        // 二分查找对应的物理距离
        for (int i = 1; i <= num_samples; ++i) {
            if (optical_depth[i] >= target_optical_depth) {
                // 线性插值
                double fraction = (target_optical_depth - optical_depth[i-1]) / 
                                (optical_depth[i] - optical_depth[i-1]);
                return ((i - 1) + fraction) * step_size;
            }
        }
        
        return max_distance + 1.0; // 无散射
    }
};

/*
  此外，上面的代码假设一旦射线离开恒定介质边界，它就会永远在边界之外继续存在。
  换句话说，它假设边界形状是凸的。因此，此特定实现适用于框或球体等边界，但不适用于包含空隙的环形或形状。
*/

#endif