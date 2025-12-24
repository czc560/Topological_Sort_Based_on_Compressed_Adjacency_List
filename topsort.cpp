#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "core/compressed_graph.hpp"
#include "core/demos.hpp"
#include "core/graph_backend.hpp"
#include "core/graph_clean.hpp"
#include "core/layout.hpp"
#include "core/toposort.hpp"

namespace {
using clock_t = std::chrono::high_resolution_clock;

std::vector<int> to_int_vector(const std::vector<uint32_t> &src) { return std::vector<int>(src.begin(), src.end()); }

void emit_result(const std::string &algo_name,
                 TopoSortSolver &solver,
                 CompressedGraph &graph,
                 const std::string &format,
                 bool with_layout) {
    std::vector<uint32_t> offsets;
    std::vector<uint32_t> neighbors;
    graph.export_csr(offsets, neighbors);

    std::vector<uint32_t> topo;
    std::vector<LayoutPoint> layout_pts;
    auto t0 = clock_t::now();
    bool has_cycle = solver.run(topo);
    if (with_layout && !has_cycle) layout_pts = make_layered_layout(graph, topo, 1.5f, 2.0f, 1.2f);
    auto t1 = clock_t::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (format == "json") {
        std::stringstream ss;
        ss << "{\"algorithm\":\"" << algo_name << "\",";
        ss << "\"has_cycle\":" << (has_cycle ? "true" : "false") << ',';
        ss << "\"time_ms\":" << ms << ',';
        ss << "\"dense_bytes\":" << graph.dense_bytes() << ',';
        ss << "\"varint_bytes\":" << graph.varint_bytes() << ',';
        ss << "\"h\":" << to_json_array(to_int_vector(offsets)) << ',';
        ss << "\"list\":" << to_json_array(to_int_vector(neighbors)) << ',';
        if (has_cycle) {
            ss << "\"topo\":null,\"steps\":null";
        } else {
            auto topo_int = to_int_vector(topo);
            ss << "\"topo\":" << to_json_array(topo_int) << ",\"steps\":" << to_json_array(topo_int);
        }
        if (with_layout && !has_cycle) ss << ",\"layout\":" << layout_to_json(layout_pts);
        ss << "}";
        std::cout << ss.str() << "\n";
    } else {
        std::cerr << "Algorithm: " << algo_name << "  Time(ms): " << ms
                  << "  Dense(bytes): " << graph.dense_bytes()
                  << "  Varint(bytes): " << graph.varint_bytes() << "\n";
        std::cout << "h:";
        for (auto v : offsets) std::cout << ' ' << v;
        std::cout << "\nlist:";
        for (auto v : neighbors) std::cout << ' ' << v;
        std::cout << '\n';
        if (has_cycle) {
            std::cout << "Graph has a cycle; topological order does not exist.\n";
        } else {
            std::cout << "Topological order:";
            for (auto v : topo) std::cout << ' ' << v;
            std::cout << '\n';
            if (with_layout) {
                std::cout << "Layout (id,x,y,z,layer):";
                for (const auto &p : layout_pts) {
                    std::cout << " [" << p.id << ':' << p.x << ',' << p.y << ',' << p.z << " l=" << p.layer << ']';
                }
                std::cout << '\n';
            }
        }
    }
}
}

struct Options {
    std::string algo{"dfs"};
    std::string format{"text"};
    bool with_layout{false};
    std::string demo;
};

Options parse_options(int argc, char **argv) {
    Options opt;
    int pos = 0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--algo" && i + 1 < argc) opt.algo = argv[++i];
        else if (a == "--format" && i + 1 < argc) opt.format = argv[++i];
        else if (a == "--layout") opt.with_layout = true;
        else if (a == "--demo" && i + 1 < argc) opt.demo = argv[++i];
        else {
            // positional fallback for backward compatibility
            if (pos == 0) opt.algo = a;
            else if (pos == 1) opt.format = a;
            pos++;
        }
    }
    return opt;
}

