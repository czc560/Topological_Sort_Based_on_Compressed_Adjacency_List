#include "compressed_graph.hpp"

#include <algorithm>
#include <stdexcept>

namespace {
void sort_and_dedup(std::vector<uint32_t> &v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
}
}

uint32_t decode_varint32(const uint8_t *&ptr, const uint8_t *end) {
    uint32_t value = 0;
    uint32_t shift = 0;
    while (ptr < end) {
        uint8_t byte = *ptr++;
        value |= static_cast<uint32_t>(byte & 0x7Fu) << shift;
        if ((byte & 0x80u) == 0) break;
        shift += 7;
    }
    return value;
}

size_t encode_varint32(uint32_t value, std::vector<uint8_t> &out) {
    size_t start = out.size();
    while (value >= 0x80u) {
        out.push_back(static_cast<uint8_t>(value | 0x80u));
        value >>= 7;
    }
    out.push_back(static_cast<uint8_t>(value));
    return out.size() - start;
}

uint32_t NodeIndex::intern(const std::string &label) {
    auto it = to_id_.find(label);
    if (it != to_id_.end()) return it->second;
    uint32_t id = static_cast<uint32_t>(labels_.size());
    labels_.push_back(label);
    to_id_.emplace(label, id);
    return id;
}

bool NodeIndex::find(const std::string &label, uint32_t &id) const {
    auto it = to_id_.find(label);
    if (it == to_id_.end()) return false;
    id = it->second;
    return true;
}

void CompressedGraph::reset(size_t n) {
    adj_lists_.assign(n, {});
    indeg_.assign(n, 0);
    offsets_.assign(n + 1, 0);
    neighbors_.clear();
    neighbors_varint_.clear();
    varint_offsets_.clear();
    dirty_ = true;
}

void CompressedGraph::build_from_adj(const std::vector<std::vector<node_t>> &adj) {
    reset(adj.size());
    for (size_t u = 0; u < adj.size(); ++u) {
        adj_lists_[u] = adj[u];
        for (node_t v : adj[u]) {
            if (v >= adj.size()) throw std::out_of_range("neighbor id exceeds node_count");
            indeg_[v]++;
        }
        sort_and_dedup(adj_lists_[u]);
    }
    dirty_ = true;
}

void CompressedGraph::build_from_edges(size_t n, const std::vector<std::pair<node_t, node_t>> &edges) {
    reset(n);
    for (const auto &e : edges) {
        node_t u = e.first;
        node_t v = e.second;
        if (u >= n || v >= n) throw std::out_of_range("edge endpoint out of bounds");
        adj_lists_[u].push_back(v);
        indeg_[v]++;
    }
    for (auto &lst : adj_lists_) sort_and_dedup(lst);
    dirty_ = true;
}

void CompressedGraph::add_edge(node_t u, node_t v) {
    if (u >= adj_lists_.size() || v >= adj_lists_.size()) throw std::out_of_range("edge endpoint out of bounds");
    auto &lst = adj_lists_[u];
    lst.push_back(v);
    std::sort(lst.begin(), lst.end());
    lst.erase(std::unique(lst.begin(), lst.end()), lst.end());
    indeg_[v]++;
    dirty_ = true;
}

void CompressedGraph::rebuild_csr_unlocked() const {
    size_t n = adj_lists_.size();
    offsets_.assign(n + 1, 0);
    for (size_t i = 0; i < n; ++i) offsets_[i + 1] = offsets_[i] + adj_lists_[i].size();
    neighbors_.resize(offsets_[n]);
    for (size_t u = 0; u < n; ++u) {
        auto base = offsets_[u];
        std::copy(adj_lists_[u].begin(), adj_lists_[u].end(), neighbors_.begin() + static_cast<ptrdiff_t>(base));
    }
    neighbors_varint_.clear();
    varint_offsets_.clear();
    dirty_ = false;
}

void CompressedGraph::ensure_csr() const {
    if (!dirty_) return;
    SpinGuard guard(csr_lock_);
    if (dirty_) rebuild_csr_unlocked();
}

std::pair<const CompressedGraph::node_t *, const CompressedGraph::node_t *> CompressedGraph::neighbor_span(node_t u) const {
    ensure_csr();
    auto base = offsets_[u];
    auto next = offsets_[u + 1];
    const node_t *beg = neighbors_.data() + static_cast<std::ptrdiff_t>(base);
    return {beg, beg + static_cast<std::ptrdiff_t>(next - base)};
}

void CompressedGraph::for_each_neighbor(node_t u, const std::function<void(node_t)> &fn) const {
    auto span = neighbor_span(u);
    for (auto it = span.first; it != span.second; ++it) fn(*it);
}

void CompressedGraph::build_varint() const {
    ensure_csr();
    varint_offsets_.assign(adj_lists_.size() + 1, 0);
    neighbors_varint_.clear();
    neighbors_varint_.reserve(neighbors_.size());
    varint_offsets_[0] = 0;
    for (size_t u = 0; u < adj_lists_.size(); ++u) {
        uint32_t prev = 0;
        bool first = true;
        for (size_t idx = offsets_[u]; idx < offsets_[u + 1]; ++idx) {
            uint32_t v = neighbors_[idx];
            uint32_t delta = first ? v : (v - prev);
            encode_varint32(delta, neighbors_varint_);
            prev = v;
            first = false;
        }
        varint_offsets_[u + 1] = static_cast<uint32_t>(neighbors_varint_.size());
    }
}

void CompressedGraph::for_each_neighbor_varint(node_t u, const std::function<void(node_t)> &fn) const {
    if (varint_offsets_.empty()) build_varint();
    const uint8_t *beg = neighbors_varint_.data() + varint_offsets_[u];
    const uint8_t *end = neighbors_varint_.data() + varint_offsets_[u + 1];
    uint32_t prev = 0;
    bool first = true;
    const uint8_t *ptr = beg;
    while (ptr < end) {
        uint32_t delta = decode_varint32(ptr, end);
        uint32_t v = first ? delta : prev + delta;
        fn(v);
        prev = v;
        first = false;
    }
}

void CompressedGraph::export_csr(std::vector<uint32_t> &offsets, std::vector<node_t> &neighbors) const {
    ensure_csr();
    offsets = offsets_;
    neighbors = neighbors_;
}
