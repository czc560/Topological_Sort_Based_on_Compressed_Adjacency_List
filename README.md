# 基于紧缩邻接表的拓扑排序程序

## 描述

这是一个 C++ 程序，实现基于紧缩邻接表（compressed adjacency list）的图结构，并提供多种拓扑排序算法：DFS、Kahn（BFS）、并行 Kahn（在本编译环境中回退为顺序）、增量维护版本，以及字典序最小/最大变体。程序支持检测图中的环，并输出拓扑排序结果，并能生成 3D 坐标用于可视化/VR。

- **核心功能**：
  - 从普通邻接表构建紧缩邻接表（`h` 和 `list`）。
  - 获取节点的邻接区间。
  - DFS 和 Kahn 拓扑排序。
  - 文本和 JSON 输出格式。

- **项目结构**：
  - `topsort.cpp`：主程序，处理命令行参数、demo、可选 layout 输出。
  - `core/compressed_graph.*`：CSR + Varint 压缩图结构，线程安全只读接口。
  - `core/toposort.*`：`TopoSortSolver` 抽象类及 5 种派生算法（DFS、Kahn、并行 Kahn、增量、字典序）。
  - `core/layout.*`：拓扑层级到 3D 坐标的转换。
  - `core/demos.*`：4 个业务示例类（CourseScheduler / TaskDependencyManager / PackageResolver / SocialHierarchyAnalysis）。
  - `web/index.html`：Three.js 3D/VR 预览界面（粘贴 JSON 输出即可）。
  - `CMakeLists.txt`：CMake 构建配置。
  - 示例输入：`sample_compressed.txt`（压缩格式）、`sample_edgelist.txt`（边列表格式）。
  - 示例输出：`result_json_example.txt`（JSON 格式）。

## 构建

### 使用 g++（推荐）

确保安装 MinGW 或类似 C++ 编译器。

```bash
g++ -Wall -Wextra -g3 topsort.cpp core/graph.cpp -o output/topsort.exe
```

### 使用 CMake

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_MAKE_PROGRAM="path/to/mingw32-make.exe"
cmake --build .
```

注意：CMake 配置可能需要指定 MinGW 路径。若有问题，直接使用 g++ 编译。

## 运行

程序从标准输入读取图数据，支持两种输入格式（自动检测）。

### 命令行参数

- `--algo <name>`：排序算法（默认 `dfs`）同上。
- `--format <text|json>`：输出格式。
- `--layout`：额外输出 3D 坐标（layout 字段）。
- `--demo <course|task|package|social>`：运行内置业务示例并输出拓扑与 layout。
- `<format>`：输出格式（可选，默认 `text`）
  - `text`：人类可读文本。
  - `json`：机器可读 JSON。

### 示例命令

#### 文本输出（默认）

```cmd
topsort.exe < sample_compressed.txt
```

输出示例：
```
Algorithm: dfs  Time(ms): 0
h: 0 0 0 1 2 4 6
list: 3 1 0 1 2 0
Topological order: 5 4 2 3 1 0
```

#### 指定算法和格式

```cmd
topsort.exe dfs text < sample_compressed.txt
```

#### JSON 输出

```cmd
topsort.exe both json < sample_compressed.txt > result.json
```

输出示例（每行一个算法）：
```json
{"algorithm":"dfs","has_cycle":false,"time_ms":0,"h":[0,0,0,1,2,4,6],"list":[3,1,0,1,2,0],"topo":[5,4,2,3,1,0]}
{"algorithm":"kahn","has_cycle":false,"time_ms":0,"h":[0,0,0,1,2,4,6],"list":[3,1,0,1,2,0],"topo":[4,5,2,0,3,1]}
```

#### 在 PowerShell 中运行（注意重定向）

使用 `cmd /c` 包装：

```powershell
cmd /c "topsort.exe < sample_compressed.txt"
```

或使用管道：

```powershell
Get-Content sample_compressed.txt | .\topsort.exe
```

## 输入格式

程序自动检测输入格式。

### 压缩邻接表格式

第一行：`n m`（节点数 n，边数 m）  
第二行：`h[0] h[1] ... h[n]`（n+1 个整数，表示邻接起始索引）后接 `list`（m 个整数，表示邻接节点）。

示例（`sample_compressed.txt`）：
```
6 6
0 0 0 1 2 4 6 3 1 0 1 2 0
```

### 边列表格式（备用）

第一行：`n m`  
接下来 m 行：每行 `u v`（有向边 u -> v）。

示例（`sample_edgelist.txt`）：
```
6 6
0 3
1 1
1 0
2 1
2 2
3 0
```

## 输出格式

### 文本格式

- 显示算法、时间（毫秒）。
- 显示 `h` 和 `list`。
- 显示拓扑排序结果（若无环）或环检测信息。

### JSON 格式

每个算法一行 JSON 对象：
- `algorithm`：算法名（"dfs" 或 "kahn"）。
- `has_cycle`：布尔值，是否检测到环。
- `time_ms`：执行时间（毫秒）。
- `h`：h 数组。
- `list`：list 数组。
- `topo`：拓扑排序数组（若无环）。

## 注意事项

- 节点编号从 0 到 n-1。
- 若检测到环，`topo` 为空，`has_cycle` 为 true。
- 程序使用 STL，编译时需支持 C++17。
- 在 Windows 上，使用 cmd 进行输入重定向更稳定。