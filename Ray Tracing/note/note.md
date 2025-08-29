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

## BVH
- **BVH算法核心为树，如果没有击中父节点，那么肯定也不会击中子节点，所以这也是BVH会加速运算的原因**

## 关于BVH与多态

### 问：
我正在学习光线追踪的BVH相关技术，但看完教程后总感觉有些混乱，我想让你帮我梳理一下思路。 现在我理解的是在hittable的派生类中构造对象时按照自己的大小构造了相关的包围盒，然后逐层将其融合起来形成一棵树，但是其中的逻辑我还是有些混乱，请你清晰得按照该代码为我梳理一下整个流程

### 答：
太好了，你已经走到 **BVH (Bounding Volume Hierarchy)** 这一步了，这是光线追踪加速的关键。你给的 `bvh_node` 实现就是 Peter Shirley *Ray Tracing: The Next Week* 中的经典写法。我来按照代码的逻辑，从整体到细节，给你**逐层清晰梳理**一下。

---

# 📌 1. BVH 的核心思想

* 在场景中，物体数量可能成百上千，若每条光线都对每个物体做求交检测，开销是 $O(N)$。
* BVH 的思路：

  1. 给物体包上简单的 **AABB 包围盒**。
  2. 将物体递归地划分成左右两组，每一组再构造自己的包围盒。
  3. 最终形成一棵树（每个节点有一个 AABB，叶子节点是具体物体）。
* 光线求交时：

  * 先检查光线是否与 **节点的 AABB 相交**；如果不相交，直接跳过整个子树；
  * 若相交，再进入子节点检测，直到叶子节点，再做具体物体的求交。
    👉 这样复杂度从 $O(N)$ 降到 $O(\log N)$ 左右。

---

# 📌 2. 构造 BVH 的流程（对应构造函数代码）

你贴的 `bvh_node` 的逻辑是这样的：

### （1）计算当前范围内的包围盒

```cpp
bbox = aabb::empty;
for (size_t object_index=start; object_index < end; object_index++)
    bbox = aabb(bbox, objects[object_index]->bounding_box());
```

* 遍历 `[start, end)` 范围内的所有物体，得到它们的 AABB。
* 然后用 `aabb(bbox, obj_bbox)` 合并起来，形成当前节点的总包围盒。
  👉 这个 `bbox` 就是**当前 BVH 节点的空间范围**。

---

### （2）选择分割轴

```cpp
int axis = bbox.longest_axis();
```

* 找到当前 `bbox` 在 x/y/z 中 **最长的轴**（例如 x）。
* 在这个轴上，把物体排序，这样可以尽量把物体分布均匀划分成左右两组。

---

### （3）判断物体数量（递归出口）

```cpp
size_t object_span = end - start;

if (object_span == 1) {
    left = right = objects[start];
} else if (object_span == 2) {
    left = objects[start];
    right = objects[start+1];
}
```

* 如果只有一个物体：左右子树都指向它。
* 如果有两个物体：左边一个，右边一个。
  👉 这就是 **递归的出口条件**。

---

### （4）递归划分

```cpp
else {
    std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);

    auto mid = start + object_span/2;
    left = make_shared<bvh_node>(objects, start, mid);
    right = make_shared<bvh_node>(objects, mid, end);
}
```

* 否则：先在选定的轴上排序，然后从中间位置 `mid` 一分为二。
* 递归调用 `bvh_node` 构造子树。
  👉 直到最后所有叶子节点都是具体的 `hittable` 对象。

---

# 📌 3. 光线求交流程（`hit` 函数）

```cpp
bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    if (!bbox.hit(r, ray_t))
        return false;   // 光线没击中这个包围盒 → 整个子树跳过

    bool hit_left = left->hit(r, ray_t, rec);
    bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

    return hit_left || hit_right;
}
```

### 步骤解释：

1. **先测当前节点的包围盒**：

   * 如果光线没击中，整棵子树直接剪枝。

2. **递归检查左子树**：

   * 如果击中，`rec.t` 会被更新为最近的交点距离。

3. **检查右子树**：

   * 如果左边已经有交点，那么右边的 `ray_t.max` 只取到左边交点的距离（这样右边如果更远的交点就不会被考虑，保证返回最近交点）。

4. **返回是否击中任意一个子树**。

👉 这个逻辑确保了：

* 剪枝：大幅减少求交次数。
* 最近优先：保证始终找到最近的交点。

