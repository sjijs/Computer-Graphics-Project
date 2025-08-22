#ifndef SPHERICAL_HARMONICS_H
#define SPHERICAL_HARMONICS_H

#include "vec3.h"
#include "color.h"
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

// 球谐函数环境光照类
class SphericalHarmonics {
public:
    // 构造函数
    SphericalHarmonics(int bands = 3) : num_bands(bands) {
        num_coefficients = bands * bands;
        sh_coefficients_r.resize(num_coefficients, 0.0);
        sh_coefficients_g.resize(num_coefficients, 0.0);
        sh_coefficients_b.resize(num_coefficients, 0.0);
    }

    // 从环境贴图生成球谐系数
    bool generateFromEnvironmentMap(const std::string& filename);

    // 根据方向向量评估球谐函数，返回颜色
    color evaluate(const vec3& direction) const;

    // 保存和加载球谐系数
    bool saveCoefficients(const std::string& filename) const;
    bool loadCoefficients(const std::string& filename);

    // 获取系数数量
    int getNumCoefficients() const { return num_coefficients; }
    int getNumBands() const { return num_bands; }

private:
    int num_bands;           // 球谐函数的阶数（通常3-4阶足够）
    int num_coefficients;    // 系数总数 = bands^2

    // RGB三个通道的球谐系数
    std::vector<double> sh_coefficients_r;
    std::vector<double> sh_coefficients_g;
    std::vector<double> sh_coefficients_b;

    // 环境贴图数据
    std::vector<unsigned char> env_map_data;
    int env_map_width, env_map_height;

    // 球谐基函数计算
    double sphericalHarmonic(int l, int m, double theta, double phi) const;
    
    // 归一化的勒让德多项式
    double associatedLegendre(int l, int m, double x) const;
    
    // 阶乘函数
    double factorial(int n) const;
    
    // 从PPM文件加载环境贴图
    bool loadEnvironmentMap(const std::string& filename);
    
    // 将笛卡尔坐标转换为球面坐标
    void cartesianToSpherical(const vec3& dir, double& theta, double& phi) const;
    
    // 从环境贴图采样颜色
    color sampleEnvironmentMap(double theta, double phi) const;
    
    // 球谐投影积分
    void projectEnvironmentMap();
};

// 实现球谐基函数
inline double SphericalHarmonics::sphericalHarmonic(int l, int m, double theta, double phi) const {
    const double PI = 3.14159265358979323846;
    
    // 预计算常用的球谐基函数（直到3阶）
    double cos_theta = cos(theta);
    double sin_theta = sin(theta);
    
    if (l == 0) {
        // Y_0^0
        return 0.282095 * sqrt(1.0 / (4.0 * PI));  // 1/(2*sqrt(π))
    }
    else if (l == 1) {
        if (m == -1) {
            // Y_1^{-1}
            return 0.488603 * sin_theta * sin(phi);
        }
        else if (m == 0) {
            // Y_1^0
            return 0.488603 * cos_theta;
        }
        else if (m == 1) {
            // Y_1^1
            return -0.488603 * sin_theta * cos(phi);
        }
    }
    else if (l == 2) {
        if (m == -2) {
            // Y_2^{-2}
            return 1.092548 * sin_theta * sin_theta * sin(2.0 * phi);
        }
        else if (m == -1) {
            // Y_2^{-1}
            return 1.092548 * sin_theta * cos_theta * sin(phi);
        }
        else if (m == 0) {
            // Y_2^0
            return 0.315392 * (3.0 * cos_theta * cos_theta - 1.0);
        }
        else if (m == 1) {
            // Y_2^1
            return -1.092548 * sin_theta * cos_theta * cos(phi);
        }
        else if (m == 2) {
            // Y_2^2
            return 0.546274 * sin_theta * sin_theta * cos(2.0 * phi);
        }
    }
    
    return 0.0;  // 更高阶的实现可以根据需要添加
}

