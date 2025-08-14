# Ray Tracing
**基于Ray tracing in one week实现光追渲染器**

## The PPM Image Format
- ![alt text](image.png)
- 在这里遇到了一个问题，就是使用PowerShell的默认重定向导致出现了以 UTF-16 编码保存文件，导致PPM 文件无法正常显示。
- **修改方法**
  - 直接在C++中使用文件读写即可解决问题，而不是使用教程中的指定输出形式


## Cmake相关知识
### 1. **CMake 与 CMakeLists.txt 的关系**

* **CMake**：一个**跨平台的构建系统生成工具**，本身并不直接编译代码，而是**根据项目的配置文件（CMakeLists.txt）生成构建系统**（如 Makefile、Visual Studio 工程、Ninja 构建脚本等），然后再由对应构建系统去调用编译器完成编译。
* **CMakeLists.txt**：CMake 的**配置脚本文件**，使用 CMake 语法编写，告诉 CMake：

  * 项目有哪些源文件
  * 编译需要哪些库
  * 需要哪些编译选项
  * 最终如何链接生成可执行文件或库
* **关系总结**：

  > CMake 是工具，CMakeLists.txt 是它的说明书。没有 CMakeLists.txt，CMake 不知道怎么构建你的项目。

---

### 2. **CMake 的基本工作流程**

假设你有一个项目 `my_project`：

```
my_project/
├── CMakeLists.txt
├── main.cpp
└── src/
    └── utils.cpp
```

工作流程通常如下：

1. **编写 CMakeLists.txt**
   定义项目构建规则，比如：

   ```cmake
   cmake_minimum_required(VERSION 3.10)   # 最低CMake版本要求
   project(MyProject)                     # 项目名称
   add_executable(my_app main.cpp src/utils.cpp)  # 生成可执行文件
   ```

2. **运行 CMake 配置项目**

   * 在项目目录外创建一个构建目录（推荐 out-of-source build）：

     ```bash
     mkdir build
     cd build
     ```
   * 运行：

     ```bash
     cmake ..
     ```

     这一步：

     * 读取 `../CMakeLists.txt`
     * 根据平台、编译器、选项生成构建系统文件（如 Linux 下的 `Makefile`，Windows 下的 `.vcxproj`）。

3. **编译项目**

   * 使用生成的构建系统进行编译：

     ```bash
     cmake --build .
     ```

     或：

     ```bash
     make
     ```

     这一步才会调用 `g++`、`clang++`、`msbuild` 等实际编译器来编译源码。

---

### 3. **CMake 的优点**

* **跨平台**：一次配置，多平台构建（Windows、Linux、MacOS）。
* **可扩展**：支持添加自定义构建规则、外部库依赖等。
* **灵活性**：可以生成不同的构建系统（Makefile、Ninja、Visual Studio 工程等）。
* **分离构建目录**：避免源代码目录被临时编译文件污染。

---

## cmd与powershell

### 1. **联系**

* **都是 Windows 的命令行工具**，用于和系统交互。
* **都能运行批处理脚本**（.bat 或 .cmd）。
* **都可以执行外部程序**（如 `ping`, `ipconfig` 等）。
* **PowerShell 可以在一定程度上兼容 cmd 的命令**（很多 cmd 命令在 PowerShell 里直接输入也能用）。

---

### 2. **区别**

| 对比点      | **cmd（命令提示符）**            | **PowerShell**                                 |
| -------- | ------------------------- | ---------------------------------------------- |
| **定位**   | 传统命令行解释器（MS-DOS 时代延续）     | 面向系统管理与自动化的脚本环境                                |
| **命令集**  | 基于字符串的命令（如 `dir`, `copy`） | 基于 .NET 的命令（称为 *cmdlet*，如 `Get-ChildItem`）     |
| **脚本文件** | `.bat` / `.cmd`           | `.ps1`                                         |
| **数据处理** | 输出都是纯文本，需要自己解析            | 输出是 **对象**（可直接访问属性和方法）                         |
| **功能**   | 基本文件操作和简单系统命令             | 支持复杂系统管理（注册表、服务、进程、WMI、网络等）                    |
| **可扩展性** | 需要外部程序                    | 内置丰富模块，可直接调用 .NET API                          |
| **跨平台**  | 仅 Windows                 | PowerShell Core（7.x 版本）已支持 Windows、Linux、macOS |