---

# 📌 4. 总结：整个 BVH 流程（从对象到渲染）

1. **每个 hittable 对象**（球体、三角形等）自己能返回一个包围盒。
2. **构造 BVH**：

   * 从整个对象集合开始；
   * 算出总包围盒 → 找最长轴 → 排序 → 分裂 → 递归；
   * 最终形成一棵树，每个节点一个 `bbox`，叶子节点是物体本身。
3. **渲染时**：

   * 发射光线 → 从根节点 `hit` 开始；
   * 先检测 AABB → 若相交再递归检测子节点；
   * 直到叶子节点，才真正和物体做求交。
     👉 整个过程相当于：**用一棵树组织物体，快速剪枝无效的求交检测**。

---

💡 可以这样直观理解：

* **构建 BVH** = “把一堆物体装进一棵递归的分区盒子树里”
* **光线求交** = “先问大盒子：光线打中了吗？如果打中再拆开小盒子继续问，直到问到具体物体”。

---

要不要我给你画一个 **BVH 构造和光线求交的流程图**，这样比文字更直观？

### 问：
std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);是如何作用的？为什么传入comparator就能按照制定轴排列，而且mid是如何获取到哪个才是中间的物体？

### 答：
这个问题问得非常好 👍，其实你已经抓到 **BVH 核心的关键点**了：物体怎么按照某个轴来排序，以及中间位置的划分是怎么起作用的。我们分开来看：

---

## 📌 1. `std::sort` 的工作机制

```cpp
std::sort(std::begin(objects) + start, 
          std::begin(objects) + end, 
          comparator);
```

* `std::sort` 是 C++ STL 的排序函数，第三个参数 `comparator` 是一个 **比较函数**。
* 当 `std::sort` 在排序两个元素 `a` 和 `b` 时，它会调用：

  ```cpp
  comparator(a, b)
  ```

  如果返回 `true`，则说明 `a` 应该排在 `b` 前面。

👉 这样，传入不同的 `comparator` 就可以改变排序规则。

---

## 📌 2. comparator 的定义（按照指定轴）

在你的代码里：

```cpp
static bool box_compare(
    const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index
) {
    auto a_axis_interval = a->bounding_box().axis_interval(axis_index);
    auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
    return a_axis_interval.min < b_axis_interval.min;
}
```

这里逻辑是：

1. 每个物体都有一个 `bounding_box()`，返回它的 AABB。
2. `axis_interval(axis_index)` 会取出包围盒在某个坐标轴上的范围（比如 X 轴上的 `[x_min, x_max]`）。
3. `a_axis_interval.min < b_axis_interval.min` → 就是比较两个物体在该轴上的**起始位置**，小的排前面。

然后在构造函数里根据 `axis` 选择比较函数：

```cpp
auto comparator = (axis == 0) ? box_x_compare
                : (axis == 1) ? box_y_compare
                : box_z_compare;
```

* 如果最长轴是 X，就用 `box_x_compare`；
* 如果最长轴是 Y，就用 `box_y_compare`；
* 如果最长轴是 Z，就用 `box_z_compare`。

👉 这样，物体就会按照 **在该轴上的最小坐标** 排序。

---

## 📌 3. 中间元素的获取

```cpp
auto mid = start + object_span/2;
```

* `object_span = end - start` 表示当前范围内物体的数量。
* 除以 2：把范围平均分成两半。
* 例如：

  * 物体下标 `[start=0, end=8)` → `object_span = 8` → `mid = 0 + 4 = 4`。
  * 左边 `[0,4)` → 4 个物体，右边 `[4,8)` → 4 个物体。

👉 由于前一步已经 **按照某个轴排序过**，所以 `mid` 的位置就是“在该轴方向上大致的中点”。这样划分后：

* 左子树的物体大致在该轴的左边；
* 右子树的物体大致在该轴的右边。

---

## 📌 4. 为什么这样划分有意义？

* 如果不排序，直接随便分一半，可能会出现左边一个大物体覆盖整个空间，右边一堆小物体 → 导致包围盒大量重叠，效率下降。
* 排序后，物体在最长轴方向上尽量 **均匀分布**，左右两边的包围盒也更紧凑。
  👉 这能减少光线遍历时的包围盒重叠，从而减少不必要的递归。

---

✅ 总结一下：

