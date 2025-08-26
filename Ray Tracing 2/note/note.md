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
```
## 对于核心逻辑代码部分(ray_color)的问题思考与解答

**具体函数逻辑已写为注释在原代码处**

---

### 1. 光线弹射的最终返回颜色是什么？

看关键的递归函数：

```cpp
color ray_color(const ray& r, int depth, const hittable& world) const {
    if (depth <= 0)
        return color(0,0,0); // 黑色

    hit_record rec;

    if (world.hit(r, interval(0.001, infinity), rec)) {
        vec3 direction = random_on_hemisphere(rec.normal);
        return 0.5 * ray_color(ray(rec.p, direction), depth-1, world);
    }

    // 背景色
    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
}
```

逻辑是：

* **如果光线和物体相交**：

  * 随机生成一个散射方向 `random_on_hemisphere(rec.normal)`。
  * 递归计算这个新光线的颜色，并乘以 0.5（模拟能量损失）。
* **如果光线没有和任何物体相交**：

  * 返回一个渐变背景色（天空从白到蓝的渐变）。
* **递归终止条件**：当 `depth <= 0` 时返回黑色 `(0,0,0)`。

最终的颜色是：

* 从相机射出光线 → 多次弹射 → 每次乘 0.5（能量衰减） →
* 到达背景色时返回天空颜色，或耗尽反弹次数时返回黑色 →
* 递归逐层返回并叠加衰减系数，得到最终像素颜色。

也就是说，这个颜色是**所有弹射路径贡献的加权和**。

---

### 2. 如何防止弹射后的光线无限传播？

有两个机制在控制光线不会无限传播：

### **机制 1：最大递归深度 `max_depth`**

* `render` 中调用：

  ```cpp
  pixel_color += ray_color(r, max_depth, world);
  ```
* `ray_color` 中：

  ```cpp
  if (depth <= 0)
      return color(0,0,0);
  ```
* 每次递归调用时 `depth-1`，直到变成 0，直接返回黑色 → 终止。

这相当于**硬性限制反弹次数**。

---

### **机制 2：每次反弹能量衰减**

* 每次反弹都会：

  ```cpp
  return 0.5 * ray_color(...);
  ```
* 这样能量指数级减少，就算有无限深度，贡献也会趋近于 0。
* 在更复杂的路径追踪里，还会用 **Russian Roulette（俄罗斯轮盘）** 随机终止路径，提高效率。

---

### 总结回答

1. **返回的颜色**：每条光线路径弹射到背景色或耗尽深度后，返回的颜色会沿着递归路径衰减叠加，得到最终像素颜色。
2. **防止无限传播**：靠 `max_depth` 限制递归次数（硬终止）+ 每次反弹乘衰减系数（软衰减），避免路径无止境计算。
3. **环境光照模型**：这里的光照模型即为环境光照即为绘制出的背景，理论上这里的光线追踪的技术实现为路径追踪，背景为像素颜色提供者，也即环境光，从摄像机发出的光线依照光路可逆的原则追踪每个像素应该最终显示出来的颜色。

## 循环引用问题
* 在文档语境中，这指的是在代码设计中，hittable（可命中物体）和material（材质）这两个类需要相互引用，形成了循环依赖的情况。
* 例如，hit_record（命中记录）需要包含指向material的指针，而material的scatter方法又需要接收hit_record作为参数。
* **这种相互引用的关系就是所谓的 “引用循环性”。**

* 为了解决这种循环引用问题，在 C++ 中通常会使用前向声明（如class material;），告知编译器material是一个类，后续会有定义，从而避免编译错误。这种处理方式允许两个类在代码中相互引用，同时保证编译过程的正常进行。

## 实际物理情况的颜色反射

### 疑问：
我对这段程序有一些比较好奇的疑问，当return attenuation*...前乘的是一个数的时候，该材质本身不会显示什么颜色，但当乘上一个三维向量后物体就会表现出某种颜色，这对应实际物理情况的什么现象呢？
```cpp
    color ray_color(const ray& r, int depth, const hittable& world) const {
        // If we've exceeded the ray bounce limit, no more light is gathered.
        // 如果深度小于等于0，表示光线已经经过了最大次数的反弹，此时返回黑色
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;

        // 计算光线与场景的交点
        if (world.hit(r, interval(0.001, infinity), rec)) {
            // 如果光线与物体相交，设置交点的法向量
            ray scattered;// 反弹光线
            color attenuation;// 反弹光线衰减系数
            if (rec.mat->scatter(r, rec, attenuation, scattered))// 如果材质有散射光线
            // 这里前方点乘的attenuation为main函数中设置的材质反射率，实际也表现为材质的颜色
                return attenuation * ray_color(scattered, depth-1, world);// 光线递归进入下一层，方向变为材质约定的方向
            return color(0,0,0);// 如果材质没有散射光线，则返回黑色
            // depth-1 表示光线已经反弹了一次，进入下一层的递归
            // 最终像素颜色是所有光线路径贡献的积分
        }

        // 背景色渐变
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
```

### 回答
* 短答：attenuation（三通道向量）表示材质对不同波长（在实现上用 RGB 通道近似）的反射/吸收率（即 albedo）。用标量只改变亮度（无色灰度），用向量则对每个颜色通道分别衰减，因而产生颜色。这在物理上对应“波长相关的反射/吸收”（表面色）和光在表面/内部被选择性吸收或散射的现象。

* 要点与类比

* 标量 attenuation（例如 0.8）相当于灰色滤光片：所有波长都按相同比例被反射，所以只变亮暗，不变色。
* 向量 attenuation（例如 (0.8,0.2,0.2)）相当于有色滤光片：红光比绿/蓝更多被反射 => 物体看起来偏红。
* 物理对应：真实物质的反射率随波长变化（光谱反射率）；光被表面或体内吸收/散射，剩余部分被观测到。
* 更精确的物理模型还涉及方向依赖的 BRDF、菲涅尔效应、金属与介质的不同响应等。RGB 是对连续光谱的粗糙近似。  

- 代码示例（说明差别）
```cpp
// 示例：无色（标量）——每通道相同
color attenuation = color(0.8, 0.8, 0.8); // 只是变暗，不带色偏

// 示例：有色（向量）——不同通道不同衰减
color attenuation = color(0.8, 0.2, 0.2); // 更多红被反射，表现为红色材质
```

- 常见 gotchas

- 确保衰减值在 [0,1] 内以近似能量守恒（否则可能人为增亮）。
- RGB 只是近似；要真实模仿复杂材料需用光谱追踪或更复杂 BRDF。
- 多次反弹时不断乘以 attenuation，会迅速变暗（这就是为什么要限制递归深度并gamma 校正）。

## 直觉矛盾：全反射与现实世界中的透明介质混淆

### 疑问
- 全反射是只有在折射率大的介质到折射率低的介质中发生，但是为什么我们现实中的玻璃等介质，在空气中常常能够同时看到折射与反射？

### 回答
---

#### 1. 全反射和普通反射是两回事

* **全反射（Total Internal Reflection, TIR）**

  * 条件：光线从 **折射率高** → **折射率低** 的介质传播，且入射角大于**临界角**
  * 一旦发生，**没有任何折射光线**，能量全都反回去
  * 典型例子：水下看水面的闪亮反射、光纤传输
  * 公式（临界角）：

    $$
    \theta_c = \arcsin\left(\frac{n_2}{n_1}\right)
    $$

    这里 $n_1 > n_2$

* **普通反射（Partial Reflection）**

  * 任意两种不同折射率的介质交界面，**几乎所有入射角都会有部分光被反射**
  * 反射比例由 **Fresnel 方程** 给出，不是 0，也不是 1（除非特殊角度）
  * 就算是玻璃（n≈1.5）到空气（n≈1.0），即使垂直入射，也会有大约 **4%** 光反射，剩下的 96% 折射进入空气

    $$
    R = \left(\frac{n_1 - n_2}{n_1 + n_2}\right)^2 \approx \left(\frac{1.5 - 1.0}{1.5 + 1.0}\right)^2 \approx 0.04
    $$

---

#### 2. 为什么玻璃在空气中能“同时”看到反射和折射？

* 因为在绝大多数情况下，**反射和折射是同时发生的**
* 在玻璃表面，Fresnel 方程会分配能量：

  * 一部分光被反射回来（比如窗户上的倒影）
  * 另一部分光折射进入玻璃（让你看到玻璃后面的物体）
* 只有当**满足全反射条件**时，折射通道才会彻底消失，反射率才变成 100%。

---

#### 3. 为什么我们容易把它们混淆

* 学习全反射时，课本通常用\*\*“玻璃 → 空气”\*\*举例，所以你会下意识认为玻璃在空气里要么反射，要么折射。
* 但实际上，**玻璃在空气中大多数时候是部分反射 + 部分折射共存**
* 全反射只是其中一个极端情况（大入射角时）。

---

#### 4. 对路径追踪的意义

* 在路径追踪里，模拟玻璃材质时：

  * 需要用 **Fresnel 方程**（常用 Schlick 近似）随机决定光线是走反射还是折射路径
  * 当计算得到**全反射条件**时，直接选择反射方向
* 这样才能既看到玻璃上的高光反射，又看到玻璃后面的折射成像。

### 疑问
- 那么施利克近似是什么原因？以陡峭的角度看一扇窗户，它就变成了一面镜子，但是在空气中不是不会发生全反射吗？

### 回答
- **真实物理中，反射率（注意不是折射率）随着入射角的角度而变化，与全反射不为同一种物理现象**

#### 1. 施利克近似（Schlick Approximation）是干什么的

在真实物理中，**Fresnel 方程**给出了反射率 $R(\theta)$ 随入射角变化的精确公式，但它比较复杂，包含正弦、余弦和平方根。
Schlick 提出了一个非常简洁的近似公式：

$$
R(\theta) \approx R_0 + (1 - R_0)(1 - \cos\theta)^5
$$

其中：

* $R_0 = \left(\frac{n_1 - n_2}{n_1 + n_2}\right)^2$（垂直入射时的反射率）
* $\theta$ 是入射角（相对于法线）

这个公式有两个关键特性：

1. 当入射角增大时，反射率急剧增加（尤其接近掠射角时）
2. 简单到路径追踪可以轻松用它随机决定“反射”还是“折射”

---

#### 2. 玻璃在空气中“变镜子”的原因

即使 **n\_1=1.5（玻璃），n\_2=1.0（空气）** 时不满足全反射条件，Fresnel 效果依然会让反射率接近 1——特别是**掠射角**（光线几乎沿着玻璃表面走）时。

举个例子：

* 垂直看玻璃（$\theta = 0°$）：
  $R_0 \approx 4\%$，你能看到大部分透过的景象，只有一点反射
* 斜着看（$\theta = 80°$）：
  Fresnel 方程告诉我们反射率可能达到 **80%以上**
* 极端掠射角（接近 $90°$）：
  反射率几乎变成 **100%**，玻璃表现得像一面镜子

这就是为什么你在陡峭角度看窗户时，几乎只能看到倒影，看不到里面的东西——并不是因为全反射，而是**反射分走了几乎所有的能量**。

---

#### 3. 为什么不是全反射

* **全反射**：入射角大于临界角 → 折射消失，反射率精确等于 1
* **Fresnel 高反射**：入射角接近 90° 时，反射率趋近于 1，但仍然有极小的折射部分（只是肉眼难察觉）
* 对玻璃–空气来说，临界角公式：

  $$
  \theta_c = \arcsin\left(\frac{n_2}{n_1}\right) \approx 41.8°
  $$

  这个是 **从玻璃内部往外** 时才生效，从空气看玻璃不会触发全反射。

---

#### 4. 在路径追踪里的表现

当我们用 Schlick 近似时：

* 从空气看玻璃：高入射角 → 高反射率 → 光线大概率选择反射方向
* 从玻璃内部看空气：如果角度超过临界角 → 强制全反射，否则用 Schlick 近似决定

这样就既能表现窗户高角度时的“镜子效果”，也能在玻璃内部正确显示全反射。

### 疑问
- 那么反射率与折射率是否都为介质本身的性质？

### 回答
折射率是介质的本征性质，但反射率不是固定的“介质常量”。

---

### 1. 折射率 $n$

* 是材料在某一波长（颜色）下的光学性质，和介质的组成、密度有关
* 在给定波长下，折射率就是一个固定值（例如常温下普通玻璃在可见光 $n \approx 1.5$）
* 会随波长变化 → 这就是色散（彩虹边缘）
* 你可以认为它是**介质的本征属性**

---

### 2. 反射率 $R$

* **不是**单一的材料常数
* 它取决于：

  1. **介质两边的折射率 $n_1, n_2$**
  2. **入射角 $\theta$**
  3. **光的偏振状态**
  4. **波长**（间接由折射率的色散引起）
* 用 Fresnel 方程可以精确算出：

  $$
  R(\theta) = f(n_1, n_2, \theta, \text{polarization})
  $$
* 所以同一种玻璃：

  * 垂直看：反射率可能只有 4%
  * 斜着看：能飙到 80%+
  * 不同波长下，这两个数字还会变化

---

### 3. 总结一句

* **折射率 $n$** → 材料固有的
* **反射率 $R$** → 由两个材料的折射率 + 入射角 + 偏振 + 波长共同决定，不是单纯的介质常量