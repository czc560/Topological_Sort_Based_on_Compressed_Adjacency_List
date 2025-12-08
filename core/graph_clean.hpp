#pragma once
#include <vector>
#include <string>
#include <utility>

// core/graph_clean.hpp
// 声明：基于紧缩邻接表的图操作与拓扑排序（干净头文件）

// 从普通邻接表构建紧缩邻接表 h 和 list
void build_compressed(const std::vector<std::vector<int>> &adj, std::vector<int> &h, std::vector<int> &list);

// 获取顶点 u 的邻接区间 [l, r)
std::pair<int,int> get_neighbors(int u, const std::vector<int> &h);

// DFS 拓扑排序；返回 true 表示检测到了环；当无环时 topo 返回拓扑序
bool topsort_dfs(int n, const std::vector<int> &h, const std::vector<int> &list, std::vector<int> &topo);

// Kahn 拓扑排序；返回 true 表示检测到了环
bool topsort_kahn(int n, const std::vector<int> &h, const std::vector<int> &list, std::vector<int> &topo);

// 简单 JSON 数组序列化
std::string to_json_array(const std::vector<int> &a);
