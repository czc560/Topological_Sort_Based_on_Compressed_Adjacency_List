#include "graph_decl.hpp"
#include <algorithm>
#include <functional>
#include <deque>
#include <sstream>

using namespace std;

void build_compressed(const vector<vector<int>> &adj, vector<int> &h, vector<int> &list) {
    int n = (int)adj.size();
    h.assign(n + 1, 0);
    for (int i = 0; i < n; ++i) h[i + 1] = h[i] + (int)adj[i].size();
    list.clear();
    list.reserve(h[n]);
    for (int i = 0; i < n; ++i) for (int v : adj[i]) list.push_back(v);
}

pair<int,int> get_neighbors(int u, const vector<int> &h) {
    return {h[u], h[u+1]};
}

bool topsort_dfs(int n, const vector<int> &h, const vector<int> &list, vector<int> &topo) {
    vector<int> state(n, 0); // 0 unvisited,1 visiting,2 done
    topo.clear(); topo.reserve(n);
    function<bool(int)> dfs = [&](int u)->bool{
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
    reverse(topo.begin(), topo.end());
    return false;
}

bool topsort_kahn(int n, const vector<int> &h, const vector<int> &list, vector<int> &topo) {
    topo.clear(); topo.reserve(n);
    vector<int> indeg(n, 0);
    for (int u = 0; u < n; ++u) for (int idx = h[u]; idx < h[u+1]; ++idx) indeg[list[idx]]++;
    deque<int> q;
    for (int i = 0; i < n; ++i) if (indeg[i] == 0) q.push_back(i);
    while (!q.empty()) {
        int u = q.front(); q.pop_front();
        topo.push_back(u);
        for (int idx = h[u]; idx < h[u+1]; ++idx) {
            int v = list[idx];
            if (--indeg[v] == 0) q.push_back(v);
        }
    }
    return (int)topo.size() != n;
}

string to_json_array(const vector<int> &a) {
    string s = "[";
    for (size_t i = 0; i < a.size(); ++i) {
        if (i) s += ",";
        s += to_string(a[i]);
    }
    s += "]";
    return s;
}