* **`comparator`**：告诉 `std::sort` 如何比较两个物体 → 按照某个轴上 AABB 的最小值排序。
* **`mid`**：因为物体已经排序过，所以取数组的中点就是在该轴方向上的“几何中点”，自然能把空间划分成两半。

---

要不要我给你画一个小示例（比如 6 个物体在 X 轴上排布，排序+取 `mid` 如何把它们分成两组）？这样你会更直观地看到 BVH 的划分过程。

### 问：
我这里还是不太理解渲染时的流程，光线是在哪一阶段判断是否与包围盒相交的？

### 答：
👌 你问到的这个就是 **BVH 的运行时流程**（和构建时排序不同），我来给你梳理一遍。

我们分成两个阶段理解：

---

## 📌 阶段 1：离线构建 BVH 树

这一部分就是你之前的代码：

1. 把物体按某个轴排序；
2. 取 `mid` 划分成左右子树；
3. 每个子树继续递归构造，直到只剩下一个或两个物体；
4. 每个节点存储一个包围盒（`aabb`），表示“这个节点下所有物体的空间范围”。

👉 最终我们得到一棵树，根节点的 `bbox` 包含整个场景，叶子节点是具体的物体。

---

## 📌 阶段 2：渲染时的光线遍历

这时，**每一条光线发射时，都会走一遍 BVH 递归 hit 流程**。
看你的代码里的关键函数：

```cpp
bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    if (!bbox.hit(r, ray_t))
        return false;   // 🚨 如果光线连当前节点的包围盒都打不中，直接返回 false

    bool hit_left = left->hit(r, ray_t, rec);
    bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

    return hit_left || hit_right;
}
```

### 流程拆解：

1. **光线 vs 当前节点的包围盒**

   ```cpp
   if (!bbox.hit(r, ray_t)) return false;
   ```

   * 如果光线没打中这个节点的包围盒 → 整个子树都不用看了，直接跳过。
   * 这就是 BVH 加速的关键：跳过大量不可能命中的物体。

2. **递归进入左右子树**

   ```cpp
   bool hit_left = left->hit(r, ray_t, rec);
   bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);
   ```

   * 如果命中左子树，就会更新 `rec.t`（最近的交点）。
   * 在检查右子树时，`ray_t.max` 变成了左子树的最近交点 `rec.t`，意味着：

     * 如果右子树里有更远的交点，就不再关心了，因为左子树已经挡住了光线。

3. **最终返回 true/false**

   * 如果左右子树都 miss → false。
   * 如果有一边 hit → true，并且 `rec` 记录最近的交点信息。

---

## 📌 总结

* **包围盒相交测试** 发生在 **每次递归调用节点 `hit` 时**，即在访问节点前先测试它的 `bbox`。
* 如果光线没打中这个 `bbox`，整个子树（成百上千个物体）就直接跳过，不再计算。
* 如果打中了，才会深入到更小的子包围盒，直到叶子节点，最后才测试具体的三角形/球体等物体。

👉 **BVH 的价值就在这里：**

* 假设场景有 10000 个物体：

  * 没有 BVH → 每条光线都要和 10000 个物体做相交测试。
  * 有了 BVH → 光线可能只需要和几十个 `bbox` + 物体测试，大大加速。

---

要不要我给你画一个 **光线遍历 BVH 的递归流程图**？（从根节点 → 测 bbox → 左右子树 → 叶子节点）这样你能直观地看到光线是怎么一步步裁剪搜索空间的。

### 问：
不不，我已经了解了这个逻辑。我刚刚研究了一下，发现我是在奇怪这里的光线调用是如何工作的，bvh继承自hittable类，同样物体也继承自hittable类，还有hittable_list也继承自hittable类，这些类中都有hit函数的重载，我是还没搞清楚这里面的运作关系，我现在大概了解了这其中运用了大量的运行时多态，以下是核心代码以及相关我的理解注释，请你帮我看看有没有问题

### 答：
你总结得已经很到位了 👍，确实核心就是 **运行时多态 + BVH 替换 world**。我帮你逐点梳理一下，看哪些地方可以更精准：

---

### 1. `hittable` 的角色

* `hittable` 是一个抽象基类，里面定义了虚函数 `hit(...)` 和 `bounding_box(...)`。
* 所有物体（`sphere`、`octahedron`）、容器（`hittable_list`）、加速结构（`bvh_node`）都继承自 `hittable`。
* **多态意义**：渲染器只关心 "这是一个 hittable"，不用关心它具体是球体、列表还是 BVH 节点，调用 `hit(...)` 时会动态分派到正确的实现。

