#include "graph_clean.hpp"

#include <algorithm>
#include <functional>
#include <deque>
#include <sstream>
#include <vector>
#include <string>
#include <utility>

void build_compressed(const std::vector<std::vector<int>> &adj, std::vector<int> &h, std::vector<int> &list) {
    int n = static_cast<int>(adj.size());
    h.assign(n + 1, 0);
    for (int i = 0; i < n; ++i) h[i + 1] = h[i] + static_cast<int>(adj[i].size());
    list.clear();
    list.reserve(h[n]);
    for (int i = 0; i < n; ++i) for (int v : adj[i]) list.push_back(v);
}

std::pair<int,int> get_neighbors(int u, const std::vector<int> &h) {
    return {h[u], h[u+1]};
}

bool topsort_dfs(int n, const std::vector<int> &h, const std::vector<int> &list, std::vector<int> &topo) {
    std::vector<int> state(n, 0); // 0 unvisited,1 visiting,2 done
    topo.clear(); topo.reserve(n);
    std::function<bool(int)> dfs = [&](int u)->bool{
        state[u] = 1;
        for (int idx = h[u]; idx < h[u+1]; ++idx) {
            int v = list[idx];
            if (state[v] == 1) return true; // found cycle
            if (state[v] == 0) if (dfs(v)) return true;
        }
        state[u] = 2;
        topo.push_back(u);
        return false;
    };
    for (int i = 0; i < n; ++i) if (state[i] == 0) if (dfs(i)) return true;
    std::reverse(topo.begin(), topo.end());
    return false;
}

bool topsort_kahn(int n, const std::vector<int> &h, const std::vector<int> &list, std::vector<int> &topo) {
    topo.clear(); topo.reserve(n);
    std::vector<int> indeg(n, 0);
    for (int u = 0; u < n; ++u) for (int idx = h[u]; idx < h[u+1]; ++idx) indeg[list[idx]]++;
    std::deque<int> q;
    for (int i = 0; i < n; ++i) if (indeg[i] == 0) q.push_back(i);
    while (!q.empty()) {
        int u = q.front(); q.pop_front();
        topo.push_back(u);
        for (int idx = h[u]; idx < h[u+1]; ++idx) {
            int v = list[idx];
            if (--indeg[v] == 0) q.push_back(v);
        }
    }
    return static_cast<int>(topo.size()) != n;
}

std::string to_json_array(const std::vector<int> &a) {
    std::string s = "[";
    for (size_t i = 0; i < a.size(); ++i) {
        if (i) s += ",";
        s += std::to_string(a[i]);
    }
    s += "]";
    return s;
}
