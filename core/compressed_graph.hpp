#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// Lightweight spin lock to avoid depending on std::mutex in environments without gthreads.
class SpinLock {
public:
    void lock() {
        while (flag_.test_and_set(std::memory_order_acquire)) {}
    }
    void unlock() { flag_.clear(std::memory_order_release); }
private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

class SpinGuard {
public:
    explicit SpinGuard(SpinLock &l) : lock_(l) { lock_.lock(); }
    ~SpinGuard() { lock_.unlock(); }
private:
    SpinLock &lock_;
};

// Lightweight graph interface so solvers stay decoupled from storage.
class GraphInterface {
public:
    using node_t = uint32_t;
    virtual ~GraphInterface() = default;
    virtual size_t node_count() const = 0;
    // Neighbor iteration is cache-friendly and thread-safe as long as the graph is not mutated.
    virtual void for_each_neighbor(node_t u, const std::function<void(node_t)> &fn) const = 0;
    // Fast contiguous neighbor span; {begin, end} over an internal CSR buffer.
    virtual std::pair<const node_t *, const node_t *> neighbor_span(node_t u) const = 0;
};

// Bidirectional index for mapping external node labels to dense ids and back.
class NodeIndex {
public:
    uint32_t intern(const std::string &label);
    bool find(const std::string &label, uint32_t &id) const;
    const std::string &label(uint32_t id) const { return labels_[id]; }
    size_t size() const { return labels_.size(); }
private:
    std::unordered_map<std::string, uint32_t> to_id_;
    std::vector<std::string> labels_;
};

// CSR with optional varint-compressed backing store. Reads are thread-safe; writes are not.
class CompressedGraph : public GraphInterface {
public:
    using node_t = GraphInterface::node_t;
    CompressedGraph() = default;
    explicit CompressedGraph(size_t n) { reset(n); }

    void reset(size_t n);
    void build_from_adj(const std::vector<std::vector<node_t>> &adj);
    void build_from_edges(size_t n, const std::vector<std::pair<node_t, node_t>> &edges);

    // Mutating edge insertion for incremental use cases; invalidates CSR cache until next read.
    void add_edge(node_t u, node_t v);

    size_t node_count() const override { return adj_lists_.size(); }
    void for_each_neighbor(node_t u, const std::function<void(node_t)> &fn) const override;
    std::pair<const node_t *, const node_t *> neighbor_span(node_t u) const override;

    // Varint-backed neighbor scan (delta-coded, ascending adjacency required).
    void build_varint() const;
    void for_each_neighbor_varint(node_t u, const std::function<void(node_t)> &fn) const;

    void export_csr(std::vector<uint32_t> &offsets, std::vector<node_t> &neighbors) const;

    const std::vector<uint32_t> &indegrees() const { return indeg_; }
    size_t dense_bytes() const { return neighbors_.size() * sizeof(node_t) + offsets_.size() * sizeof(uint32_t); }
    size_t varint_bytes() const { return neighbors_varint_.size() + varint_offsets_.size() * sizeof(uint32_t); }

private:
    void ensure_csr() const;
    void rebuild_csr_unlocked() const;

    std::vector<std::vector<node_t>> adj_lists_{}; // mutable adjacency for incremental updates
    mutable std::vector<node_t> neighbors_{};       // CSR neighbors (dense)
    mutable std::vector<uint32_t> offsets_{};       // CSR offsets (size n+1)
    std::vector<uint32_t> indeg_{};

    // Varint buffers (built on demand from CSR).
    mutable std::vector<uint8_t> neighbors_varint_{};
    mutable std::vector<uint32_t> varint_offsets_{};

    mutable SpinLock csr_lock_{};
    mutable bool dirty_{true};
};

// Minimal varint helpers (7-bit groups, LEB128-compatible for uint32_t).
size_t encode_varint32(uint32_t value, std::vector<uint8_t> &out);
uint32_t decode_varint32(const uint8_t *&ptr, const uint8_t *end);
