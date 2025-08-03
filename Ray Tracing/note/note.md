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