---

### 2. `ray_color` 的调用链

你在注释里写的逻辑基本正确，我稍微精炼一下调用路径：

1. **顶层调用**

   ```cpp
   if (world.hit(r, interval(0.001, infinity), rec)) { ... }
   ```

   * 这里的 `world` 最终是一个 `bvh_node`（因为最后你 `world = hittable_list(make_shared<bvh_node>(world));`）。
   * 所以实际调用的是 `bvh_node::hit(...)`。

2. **BVH 节点 hit**

   * 检查光线是否打到当前节点的 `bbox`。
   * 如果命中，递归调用 `left->hit(...)` 和 `right->hit(...)`。
   * `left` 和 `right` 可能是另一个 `bvh_node`，也可能是最终的 `sphere` 或 `octahedron`。

3. **物体 hit**

   * 如果走到叶子节点（具体几何体），就进入具体物体的 `hit` 计算：解方程，算交点、法线等。
   * 命中后，更新 `hit_record`，用于后续材质计算。

4. **材质 scatter**

   * 命中后由材质的 `scatter` 决定如何生成新的反射 / 折射光线。
   * `ray_color` 递归继续追踪。

---

### 3. 你的注释中几个关键点的修正

* ✅ **正确**

  > “BVH 在该项目的代码逻辑中核心作用是在这里，简化了光线与物体的相交测试”
  > 没错，构建好 BVH 后，`world.hit()` 就变成了 BVH 的递归查找，而不是 `hittable_list` 的 O(N) 线性遍历。

* ⚠️ **小调整**

  > “先进入 hittable\_list 的 hit 函数，在 hittable\_list 的 hit 函数中递归进入 BVH 节点的 hit 函数”
  > 这里不完全对：

  * **构建 BVH 前**：`world` 是 `hittable_list`，`hit` 会 O(N) 遍历所有物体。
  * **构建 BVH 后**：你写的 `world = hittable_list(make_shared<bvh_node>(world));` 实际上是把整个 `world` 包进一个 `hittable_list`，里面只有一个元素：`bvh_node`。
    所以顶层 `world.hit()` 先进入 `hittable_list::hit()`，它会调用唯一元素 `bvh_node::hit()`，然后才进入 BVH 的递归。

  👉 换句话说：最终还是通过 **BVH** 管理的，不会再直接进入 `sphere` 或 `octahedron`，而是先经过 BVH 的层级裁剪。

* ✅ **正确**

  > “最终像素颜色是所有光线路径贡献的积分”
  > 是的，递归 `ray_color` 就在做蒙特卡洛积分。

---

### 4. 关键思想总结

* `hittable` 作为接口类，保证了“统一调用方式”。
* `hittable_list` 和 `bvh_node` 都是 **组合** 关系（包含多个 hittable），但 `hit` 的实现策略不同：

  * `hittable_list`：遍历全部对象 → O(N)
  * `bvh_node`：递归裁剪 + 遍历少量对象 → O(logN)
* 在渲染时，`ray_color` 不需要知道场景是由什么组成的，它只管调用 `world.hit()`，多态帮你把请求派发到正确的逻辑里。

---

所以你的理解整体正确，只是关于 “hittable\_list 和 BVH 调用顺序” 那里稍微有点混淆：

* 在构建 BVH 之后，顶层 `world` 其实还是个 `hittable_list`，但它里面只剩一个 `bvh_node`。
* 所以运行时调用关系是：
  `ray_color → hittable_list::hit → bvh_node::hit → [递归到物体]`。

---

要不要我给你画一个 **类图 + 调用链流程图**（UML 风格）？这样你能清晰看到 `hittable`、`hittable_list`、`bvh_node`、`sphere` 之间的继承和调用关系。

### 问：
我还有一点小小的问题，例如以下是派生自hittable的sphere类，这里的构造函数中即声明了该物体的最小包围盒，但是编译器是怎么知道在BVH树中遍历到叶子节点时会进入调用具体物体的例如sphere的hit函数呢？

### 答：
啊，这个问题问得非常好 🎯 —— 你其实已经踩到 C++ **运行时多态 (runtime polymorphism)** 的核心点了：

> **“编译器是怎么知道 BVH 树遍历到叶子时，要调用具体物体 (比如 `sphere`) 的 `hit` 函数？”**