---

### 3. **工作方式的核心区别**

* **cmd**：命令运行后，返回的是纯文本输出。例如：

  ```cmd
  dir
  ```

  返回一堆字符串，如果你想处理这些数据，得用字符串解析。

* **PowerShell**：命令运行后，返回的是**对象集合**。例如：

  ```powershell
  Get-ChildItem
  ```

  返回的是 `FileInfo`、`DirectoryInfo` 对象，可以直接用：

  ```powershell
  Get-ChildItem | Where-Object {$_.Length -gt 100KB}
  ```

  按文件大小直接筛选。

---

### 4. **什么时候用哪个？**

* **cmd**：运行一些旧脚本、批处理文件、简单命令。
* **PowerShell**：做系统管理、批量任务处理、复杂自动化、跨平台管理。

---

- **眼点与视窗模型天然就是透视视角**

## Adding a Cube
- 该思路基于教程中的添加球体进一步提出了添加立方体的方法
- 求出立方体相交方程后找到了如下的方法来求解光线与立方体相交的解  

方程是：

$$
|C_x - Q_x - t d_x| + |C_y - Q_y - t d_y| + |C_z - Q_z - t d_z| = F
$$

其中 $t$ 是未知实数，其余都是已知量。
判断是否有解，本质上是判断 **函数**

$$
g(t) = |C_x - Q_x - t d_x| + |C_y - Q_y - t d_y| + |C_z - Q_z - t d_z|
$$

是否存在某个 $t \in \mathbb{R}$ 使 $g(t) = F$。

### 方法思路

1. **函数性质**

   * 这是绝对值函数的和，**分段线性且连续**，在各个拐点之间是一次函数。
   * 它的最小值出现在三个分量的“零点”附近，即每个 $|a_i - t b_i|$ 变号的地方。

2. **求最小值**

   * 记：

     $$
     a_x = C_x - Q_x, \quad a_y = C_y - Q_y, \quad a_z = C_z - Q_z
     $$
   * 三个绝对值项的零点：

     $$
     t_x = \frac{a_x}{d_x} \quad (d_x \neq 0)
     $$

     类似求出 $t_y, t_z$。
   * 取这些零点以及无穷远端点，计算 $g(t)$ 的最小值 $g_{\min}$。

3. **判断是否有解**

   * 因为 $g(t)$ 连续且趋向无穷大（当 $t \to \pm\infty$），如果

     $$
     F \ge g_{\min}
     $$

     就必然有解（中值定理）。
   * 如果 $F < g_{\min}$，则无解。

## AABB 方法 (Axis-Aligned Bounding Box)

### 算法原理

**AABB 方法** 是计算射线与轴对齐立方体相交的标准算法，核心思想是将 3D 相交问题分解为 3 个独立的 1D 问题。

### 算法步骤

1. **定义立方体边界**：
   ```
   min_bound = center - (length/2, length/2, length/2)
   max_bound = center + (length/2, length/2, length/2)
   ```

2. **对每个轴计算交点参数 t**：
   对于射线 $\mathbf{r}(t) = \mathbf{o} + t\mathbf{d}$，计算与每对平行面的交点：
   
   $$
   t_1 = \frac{\text{min\_bound}_i - o_i}{d_i}, \quad t_2 = \frac{\text{max\_bound}_i - o_i}{d_i}
   $$
   
   其中 $i \in \{x, y, z\}$

3. **求有效 t 范围**：
   ```
   t_min = max(t_1x, t_1y, t_1z)  // 进入立方体的最晚时间
   t_max = min(t_2x, t_2y, t_2z)  // 离开立方体的最早时间
   ```

4. **判断相交**：
   - 如果 `t_min <= t_max` 且 `t_max > 0`，则相交
   - 返回 `t_min`（如果 > 0）或 `t_max`

### 法线计算算法

对于击中点的法线计算：

1. **计算局部坐标**：`local_point = hit_point - center`

2. **确定最接近的面**：
   ```cpp
   if (|local_x| >= |local_y| && |local_x| >= |local_z|) {
       normal = (local_x > 0) ? (1,0,0) : (-1,0,0)  // X面
   } else if (|local_y| >= |local_z|) {
       normal = (local_y > 0) ? (0,1,0) : (0,-1,0)  // Y面  
   } else {
       normal = (local_z > 0) ? (0,0,1) : (0,0,-1)  // Z面
   }
   ```