// 将笛卡尔坐标转换为球面坐标
inline void SphericalHarmonics::cartesianToSpherical(const vec3& dir, double& theta, double& phi) const {
    const double PI = 3.14159265358979323846;
    
    // 归一化方向向量
    vec3 d = unit_vector(dir);
    
    // theta: 从+Y轴测量的极角 [0, π]
    theta = acos(std::clamp(d.y(), -1.0, 1.0));
    
    // phi: 从+X轴测量的方位角 [-π, π]
    phi = atan2(d.z(), d.x());
}

// 从环境贴图采样颜色
inline color SphericalHarmonics::sampleEnvironmentMap(double theta, double phi) const {
    const double PI = 3.14159265358979323846;
    
    if (env_map_data.empty()) {
        return color(0.5, 0.7, 1.0);  // 默认天空色
    }
    
    // 将球面坐标映射到纹理坐标
    double u = (phi + PI) / (2.0 * PI);  // [0, 1]
    double v = theta / PI;               // [0, 1]
    
    // 边界处理
    u = u - std::floor(u);  // 环绕
    v = std::clamp(v, 0.0, 1.0);
    
    // 转换为像素坐标
    int i = static_cast<int>(u * env_map_width);
    int j = static_cast<int>(v * env_map_height);
    
    i = std::clamp(i, 0, env_map_width - 1);
    j = std::clamp(j, 0, env_map_height - 1);
    
    // 采样像素
    int idx = (j * env_map_width + i) * 3;
    if (idx + 2 < static_cast<int>(env_map_data.size())) {
        return color(
            env_map_data[idx] / 255.0,
            env_map_data[idx + 1] / 255.0,
            env_map_data[idx + 2] / 255.0
        );
    }
    
    return color(0.5, 0.7, 1.0);
}

// 评估球谐函数
inline color SphericalHarmonics::evaluate(const vec3& direction) const {
    double theta, phi;
    cartesianToSpherical(direction, theta, phi);
    
    double r = 0.0, g = 0.0, b = 0.0;
    
    // 计算球谐基函数的线性组合
    int idx = 0;
    for (int l = 0; l < num_bands; ++l) {
        for (int m = -l; m <= l; ++m) {
            double sh_value = sphericalHarmonic(l, m, theta, phi);
            r += sh_coefficients_r[idx] * sh_value;
            g += sh_coefficients_g[idx] * sh_value;
            b += sh_coefficients_b[idx] * sh_value;
            ++idx;
        }
    }
    
    // 确保颜色值在有效范围内
    r = std::clamp(r, 0.0, 1.0);
    g = std::clamp(g, 0.0, 1.0);
    b = std::clamp(b, 0.0, 1.0);
    
    return color(r, g, b);
}

// 从PPM文件加载环境贴图
inline bool SphericalHarmonics::loadEnvironmentMap(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法打开环境贴图文件: " << filename << std::endl;
        return false;
    }
    
    std::string magic;
    file >> magic;
    
    if (magic != "P6") {
        std::cerr << "不支持的PPM格式: " << magic << std::endl;
        return false;
    }
    
    int maxval;
    file >> env_map_width >> env_map_height >> maxval;
    file.ignore(1);  // 跳过换行符
    
    std::cout << "加载环境贴图: " << env_map_width << "x" << env_map_height << std::endl;
    
    env_map_data.resize(env_map_width * env_map_height * 3);
    file.read(reinterpret_cast<char*>(env_map_data.data()), env_map_data.size());
    
    if (!file) {
        std::cerr << "读取环境贴图数据失败" << std::endl;
        return false;
    }
    
    file.close();
    return true;
}