答案就是：**编译器并不知道，它是运行时由虚函数机制（vtable）决定的**。
我帮你完整拆开：

---

### 1. 基类的虚函数定义

在 `hittable` 中，`hit` 被声明为 **虚函数**：

```cpp
class hittable {
  public:
    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
    virtual aabb bounding_box() const = 0;
};
```

* `virtual` → 告诉编译器，这个函数需要支持动态分派。
* `= 0` → 纯虚函数，`hittable` 变成抽象基类，不能直接实例化。

这样，每个派生类（`sphere`、`bvh_node`、`hittable_list`）都必须实现自己的 `hit`。

---

### 2. vtable（虚函数表）

当你写：

```cpp
sphere s(point3(0,0,0), 1.0, some_material);
shared_ptr<hittable> obj = make_shared<sphere>(s);
```

编译器在幕后会为 `sphere` 类生成一个 **虚函数表 (vtable)**，表里存放着指针，指向 `sphere::hit` 和 `sphere::bounding_box` 等函数的具体实现。

`obj` 作为 `shared_ptr<hittable>`，虽然它的静态类型是 `hittable`，但它里面记录了一个指向 `sphere` 对象的指针，并且持有“这个对象的 vtable 地址”。

所以当你写：

```cpp
obj->hit(r, t, rec);
```

编译器会在运行时：

1. 从对象里找到它的 vtable 指针；
2. 在 vtable 里找到对应的 `hit` 实现；
3. 跳转到 `sphere::hit` 去执行。

这就是 **多态的动态绑定**。

---

### 3. 回到 BVH 的上下文

在 BVH 里：

```cpp
bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
    if (!bbox.hit(r, ray_t))
        return false;

    bool hit_left = left->hit(r, ray_t, rec);
    bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

    return hit_left || hit_right;
}
```

* `left` 和 `right` 都是 `shared_ptr<hittable>`。
* 如果它们指向的是 `sphere`，那么 `left->hit(...)` 就会通过 vtable 跳到 `sphere::hit`。
* 如果它们指向的是另一个 `bvh_node`，那么就会进入 `bvh_node::hit`，继续递归。

**BVH 根本不需要知道“叶子节点是球还是八面体”**，它只知道“我手里拿的是一个 `hittable`，它有 `hit` 方法”。

---

### 4. 总结一下

* **编译期**：编译器只知道调用的是 `hittable::hit`，因为静态类型是 `hittable`。
* **运行期**：通过 vtable，程序才知道具体的对象是 `sphere`，于是调用 `sphere::hit`。

👉 所以答案就是：
**进入 `sphere::hit` 完全依赖于 C++ 的运行时多态机制，而不是编译器“提前知道”**。

## Perlin噪声中三线性插值


### 1. 插值的目的

噪声函数 `noise(p)` 需要返回 **点 `p` 在单位立方体中的一个平滑数值**。
由于我们只在立方体的 8 个角（顶点）预先存储了随机值 `c[2][2][2]`，那么如果 `p` 在立方体内部（比如 `(0.3, 0.7, 0.2)`），我们就必须用插值的方法，把这 8 个顶点的值平滑“混合”起来。

所以 `trilinear_interp` 的工作就是：

> 根据点 `(u,v,w)` 在立方体中的相对位置，利用 8 个顶点的值 `c[i][j][k]`，计算一个平滑的插值。

---

### 2. 立方体的顶点索引

* `c[i][j][k]` 对应立方体的一个角：

  * `i ∈ {0,1}` → x 方向：左/右
  * `j ∈ {0,1}` → y 方向：下/上
  * `k ∈ {0,1}` → z 方向：前/后

这样 8 个 `(i,j,k)` 组合就覆盖了一个立方体的 8 个角。

---

### 3. 插值权重公式

代码里的核心：

```cpp
accum += (i*u + (1-i)*(1-u))
       * (j*v + (1-j)*(1-v))
       * (k*w + (1-k)*(1-w))
       * c[i][j][k];
```

👉 这是 **三维插值的权重计算公式**。

举个例子：

* 当 `i=0` → 权重是 `(1-u)`（离开 `x=0` 面越远，权重越小）。
* 当 `i=1` → 权重是 `u`（离开 `x=1` 面越远，权重越小）。

所以 `(i*u + (1-i)*(1-u))` 其实就是在 **x 方向上根据 u 插值**。
同理，

* `(j*v + (1-j)*(1-v))` → y 方向的插值。
* `(k*w + (1-k)*(1-w))` → z 方向的插值。

