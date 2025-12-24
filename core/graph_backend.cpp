#include "graph_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <stdexcept>

namespace {
// Simple helper to ensure all values are within [0, n).
bool neighbors_in_range(const std::vector<std::vector<GraphDataStore::node_t>> &adj, size_t n, std::string &err) {
    for (size_t u = 0; u < adj.size(); ++u) {
        for (auto v : adj[u]) {
            if (v >= n) {
                std::stringstream ss;
                ss << "neighbor out of range at node " << u << ": " << v << " >= " << n;
                err = ss.str();
                return false;
            }
        }
    }
    return true;
}
}

void GraphDataStore::sort_and_dedup(std::vector<node_t> &v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
}

bool GraphDataStore::load_from_adj(const std::vector<std::vector<node_t>> &adj, std::string &err) {
    adj_ = adj;
    for (auto &lst : adj_) sort_and_dedup(lst);
    rebuild_csr();

    ValidationResult vr;
    if (!validate_graph(vr)) {
        err = vr.error;
        return false;
    }
    return true;
}

bool GraphDataStore::parse_rest(int n, int m, const std::vector<int> &rest, std::string &err) {
    adj_.assign(static_cast<size_t>(n), {});

    // Try compressed format: rest[0..n] = h, rest[n] = list size, rest[n+1..] = list
    if (static_cast<int>(rest.size()) >= n + 1) {
        int list_size = rest[n];
        if (list_size >= 0 && list_size == static_cast<int>(rest.size()) - (n + 1)) {
            const size_t h_size = static_cast<size_t>(n + 1);
            std::vector<int> h(rest.begin(), rest.begin() + static_cast<std::ptrdiff_t>(h_size));
            const std::vector<int> list(rest.begin() + static_cast<std::ptrdiff_t>(h_size), rest.end());
            for (int u = 0; u < n; ++u) {
                int l = h[u];
                int r = h[u + 1];
                if (l < 0 || r < l || r > list_size) {
                    err = "invalid CSR offsets";
                    return false;
                }
                for (int idx = l; idx < r; ++idx) {
                    int v = list[static_cast<size_t>(idx)];
                    if (v < 0 || v >= n) {
                        std::stringstream ss;
                        ss << "neighbor out of range at node " << u;
                        err = ss.str();
                        return false;
                    }
                    adj_[static_cast<size_t>(u)].push_back(static_cast<node_t>(v));
                }
            }
            for (auto &lst : adj_) sort_and_dedup(lst);
            rebuild_csr();
            return true;
        }
    }

    // Try edge list: m pairs
    if (static_cast<int>(rest.size()) == 2 * m) {
        for (int i = 0; i < m; ++i) {
            int u = rest[2 * i];
            int v = rest[2 * i + 1];
            if (u < 0 || v < 0 || u >= n || v >= n) {
                err = "edge endpoint out of range";
                return false;
            }
            adj_[static_cast<size_t>(u)].push_back(static_cast<node_t>(v));
        }
        for (auto &lst : adj_) sort_and_dedup(lst);
        rebuild_csr();
        return true;
    }

    err = "unrecognized input format";
    return false;
}

bool GraphDataStore::load_from_text(const std::string &text, std::string &err) {
    std::stringstream ss(text);
    int n, m;
    if (!(ss >> n >> m)) {
        err = "failed to read n m";
        return false;
    }
    if (n < 0 || m < 0) {
        err = "n and m must be non-negative";
        return false;
    }
    std::vector<int> rest;
    rest.reserve(static_cast<size_t>(n + m) * 2);
    int x;
    while (ss >> x) rest.push_back(x);

    if (!parse_rest(n, m, rest, err)) return false;

    ValidationResult vr;
    if (!validate_graph(vr)) {
        err = vr.error;
        return false;
    }
    return true;
}

void GraphDataStore::rebuild_csr() {
    const size_t n = adj_.size();
    offsets_.assign(n + 1, 0);
    for (size_t i = 0; i < n; ++i) offsets_[i + 1] = offsets_[i] + static_cast<node_t>(adj_[i].size());
    neighbors_.resize(offsets_[n]);
    for (size_t u = 0; u < n; ++u) {
        auto base = offsets_[u];
        std::copy(adj_[u].begin(), adj_[u].end(), neighbors_.begin() + static_cast<std::ptrdiff_t>(base));
    }
}

bool GraphDataStore::detect_cycle() const {
    const size_t n = adj_.size();
    std::vector<node_t> indeg(n, 0);
    for (size_t u = 0; u < n; ++u) for (auto v : adj_[u]) indeg[v]++;
    std::vector<node_t> q;
    q.reserve(n);
    for (size_t i = 0; i < n; ++i) if (indeg[i] == 0) q.push_back(static_cast<node_t>(i));
    size_t head = 0;
    while (head < q.size()) {
        node_t u = q[head++];
        for (auto v : adj_[u]) if (--indeg[v] == 0) q.push_back(v);
    }
    return q.size() != n;
}

bool GraphDataStore::validate_graph(ValidationResult &out) const {
    out = ValidationResult{};

    const size_t n = adj_.size();
    // Neighbor range check on adjacency.
    {
        std::string err;
        if (!neighbors_in_range(adj_, n, err)) {
            out.ok = false;
            out.error = err;
            return false;
        }
    }

    // CSR consistency check.
    if (offsets_.size() != n + 1) {
        out.ok = false;
        out.error = "offsets length mismatch";
        return false;
    }
    for (size_t i = 0; i + 1 < offsets_.size(); ++i) {
        if (offsets_[i] > offsets_[i + 1]) {
            out.ok = false;
            out.error = "offsets not non-decreasing";
            return false;
        }
    }
    if (neighbors_.size() != offsets_[n]) {
        out.ok = false;
        out.error = "neighbors size mismatch";
        return false;
    }
    for (auto v : neighbors_) {
        if (v >= n) {
            out.ok = false;
            out.error = "neighbor out of range in CSR";
            return false;
        }
    }

    // Cycle detection.
    bool has_cycle = detect_cycle();
    out.has_cycle = has_cycle;
    if (has_cycle) {
        out.ok = false;
        out.error = "graph has a cycle; topological order does not exist";
        return false;
    }

    out.ok = true;
    return true;
}