bool run_demo(const Options &opt) {
    if (opt.demo.empty()) return false;
    DemoResult r;
    std::string name = opt.demo;
    if (name == "course") r = CourseScheduler::Sample().run(opt.algo);
    else if (name == "task") r = TaskDependencyManager::Sample().run(opt.algo);
    else if (name == "package") r = PackageResolver::Sample().run(opt.algo);
    else if (name == "social") r = SocialHierarchyAnalysis::Sample().run(opt.algo);
    else {
        std::cerr << "Unknown demo: " << name << "\n";
        return true;
    }

    if (opt.format == "json") {
        std::stringstream ss;
        ss << "{\"demo\":\"" << name << "\",";
        ss << "\"algorithm\":\"" << opt.algo << "\",";
        ss << "\"has_cycle\":" << (r.has_cycle ? "true" : "false") << ',';
        ss << "\"topo\":" << (r.has_cycle ? std::string("null") : to_json_array(to_int_vector(r.order)));
        if (opt.with_layout && !r.has_cycle) ss << ",\"layout\":" << layout_to_json(r.layout);
        ss << "}";
        std::cout << ss.str() << "\n";
    } else {
        std::cout << "Demo: " << name << " using " << opt.algo << '\n';
        if (r.has_cycle) {
            std::cout << "Cycle detected, ordering unavailable.\n";
        } else {
            std::cout << "Topological order:";
            for (auto v : r.order) std::cout << ' ' << v;
            std::cout << '\n';
            if (opt.with_layout) {
                std::cout << "Layout (id,x,y,z,layer):";
                for (const auto &p : r.layout) {
                    std::cout << " [" << p.id << ':' << p.x << ',' << p.y << ',' << p.z << " l=" << p.layer << ']';
                }
                std::cout << '\n';
            }
        }
    }
    return true;
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Options opt = parse_options(argc, argv);
    if (run_demo(opt)) return 0;

    std::ostringstream ss;
    ss << std::cin.rdbuf();
    std::string raw = ss.str();
    if (raw.empty()) {
        std::cerr << "No input provided. Expecting edge list or CSR text.\n";
        return 1;
    }

    GraphDataStore store;
    std::string err;
    if (!store.load_from_text(raw, err)) {
        std::cerr << "Input invalid: " << err << "\n";
        return 1;
    }

    CompressedGraph graph;
    graph.build_from_adj(store.adjacency());
    graph.build_varint(); // materialize compressed view for memory report

    if (opt.algo == "both") {
        DFSTopoSolver dfs(graph);
        KahnTopoSolver kahn(graph);
        emit_result(dfs.name(), dfs, graph, opt.format, opt.with_layout);
        emit_result(kahn.name(), kahn, graph, opt.format, opt.with_layout);
        return 0;
    }

    if (opt.algo == "dfs") {
        DFSTopoSolver solver(graph);
        emit_result(solver.name(), solver, graph, opt.format, opt.with_layout);
    } else if (opt.algo == "kahn") {
        KahnTopoSolver solver(graph);
        emit_result(solver.name(), solver, graph, opt.format, opt.with_layout);
    } else if (opt.algo == "parallel") {
        ParallelKahnSolver solver(graph, 1);
        emit_result(solver.name(), solver, graph, opt.format, opt.with_layout);
    } else if (opt.algo == "lexi_min") {
        LexicographicKahnSolver solver(graph, true);
        emit_result(solver.name(), solver, graph, opt.format, opt.with_layout);
    } else if (opt.algo == "lexi_max") {
        LexicographicKahnSolver solver(graph, false);
        emit_result(solver.name(), solver, graph, opt.format, opt.with_layout);
    } else if (opt.algo == "incremental") {
        IncrementalTopoSolver solver(graph);
        emit_result(solver.name(), solver, graph, opt.format, opt.with_layout);
    } else {
        std::cerr << "Unknown algorithm: " << opt.algo << "\n";
        return 1;
    }
    return 0;
}