最后把三个方向的权重乘起来，就是点 `(u,v,w)` 对应角 `(i,j,k)` 的综合贡献。

---

### 4. 插值过程（逐层理解）

三线性插值可以拆解成三次 **线性插值**：

1. **X 方向**：
   对每一对 `x=0` 和 `x=1` 的顶点，插值得到 4 个值。

2. **Y 方向**：
   对这 4 个值再按 `v` 插值，得到 2 个值。

3. **Z 方向**：
   对最后 2 个值按 `w` 插值，得到最终结果。

代码里的三重循环 `(i,j,k)` 其实就是把这个分步骤的插值压缩成一个公式。

---

### 5. 举个例子

假设 `u=0.2, v=0.7, w=0.5`，则

* `(i*u + (1-i)*(1-u))` = `0.8` (当 i=0)，`0.2` (当 i=1)。
* `(j*v + (1-j)*(1-v))` = `0.3` (当 j=0)，`0.7` (当 j=1)。
* `(k*w + (1-k)*(1-w))` = `0.5` (当 k=0)，`0.5` (当 k=1)。

于是每个角的贡献就是这三个因子的乘积，再乘以该角的随机值 `c[i][j][k]`。
最后累加，就得到点 `(u,v,w)` 的平滑噪声值。

---

✅ 总结：

* 三线性插值的本质：**用点 `(u,v,w)` 在立方体内的位置，按比例混合 8 个角的值。**
* 代码中用三重循环计算所有 8 个角的贡献，并把它们加权求和。
* Hermite 曲线 `u=u*u*(3-2*u)` 之前已经把 `u,v,w` 平滑化，使得插值更加自然。

### 将顶点换成随机向量好处具体是怎么体现的？
## 1. 之前的做法（标量值 Noise）

在你给的 `perlin` 代码里，每个格点存的只是一个随机浮点数（`randfloat[i]`）。
然后在立方体内部做三线性插值。

问题是：

* 因为格点的随机值是独立的、无关联的，所以插值出来的曲面虽然连续，但在格点处必然是固定极值。
* 结果就是噪声 **容易在格点边界上出现“方块感”**：极大值和极小值总是对齐在整数坐标位置，看起来带有格子印记。

---

## 2. Ken Perlin 的改进（Gradient Noise）

Perlin 提出的 trick 是：

* 不再存随机浮点值，而是在每个格点存一个 **随机方向向量**（通常单位长度）。
* 然后对于立方体内部任意一点 `p`，**取 `p - lattice_point` 作为方向向量**，和该格点存的随机向量做点积。

即：

```math
value(p) = dot(random_gradient(lattice_point), p - lattice_point)
```

---

## 3. 改进的好处

### (1) 消除了格点对齐的极值

* 在原始标量版本中，每个格点值是随机浮点，插值时格点自然固定了极大/极小值。
* 在 gradient 版本里，格点处的值总是 `dot(g, 0) = 0`，所以格点本身没有固定的极值。
* 极值的位置被 **推动到单元格内部**，这样就避免了 “格点方块印记”，模式更自然。

---

### (2) 模式更平滑、更自然

* 由于值来源于 **向量和相对位置的点积**，相邻格点的值之间带有一定相关性，而不是完全独立。
* 这种相关性让结果曲面既连续又有平滑的梯度过渡，看起来更像自然界中的纹理（云、烟、木纹、大理石）。

---

### (3) 控制频谱特性

* 标量 noise 在频域里有强烈的格点对齐“谐波噪声”。
* Gradient noise 的频谱能量更均匀，格子印记大幅减弱。
* 这就是为什么 Perlin 噪声看上去“有机”，而 Value noise 看上去“马赛克方块感”。

---

## 4. 举个直观对比

* **Value Noise (随机标量 + 插值)** → 看起来像模糊后的棋盘，极值总在格点处。
* **Gradient Noise (随机向量 + 点积)** → 极值被挤到单元格中间，模式不再对齐格点，更加自然。

---

✅ 总结：
**用随机向量取代随机浮点数的核心好处是：**

1. 消除了格点处必然的极值 → 避免格子印记。
2. 通过点积引入相关性 → 结果更平滑自然。
3. 频域分布更均匀 → 更接近自然纹理。

这就是 Ken Perlin 当年那个“非常聪明的小技巧”，也是他拿图形学奥斯卡奖的原因之一 😄。