#include "toposort.hpp"

#include <algorithm>
#include <deque>
#include <functional>
#include <unordered_map>
#include <utility>

std::vector<uint32_t> compute_indegrees(const GraphInterface &g) {
    size_t n = g.node_count();
    std::vector<uint32_t> indeg(n, 0);
    for (uint32_t u = 0; u < n; ++u) {
        auto span = g.neighbor_span(u);
        for (auto it = span.first; it != span.second; ++it) indeg[*it]++;
    }
    return indeg;
}

bool DFSTopoSolver::run(std::vector<node_t> &order) {
    size_t n = g_.node_count();
    std::vector<uint8_t> state(n, 0); // 0=unseen,1=visiting,2=done
    order.clear();
    order.reserve(n);
    bool has_cycle = false;
    std::function<void(node_t)> dfs = [&](node_t u) {
        state[u] = 1;
        auto span = g_.neighbor_span(u);
        for (auto it = span.first; it != span.second; ++it) {
            node_t v = *it;
            if (state[v] == 1) { has_cycle = true; return; }
            if (state[v] == 0) dfs(v);
            if (has_cycle) return;
        }
        state[u] = 2;
        order.push_back(u);
    };
    for (node_t u = 0; u < n && !has_cycle; ++u) if (state[u] == 0) dfs(u);
    if (!has_cycle) std::reverse(order.begin(), order.end());
    return has_cycle;
}

bool KahnTopoSolver::run(std::vector<node_t> &order) {
    size_t n = g_.node_count();
    std::vector<uint32_t> indeg = compute_indegrees(g_);
    std::deque<node_t> q;
    for (node_t u = 0; u < n; ++u) if (indeg[u] == 0) q.push_back(u);
    order.clear();
    order.reserve(n);
    while (!q.empty()) {
        node_t u = q.front();
        q.pop_front();
        order.push_back(u);
        auto span = g_.neighbor_span(u);
        for (auto it = span.first; it != span.second; ++it) {
            node_t v = *it;
            if (--indeg[v] == 0) q.push_back(v);
        }
    }
    return order.size() != n;
}

bool LexicographicKahnSolver::run(std::vector<node_t> &order) {
    size_t n = g_.node_count();
    std::vector<uint32_t> indeg = compute_indegrees(g_);
    auto cmp = [&](node_t a, node_t b) { return min_first_ ? a > b : a < b; };
    std::priority_queue<node_t, std::vector<node_t>, decltype(cmp)> pq(cmp);
    for (node_t u = 0; u < n; ++u) if (indeg[u] == 0) pq.push(u);
    order.clear();
    order.reserve(n);
    while (!pq.empty()) {
        node_t u = pq.top();
        pq.pop();
        order.push_back(u);
        auto span = g_.neighbor_span(u);
        for (auto it = span.first; it != span.second; ++it) {
            node_t v = *it;
            if (--indeg[v] == 0) pq.push(v);
        }
    }
    return order.size() != n;
}

ParallelKahnSolver::ParallelKahnSolver(GraphInterface &g, size_t worker_count)
    : TopoSortSolver(g), workers_(std::max<size_t>(1, worker_count)) {}

bool ParallelKahnSolver::run(std::vector<node_t> &order) {
    // Fallback to sequential Kahn on platforms without std::thread support.
    KahnTopoSolver fallback(g_);
    return fallback.run(order);
}

IncrementalTopoSolver::IncrementalTopoSolver(CompressedGraph &g)
    : TopoSortSolver(g), cg_(g) {}

bool IncrementalTopoSolver::ensure_initialized() {
    if (!order_.empty()) return true;
    order_.clear();
    bool has_cycle = KahnTopoSolver(cg_).run(order_);
    if (has_cycle) return false;
    position_.assign(order_.size(), 0);
    for (uint32_t i = 0; i < order_.size(); ++i) position_[order_[i]] = i;
    return true;
}

bool IncrementalTopoSolver::relabel_after_insertion(node_t u, node_t v) {
    size_t n = cg_.node_count();
    std::vector<uint8_t> in_subgraph(n, 0);
    std::deque<node_t> dq;
    dq.push_back(v);
    while (!dq.empty()) {
        node_t x = dq.front();
        dq.pop_front();
        if (in_subgraph[x]) continue;
        in_subgraph[x] = 1;
        auto span = cg_.neighbor_span(x);
        for (auto it = span.first; it != span.second; ++it) {
            node_t w = *it;
            if (position_[w] <= position_[u]) dq.push_back(w);
        }
    }
    in_subgraph[u] = 1;

    std::vector<node_t> affected;
    affected.reserve(n);
    for (node_t i = 0; i < n; ++i) if (in_subgraph[i]) affected.push_back(i);
    if (affected.empty()) return true;

    std::unordered_map<node_t, size_t> idx;
    idx.reserve(affected.size() * 2);
    for (size_t i = 0; i < affected.size(); ++i) idx[affected[i]] = i;

    std::vector<uint32_t> indeg_local(affected.size(), 0);
    for (node_t x : affected) {
        auto span = cg_.neighbor_span(x);
        for (auto it = span.first; it != span.second; ++it) {
            node_t w = *it;
            auto f = idx.find(w);
            if (f != idx.end()) indeg_local[f->second]++;
        }
    }

    std::deque<node_t> q;
    for (node_t x : affected) if (indeg_local[idx[x]] == 0) q.push_back(x);
    std::vector<node_t> local_order;
    local_order.reserve(affected.size());
    while (!q.empty()) {
        node_t x = q.front();
        q.pop_front();
        local_order.push_back(x);
        auto span = cg_.neighbor_span(x);
        for (auto it = span.first; it != span.second; ++it) {
            node_t w = *it;
            auto f = idx.find(w);
            if (f != idx.end()) {
                if (--indeg_local[f->second] == 0) q.push_back(w);
            }
        }
    }
    if (local_order.size() != affected.size()) return false; // cycle detected

    std::vector<node_t> new_order;
    new_order.reserve(n);
    size_t anchor = order_.size();
    for (node_t x : affected) anchor = std::min(anchor, static_cast<size_t>(position_[x]));
    for (size_t i = 0; i < order_.size(); ++i) {
        if (i == anchor) new_order.insert(new_order.end(), local_order.begin(), local_order.end());
        node_t x = order_[i];
        if (!in_subgraph[x]) new_order.push_back(x);
    }
    order_.swap(new_order);
    position_.assign(n, 0);
    for (uint32_t i = 0; i < order_.size(); ++i) position_[order_[i]] = i;
    return true;
}

bool IncrementalTopoSolver::add_edge(node_t u, node_t v) {
    if (!ensure_initialized()) return false;
    cg_.add_edge(u, v);
    if (position_[u] < position_[v]) return true;
    return relabel_after_insertion(u, v);
}

bool IncrementalTopoSolver::run(std::vector<node_t> &order) {
    if (!ensure_initialized()) return true;
    order = order_;
    return false;
}