### 算法优势

- **高效**：时间复杂度 O(1)
- **稳定**：数值稳定性好
- **标准**：广泛应用于碰撞检测和光线追踪
- **直观**：几何意义清晰

### 与推导方程的比较

| 方法 | 优点 | 缺点 |
|------|------|------|
| **推导方程法** | 数学严谨，理论完整 | 计算复杂，需要求最值 |
| **AABB方法** | 实现简单，计算高效 | 仅适用于轴对齐立方体 |

## 重要发现：方程的几何意义

### 原推导方程实际描述的是八面体

经过分析发现，原方程：

$$
g(t) = |C_x - Q_x - t d_x| + |C_y - Q_y - t d_y| + |C_z - Q_z - t d_z| = F
$$

实际上描述的是**八面体**（Octahedron），而不是立方体！

### 几何解释

- 这个方程表示的是到中心点的**曼哈顿距离**（L1 距离）等于常数 F 的点集合
- 八面体有 8 个三角形面，每个面的法向量都是 $(±1, ±1, ±1)$ 的某种组合
- 立方体的方程应该是：$\max(|x|, |y|, |z|) = F$（无穷范数）

### 八面体法向量计算

对于八面体上的点 $(x, y, z)$，其法向量为：

```cpp
normal = normalize(sign(x), sign(y), sign(z))
```

其中 `sign()` 函数返回坐标的符号（+1 或 -1）。

### 对比总结

| 几何体 | 距离度量 | 方程形式 | 面数 |
|--------|----------|----------|------|
| **八面体** | L1 距离（曼哈顿） | $\|x\| + \|y\| + \|z\| = F$ | 8个三角形面 |
| **立方体** | L∞ 距离（切比雪夫） | $\max(\|x\|, \|y\|, \|z\|) = F$ | 6个正方形面 |
| **球体** | L2 距离（欧几里得） | $x^2 + y^2 + z^2 = F^2$ | 1个曲面 |

## shared_ptr<type>
- shared_ptr<type> 是指向某个分配类型的指针，具有引用计数语义。
- 每次将其值分配给另一个共享指针（通常使用简单赋值）时，引用计数都会递增。
- 当共享指针超出范围（例如在块或函数的末尾）时，引用计数会递减。一旦计数变为零，对象就会被安全删除。
- **初始化示例**
```cpp
shared_ptr<double> double_ptr = make_shared<double>(0.37);
shared_ptr<vec3>   vec3_ptr   = make_shared<vec3>(1.414214, 2.718281, 1.618034);
shared_ptr<sphere> sphere_ptr = make_shared<sphere>(point3(0,0,0), 1.0);
```

## camera类重构前主函数中绘制逻辑
```cpp
int main() {

    // Image

    auto aspect_ratio = 16.0 / 9.0;
    int image_width = 400;

    // Calculate the image height, and ensure that it's at least 1.
    // 计算图像高度，并确保至少为1。
    // 如果计算结果小于1，则将其设置为1。
    int image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    // World

    hittable_list world;
    world.add(make_shared<sphere>(point3(0.5,0,-1), 0.5));
    world.add(make_shared<octahedron>(point3(-0.5,0,-1), 0.5));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));

    // Camera

    // 视口是相机的可视区域，定义在实际物理空间中，与最终图像有映射关系
    // image定义了最终图像的分辨率与像素
    // 视口是实际物理空间中的一个矩形区域，而image是这个区域的采样结果。
    auto focal_length = 1.0;// 相机焦距
    auto viewport_height = 2.0;// 相机视口高度
    auto viewport_width = viewport_height * (double(image_width)/image_height);// 相机视口宽度
    auto camera_center = point3(0, 0, 0);// 相机中心位置

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    // 计算水平和垂直视口边缘的向量。
    // 这些向量定义了相机视口的大小和方向。
    auto viewport_u = vec3(viewport_width, 0, 0);
    auto viewport_v = vec3(0, -viewport_height, 0);

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    // 计算从一个像素到下一个像素的水平和垂直增量向量。
    // 这些向量用于在视口上定位每个像素的位置。
    auto pixel_delta_u = viewport_u / image_width;
    auto pixel_delta_v = viewport_v / image_height;

    // Calculate the location of the upper left pixel.
    // 计算左上角像素的位置。
    auto viewport_upper_left = camera_center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
    // 计算左上角像素的中心位置。
    // 这里的0.5是为了将像素中心对齐到视口的左上角。
    auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    // 创建输出文件流
    std::ofstream file("image_direct.ppm");
    
    // Render

    file << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
        std::clog << "\r渲染进度: " << (image_height - j) << ' ' << std::flush;
        for (int i = 0; i < image_width; i++) {
            auto pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
            auto ray_direction = pixel_center - camera_center;
            ray r(camera_center, ray_direction);// 创建从相机中心到像素中心的光线

            color pixel_color = ray_color(r, world);// 获取光线颜色
            write_color(file, pixel_color);// 使用 write_color 函数将颜色写入文件
        }
    }
    
    file.close();
    std::clog << "\rDone.                 \n";
    std::cout << "PPM file generated successfully!\n";
    
    return 0;
}
```