// 球谐投影 - 将环境贴图投影到球谐基函数
inline void SphericalHarmonics::projectEnvironmentMap() {
    const double PI = 3.14159265358979323846;
    const int samples_theta = 64;  // θ方向采样数
    const int samples_phi = 128;   // φ方向采样数
    
    // 初始化系数
    std::fill(sh_coefficients_r.begin(), sh_coefficients_r.end(), 0.0);
    std::fill(sh_coefficients_g.begin(), sh_coefficients_g.end(), 0.0);
    std::fill(sh_coefficients_b.begin(), sh_coefficients_b.end(), 0.0);
    
    std::cout << "开始球谐投影..." << std::endl;
    
    // 蒙特卡洛积分
    double total_weight = 0.0;
    
    for (int i = 0; i < samples_theta; ++i) {
        for (int j = 0; j < samples_phi; ++j) {
            // 均匀采样球面
            double theta = PI * (i + 0.5) / samples_theta;
            double phi = 2.0 * PI * (j + 0.5) / samples_phi;
            
            // 球面积分的雅可比行列式
            double sin_theta = sin(theta);
            double weight = sin_theta * (PI / samples_theta) * (2.0 * PI / samples_phi);
            
            // 从环境贴图采样
            color env_color = sampleEnvironmentMap(theta, phi);
            
            // 投影到每个球谐基函数
            int idx = 0;
            for (int l = 0; l < num_bands; ++l) {
                for (int m = -l; m <= l; ++m) {
                    double sh_value = sphericalHarmonic(l, m, theta, phi);
                    sh_coefficients_r[idx] += env_color.x() * sh_value * weight;
                    sh_coefficients_g[idx] += env_color.y() * sh_value * weight;
                    sh_coefficients_b[idx] += env_color.z() * sh_value * weight;
                    ++idx;
                }
            }
            
            total_weight += weight;
        }
        
        // 显示进度
        if (i % 8 == 0) {
            std::cout << "投影进度: " << (100 * i / samples_theta) << "%" << std::endl;
        }
    }
    
    std::cout << "球谐投影完成!" << std::endl;
    
    // 打印系数信息
    std::cout << "球谐系数 (前9个):" << std::endl;
    for (int i = 0; i < std::min(9, num_coefficients); ++i) {
        std::cout << "  [" << i << "] R:" << sh_coefficients_r[i] 
                  << " G:" << sh_coefficients_g[i] 
                  << " B:" << sh_coefficients_b[i] << std::endl;
    }
}

// 从环境贴图生成球谐系数
inline bool SphericalHarmonics::generateFromEnvironmentMap(const std::string& filename) {
    if (!loadEnvironmentMap(filename)) {
        return false;
    }
    
    projectEnvironmentMap();
    return true;
}

// 保存球谐系数到文件
inline bool SphericalHarmonics::saveCoefficients(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "无法创建系数文件: " << filename << std::endl;
        return false;
    }
    
    file << "# 球谐函数系数文件" << std::endl;
    file << "# 阶数: " << num_bands << std::endl;
    file << "# 系数数量: " << num_coefficients << std::endl;
    file << num_bands << " " << num_coefficients << std::endl;
    
    for (int i = 0; i < num_coefficients; ++i) {
        file << sh_coefficients_r[i] << " " 
             << sh_coefficients_g[i] << " " 
             << sh_coefficients_b[i] << std::endl;
    }
    
    file.close();
    std::cout << "球谐系数已保存到: " << filename << std::endl;
    return true;
}

// 从文件加载球谐系数
inline bool SphericalHarmonics::loadCoefficients(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "无法打开系数文件: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    // 跳过注释行
    while (std::getline(file, line) && line[0] == '#') {}
    
    // 解析头部信息
    std::istringstream iss(line);
    int loaded_bands, loaded_coeffs;
    iss >> loaded_bands >> loaded_coeffs;
    
    if (loaded_bands != num_bands || loaded_coeffs != num_coefficients) {
        std::cerr << "系数文件格式不匹配" << std::endl;
        return false;
    }
    
    // 读取系数
    for (int i = 0; i < num_coefficients; ++i) {
        file >> sh_coefficients_r[i] >> sh_coefficients_g[i] >> sh_coefficients_b[i];
    }
    
    file.close();
    std::cout << "球谐系数已从文件加载: " << filename << std::endl;
    return true;
}

#endif
