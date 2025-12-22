# Topological Sort (CSR + Layout + Demos)

## 1. 快速构建

### g++（当前工程已测试）
```cmd
g++ -std=c++17 -O2 -Wall -Wextra topsort.cpp core/graph.cpp core/compressed_graph.cpp core/toposort.cpp core/layout.cpp core/demos.cpp -o output/topsort.exe
```

### CMake（可选）
```cmd
mkdir build
cd build
cmake ..
cmake --build .
```

## 2. CLI 用法（含生成 layout）

通用参数：
- `--algo <dfs|kahn|parallel|lexi_min|lexi_max|incremental|both>`（默认 dfs，parallel 在当前工具链回退为顺序）
- `--format <text|json>`（默认 text）
- `--layout`：输出 layout（3D 坐标）
- `--demo <course|task|package|social>`：运行内置示例

输入可为压缩 CSR（n m 后接 h 与 list）或边列表（n m 后接 m 对 u v），自动识别。

示例：
- 从示例压缩输入生成 JSON + layout：
  ```cmd
  cmd /c "output\topsort.exe dfs json --layout < sample_compressed.txt"
  ```
- 运行 Kahn 文本输出：
  ```cmd
  cmd /c "output\topsort.exe kahn text < sample_compressed.txt"
  ```
- 内置 demo（课程排课），含 layout：
  ```cmd
  cmd /c "output\topsort.exe --demo course --algo kahn --format json --layout"
  ```
- PowerShell 管道写法：
  ```powershell
  Get-Content sample_compressed.txt | .\output\topsort.exe dfs json --layout
  ```

## 3. 输入格式参考
- 压缩 CSR：第一行 `n m`，第二行开始 `h[0..n]` 与 `list`。
  示例 `sample_compressed.txt`:
  ```
  6 6
  0 0 0 1 2 4 6 3 1 0 1 2 0
  ```
- 边列表：第一行 `n m`，后续 m 行 `u v`。

## 4. Web 可视化（3D + 动画删边过程）

### 启动本地静态服务器
在项目根目录：
```cmd
python -m http.server 8000
```
浏览器访问 `http://localhost:8000/web/index.html`。

### 使用步骤
1) 在 CLI 生成包含 `layout`、`h`、`list`、`topo` 的 JSON（见上方命令）。
2) 打开页面左侧文本框，粘贴整段 JSON，点击 `Load Layout`。
3) 控件：`Play`（播放删边动画，红色高亮即将删除的边，黑色为未删，灰色为已删；节点处理时换色）、`Pause`、`Reset`。`Step(ms)` 可调帧间隔。
4) 场景：白色背景；节点初始灰色，处理后依次换不同颜色；边状态如上。

## 5. 项目结构
- `topsort.cpp`：CLI 入口，解析参数/输出 JSON 与布局。
- `core/compressed_graph.*`：CSR + Varint 压缩存储。
- `core/toposort.*`：DFS、Kahn、并行（回退顺序）、增量、字典序算法。
- `core/layout.*`：拓扑层级生成 3D 坐标。
- `core/demos.*`：CourseScheduler / TaskDependencyManager / PackageResolver / SocialHierarchyAnalysis。
- `web/index.html`：Three.js 可视化与删边动画。
- `sample_compressed.txt` / `sample_edgelist.txt`：示例输入。

## 6. 注意
- 节点编号 0..n-1。
- 检测到环时 `has_cycle=true`，`topo` 为空（或 null）。
- 需 C++17。parallel 在当前 MinGW 工具链回退为顺序版本。