## 抗锯齿技术详解

### 抗锯齿的工作原理

抗锯齿在光线追踪中通过**随机超采样的蒙特卡洛方法**实现：

1. **多重采样**：在每个像素内部随机抖动采样多条光线
2. **颜色累积**：将所有采样光线的颜色求和
3. **平均化处理**：除以采样数量得到最终像素颜色

```cpp
// 核心抗锯齿代码
for (int sample = 0; sample < samples_per_pixel; sample++) {
    ray r = get_ray(i, j);  // 获取带随机偏移的光线
    pixel_color += ray_color(r, world);  // 累积颜色
}
write_color(file, pixel_samples_scale * pixel_color);  // 平均化输出
```

### 随机采样实现

```cpp
vec3 sample_square() const {
    // 返回[-0.5, 0.5]²单位正方形内的随机点
    // 盒式滤波：将随机样本限定在以像素中心为中心、边长为1的正方形内
    return vec3(random_double() - 0.5, random_double() - 0.5, 0);
}

ray get_ray(int i, int j) const {
    auto offset = sample_square();  // 随机偏移
    auto pixel_sample = pixel00_loc 
                      + ((i + offset.x()) * pixel_delta_u)
                      + ((j + offset.y()) * pixel_delta_v);
    
    return ray(center, pixel_sample - center);
}
```

### 抗锯齿原理

- **系统性误差转化**：将锯齿的系统性误差转换为可收敛的噪声
- **采样数量影响**：采样越多，噪声越低，边缘越平滑
- **滤波类型**：当前实现为均匀盒式滤波（box filter）

### 改进建议：分层采样

为了在相同采样数下获得更好的效果，可以使用分层采样：

```cpp
// 分层采样实现
vec3 sample_square_stratified(int sample_index, int spp) const {
    int n = int(std::ceil(std::sqrt(double(spp))));
    int sx = sample_index % n;
    int sy = sample_index / n;
    double jx = (sx + random_double()) / n;  // [0,1)
    double jy = (sy + random_double()) / n;  // [0,1)
    return vec3(jx - 0.5, jy - 0.5, 0);
}
```

## 全局光照与"阴影"现象

### 观察到的现象

在光线追踪渲染中，即使没有明确的阴影计算，某些区域仍会呈现出"阴影"效果。这是**全局光照**的自然结果。

### 物理原理解释

#### 1. 间接光照的衰减
```cpp
// 漫反射材质中的能量衰减
color attenuation = albedo;  // 材质吸收部分光线
return attenuation * ray_color(scattered, world, depth-1);
```

每次光线与表面交互都会损失能量，深度越深的区域光强度越弱。

#### 2. 几何遮挡效应
- **直接可见表面**：摄像机 → 表面（1次反射）
- **凹陷区域**：摄像机 → 表面1 → 表面2 → ... → 凹陷表面（多次反射）

#### 3. 环境光遮蔽（Ambient Occlusion）
被其他几何体包围的区域接收到的环境光较少，自然显得更暗。

### 全局光照的数学模型

