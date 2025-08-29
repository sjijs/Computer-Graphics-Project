#ifndef PERLIN_H
#define PERLIN_H

#include "rtweekend.h"
#include "vec3.h"

/*
perlin - 简单的“伪随机值噪声”生成器（value noise 风格）
--------------------------------------------------------------------
概述：
    该类实现了一种基于事先生成的随机数表和三组置换表的伪随机空间噪声。
    与经典的 Perlin gradient noise 不同，这里返回的是基于格点索引查表得到的标量值（value noise），
    而不是基于梯度插值的连续梯度噪声。

主要成员说明：
    - perlin() [构造函数]
            初始化三个置换数组（perm_x、perm_y、perm_z）和一个长度为 point_count 的随机浮点数组 randfloat。
            randfloat[i] 保存介于 [0,1) 的随机 double，perm_* 是对 [0..point_count-1] 的随机置换。
            置换通过 Fisher–Yates 洗牌（permute）完成，使用外部提供的 random_int 作为随机源。
            注意：随机数生成器（random_double/random_int）在类外定义并控制种子；因此复现性取决于其实现。

    - double noise(const point3& p) const
            基于三维点 p 给出一个标量噪声值（double）。
            逻辑步骤（高层）：
                1. 将点坐标分别按分辨率因子 4.0 放大并截断为整数（int(4*p.x()) 等）。
                2. 对这三个整数做位掩码 & 255，确保索引落在 [0, point_count-1] 范围（point_count==256）。
                3. 使用三个置换表取出对应的置换索引，按位异或（^）合并三个置换值，得到最终索引。
                4. 用该索引在 randfloat 中查表并返回对应的随机值。
            语义与性质：
                - 返回的噪声是离散格点驱动的“值噪声”，随位置变化呈块状/分段变化（没有插值）。
                - 通过对坐标放大（乘以 4）可以控制噪声的频率；放大倍数越大，噪声变化越频繁。
                - 使用三组独立置换并通过异或合并，目的是将 x/y/z 的分量混合以获得更复杂的伪随机索引。
                - 使用位掩码保证周期为 256（即沿每轴的周期性为 point_count），因此噪声是周期性的。

私有成员与辅助函数：
    - static const int point_count = 256;
            噪声表和置换表的固定大小（256），并与 mask (255) 配合使用以快速取模。

    - double randfloat[point_count];
            存储预生成的随机双精度浮点数，用作格点值查表。

    - int perm_x[point_count], perm_y[point_count], perm_z[point_count];
            三组长度为 point_count 的置换数组。通过随机置换对格点索引进行扰动以打破轴向相关性。

    - static void perlin_generate_perm(int* p)
            将 p 初始化为 0..point_count-1 的顺序序列，然后调用 permute 对其随机置换。

    - static void permute(int* p, int n)
            使用 Fisher–Yates 算法对数组 p[0..n-1] 进行原地随机打乱：
                for i 从 n-1 到 1：
                    target = random_int(0, i)
                    swap(p[i], p[target])
            注意：random_int(0,i) 应返回闭区间内的整数，shuffle 的正确性依赖于该合同。

额外注意事项与限制：
    - 该实现产生的是值噪声（value noise），缺少插值步骤（如线性/三次插值）会导致噪声随位置跳变。
        若需要平滑连续的噪声，应在格点值间做插值或改用梯度噪声实现。
    - 周期性：由于对索引做了 & 255 操作，噪声在每个轴方向上以 256 为周期重复。
    - 线程安全：构造函数在内部调用全局随机函数（random_double/random_int）；并发调用这些函数或在多线程环境下
        共享同一 perlin 实例可能需要额外同步，视外部随机数实现而定。
    - 复杂度：构造时为 O(point_count)（用于生成随机数组和置换），噪声查询为常数时间 O(1)（若把随机表视为 O(1) 访问）。
    - 依赖项：该实现依赖外部定义的 random_double() 和 random_int(int a,int b)；这些函数的行为（范围、包含端点、种子）会直接影响噪声特性。
*/
class perlin {
  public:
    perlin() {
        for (int i = 0; i < point_count; i++) {
            randvec[i] = unit_vector(vec3::random(-1,1)); // 随机单位向量
        }

        perlin_generate_perm(perm_x);
        perlin_generate_perm(perm_y);
        perlin_generate_perm(perm_z);
    }

    double noise(const point3& p) const {
        auto u = p.x() - std::floor(p.x());
        auto v = p.y() - std::floor(p.y());
        auto w = p.z() - std::floor(p.z());
        // 使用 Hermite 立方来四舍五入插值
        // u = u*u*(3-2*u);
        // v = v*v*(3-2*v);
        // w = w*w*(3-2*w);

        auto i = int(std::floor(p.x()));
        auto j = int(std::floor(p.y()));
        auto k = int(std::floor(p.z()));
        vec3 c[2][2][2]; // 用于三线性插值的立方体顶点值

        // 计算立方体顶点值
        for (int di=0; di < 2; di++)
            for (int dj=0; dj < 2; dj++)
                for (int dk=0; dk < 2; dk++)
                    c[di][dj][dk] = randvec[
                        perm_x[(i+di) & 255] ^
                        perm_y[(j+dj) & 255] ^
                        perm_z[(k+dk) & 255]
                    ];

        return perlin_interp(c, u, v, w);
    }

    // 纹理涡旋
    double turb(const point3& p, int depth) const {
        auto accum = 0.0;
        auto temp_p = p;
        auto weight = 1.0;

        for (int i = 0; i < depth; i++) { // depth愈高，细节愈丰富
            accum += weight * noise(temp_p); // 多次重复添加噪声值
            weight *= 0.5; // 每次迭代减小权重
            temp_p *= 2; // 放大坐标以增加细节
        }

        return std::fabs(accum); // 取绝对值以避免负值
    }

  private:
    static const int point_count = 256;
    vec3 randvec[point_count];
    int perm_x[point_count];
    int perm_y[point_count];
    int perm_z[point_count];

    static void perlin_generate_perm(int* p) {
        for (int i = 0; i < point_count; i++)
            p[i] = i;

        permute(p, point_count);
    }

    static void permute(int* p, int n) {
        for (int i = n-1; i > 0; i--) {
            int target = random_int(0, i);
            int tmp = p[i];
            p[i] = p[target];
            p[target] = tmp;
        }
    }

    // 三线性插值
    static double trilinear_interp(double c[2][2][2], double u, double v, double w) {
        auto accum = 0.0;
        for (int i=0; i < 2; i++)
            for (int j=0; j < 2; j++)
                for (int k=0; k < 2; k++)
                    accum += (i*u + (1-i)*(1-u))
                           * (j*v + (1-j)*(1-v))
                           * (k*w + (1-k)*(1-w))
                           * c[i][j][k];

        return accum;
    }

    // 逐点插值
    static double perlin_interp(const vec3 c[2][2][2], double u, double v, double w) {
        auto uu = u*u*(3-2*u);
        auto vv = v*v*(3-2*v);
        auto ww = w*w*(3-2*w);
        auto accum = 0.0;

        for (int i=0; i < 2; i++)
            for (int j=0; j < 2; j++)
                for (int k=0; k < 2; k++) {
                    vec3 weight_v(u-i, v-j, w-k);
                    accum += (i*uu + (1-i)*(1-uu))
                           * (j*vv + (1-j)*(1-vv))
                           * (k*ww + (1-k)*(1-ww))
                           * dot(c[i][j][k], weight_v);
                }

        return accum;
    }
};

#endif