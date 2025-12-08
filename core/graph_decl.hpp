#pragma once
#include <vector>
#include <string>
#include <utility>

// core/graph_decl.hpp
// 声明：基于紧缩邻接表的图操作与拓扑排序

void build_compressed(const std::vector<std::vector<int>> &adj, std::vector<int> &h, std::vector<int> &list);
std::pair<int,int> get_neighbors(int u, const std::vector<int> &h);
bool topsort_dfs(int n, const std::vector<int> &h, const std::vector<int> &list, std::vector<int> &topo);
bool topsort_kahn(int n, const std::vector<int> &h, const std::vector<int> &list, std::vector<int> &topo);
std::string to_json_array(const std::vector<int> &a);