```cpp
color ray_color(const ray& r, int depth, const hittable& world) const {
    if (depth <= 0) return color(0,0,0);  // 递归深度限制
    
    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        ray scattered;
        color attenuation;
        if (rec.mat->scatter(r, rec, attenuation, scattered)) {
            // 递归计算间接光照，每次反射都有能量衰减
            return attenuation * ray_color(scattered, world, depth-1);
        }
        return color(0,0,0);
    }
    
    // 背景/天空光作为最终光源
    return background_color;
}
```

### 物理现象的成因

1. **朗伯反射定律**：漫反射表面按余弦分布散射光线
2. **能量守恒**：每次反射都会损失能量（attenuation < 1.0）  
3. **路径积分**：最终像素颜色是所有光线路径贡献的积分

### 与传统阴影的区别

| 类型 | 传统阴影 | 全局光照"阴影" |
|------|----------|----------------|
| **成因** | 直接光源被遮挡 | 间接光衰减 |
| **边界** | 明确的明暗分界 | 连续渐变 |
| **计算** | 阴影射线检测 | 多次反射累积 |

## 深度可视化技术

### 深度可视化的意义

深度可视化可以帮助理解：
- 光线在场景中的传播路径
- 不同区域需要的反射次数
- 全局光照的复杂性分布

### 实现方法

```cpp
color ray_color_depth_vis(const ray& r, int depth, const hittable& world) const {
    if (depth <= 0)
        return color(1, 0, 0);  // 红色表示达到最大深度

    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        // 用反弹次数计算深度可视化
        int bounce_count = max_depth - depth;
        double depth_ratio = double(bounce_count) / double(max_depth);
        
        // 深度颜色映射
        color depth_color = depth_visualization(depth_ratio);
        
        // 继续递归
        vec3 direction = random_on_hemisphere(rec.normal);
        ray_color_depth_vis(ray(rec.p, direction), depth-1, world);
        
        return depth_color;  // 返回深度可视化颜色
    }

    return color(0, 0, 1);  // 蓝色背景表示直接击中背景
}
```

### 深度颜色映射策略

```cpp
// 彩虹色谱映射
color depth_visualization(double bounce_ratio) const {
    if (bounce_ratio < 0.2) {
        // 初始击中：白色到黄色
        double t = bounce_ratio / 0.2;
        return (1.0-t) * color(1, 1, 1) + t * color(1, 1, 0);
    } else if (bounce_ratio < 0.4) {
        // 黄色到绿色  
        double t = (bounce_ratio - 0.2) / 0.2;
        return (1.0-t) * color(1, 1, 0) + t * color(0, 1, 0);
    } else if (bounce_ratio < 0.6) {
        // 绿色到青色
        double t = (bounce_ratio - 0.4) / 0.2;
        return (1.0-t) * color(0, 1, 0) + t * color(0, 1, 1);
    } else if (bounce_ratio < 0.8) {
        // 青色到蓝色
        double t = (bounce_ratio - 0.6) / 0.2;
        return (1.0-t) * color(0, 1, 1) + t * color(0, 0, 1);
    } else {
        // 蓝色到红色（深层反弹）
        double t = (bounce_ratio - 0.8) / 0.2;
        return (1.0-t) * color(0, 0, 1) + t * color(1, 0, 0);
    }
}
```

### 为什么简单几何体需要多次反弹？

即使是简单的球体，也存在需要多次光线反弹才能被"照亮"的区域：

1. **凹陷区域**：球体底部需要天空→球体表面→底部的间接光照
2. **相互遮挡**：多个物体间的接触区域  
3. **环境光遮蔽**：被其他几何体"包围"的区域接收环境光较少

### 验证实验

```cpp
// 观察不同深度的贡献
color ray_color_debug(const ray& r, int depth, const hittable& world) const {
    if (depth <= 0)
        return color(0.1, 0.1, 0.1);  // 给最大深度一点灰色

    hit_record rec;
    if (world.hit(r, interval(0.001, infinity), rec)) {
        vec3 direction = random_on_hemisphere(rec.normal);
        
        // 不同深度使用不同的衰减系数
        double attenuation = 0.7;  // 可以调整：0.3, 0.5, 0.9
        
        color bounce_color = ray_color_debug(ray(rec.p, direction), depth-1, world);
        
        // 输出调试信息
        if (depth == max_depth) {
            std::clog << "First hit\n";
        } else if (depth == max_depth - 1) {
            std::clog << "Second bounce\n";
        }
        
        return attenuation * bounce_color;
    }

    return background_color;
}