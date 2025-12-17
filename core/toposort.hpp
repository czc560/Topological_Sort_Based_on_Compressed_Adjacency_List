#pragma once

#include "compressed_graph.hpp"

#include <atomic>
#include <memory>
#include <queue>
#include <vector>

// Base class: all solvers return true when a cycle is found.
class TopoSortSolver {
public:
    using node_t = GraphInterface::node_t;
    explicit TopoSortSolver(GraphInterface &g) : g_(g) {}
    virtual ~TopoSortSolver() = default;

    virtual bool run(std::vector<node_t> &order) = 0;
    virtual const char *name() const = 0;

protected:
    GraphInterface &g_;
};

// DFS with three-color marking. Time O(n+m), space O(n).
class DFSTopoSolver : public TopoSortSolver {
public:
    using TopoSortSolver::TopoSortSolver;
    bool run(std::vector<node_t> &order) override;
    const char *name() const override { return "dfs"; }
};

// Kahn queue-based solver. Time O(n+m), space O(n).
class KahnTopoSolver : public TopoSortSolver {
public:
    using TopoSortSolver::TopoSortSolver;
    bool run(std::vector<node_t> &order) override;
    const char *name() const override { return "kahn"; }
};

// Lexicographic Kahn using a priority queue for min/max order. Time O((n+m) log n), space O(n).
class LexicographicKahnSolver : public TopoSortSolver {
public:
    LexicographicKahnSolver(GraphInterface &g, bool min_first) : TopoSortSolver(g), min_first_(min_first) {}
    bool run(std::vector<node_t> &order) override;
    const char *name() const override { return min_first_ ? "lexi_min" : "lexi_max"; }
private:
    bool min_first_;
};

// Parallel Kahn: work-stealing via shared queue; thread-safe reads only. Time roughly O((n+m)/p + contention), space O(n).
class ParallelKahnSolver : public TopoSortSolver {
public:
    ParallelKahnSolver(GraphInterface &g, size_t worker_count);
    bool run(std::vector<node_t> &order) override;
    const char *name() const override { return "parallel_kahn"; }
private:
    size_t workers_;
};

// Incremental topological sort supporting edge insertions without full recompute.
// Amortized time O(k + |E_s|) for subgraph size |E_s| touched by the violating edge; space O(n + m).
// Proof sketch for correctness: (1) maintain invariant that order_ is a valid topological ordering; (2) on insertion u->v
// where position[u] >= position[v], the BFS collects exactly the nodes reachable from v that violate the invariant;
// (3) the local Kahn pass reorders only that induced subgraph, producing a valid order_ restricted to it; (4) splicing
// the local order at the earliest affected position preserves precedence constraints for all unaffected nodes, hence the
// resulting order_ is a topological order for the updated DAG.
class IncrementalTopoSolver : public TopoSortSolver {
public:
    explicit IncrementalTopoSolver(CompressedGraph &g);
    bool run(std::vector<node_t> &order) override; // runs/refreshes current order
    const char *name() const override { return "incremental"; }

    // Returns true if no cycle introduced by the new edge.
    bool add_edge(node_t u, node_t v);
private:
    bool ensure_initialized();
    bool relabel_after_insertion(node_t u, node_t v);

    CompressedGraph &cg_;
    std::vector<node_t> order_;
    std::vector<uint32_t> position_;
};

// Helper: compute indegrees from a graph view.
std::vector<uint32_t> compute_indegrees(const GraphInterface &g);
