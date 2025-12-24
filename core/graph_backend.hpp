#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// GraphDataStore keeps both adjacency lists and CSR (offsets/neighbors) in sync.
// All node ids are 0..n-1.
struct ValidationResult {
    bool ok{false};
    bool has_cycle{false};
    std::string error; // empty when ok==true
};

class GraphDataStore {
public:
    using node_t = uint32_t;

    // Load from an adjacency list; deduplicates neighbors and rebuilds CSR.
    bool load_from_adj(const std::vector<std::vector<node_t>> &adj, std::string &err);

    // Parse from text. Supported formats:
    // 1) Edge list: first line n m; then m pairs u v.
    // 2) Compressed CSR: first line n m; then h[0..n] followed by list, where h[n] == list.size().
    bool load_from_text(const std::string &text, std::string &err);

    // Validate internal consistency and check DAG (cycle-free). Returns false on any error.
    bool validate_graph(ValidationResult &out) const;

    // Accessors
    size_t node_count() const { return adj_.size(); }
    const std::vector<std::vector<node_t>> &adjacency() const { return adj_; }
    const std::vector<node_t> &offsets() const { return offsets_; }
    const std::vector<node_t> &neighbors() const { return neighbors_; }

private:
    void rebuild_csr();
    bool parse_rest(int n, int m, const std::vector<int> &rest, std::string &err);
    static void sort_and_dedup(std::vector<node_t> &v);
    bool detect_cycle() const;

    std::vector<std::vector<node_t>> adj_{};
    std::vector<node_t> offsets_{};  // size n+1
    std::vector<node_t> neighbors_{};
};
