#include "demos.hpp"

namespace {
DemoResult solve_demo(size_t n,
                      const std::vector<std::pair<uint32_t, uint32_t>> &edges,
                      const std::string &algo) {
    CompressedGraph g;
    g.build_from_edges(n, edges);
    g.build_varint();

    DemoResult r;
    if (algo == "dfs") {
        DFSTopoSolver solver(g);
        r.has_cycle = solver.run(r.order);
    } else if (algo == "kahn" || algo == "parallel") {
        KahnTopoSolver solver(g);
        r.has_cycle = solver.run(r.order);
    } else if (algo == "lexi_min") {
        LexicographicKahnSolver solver(g, true);
        r.has_cycle = solver.run(r.order);
    } else if (algo == "lexi_max") {
        LexicographicKahnSolver solver(g, false);
        r.has_cycle = solver.run(r.order);
    } else {
        KahnTopoSolver solver(g);
        r.has_cycle = solver.run(r.order);
    }
    if (!r.has_cycle) r.layout = make_layered_layout(g, r.order, 1.5f, 2.0f, 1.2f);
    return r;
}
}

CourseScheduler::CourseScheduler(size_t course_count, const std::vector<std::pair<uint32_t, uint32_t>> &prereq_edges)
    : n_(course_count), edges_(prereq_edges) {}

DemoResult CourseScheduler::run(const std::string &algo) { return solve_demo(n_, edges_, algo); }

CourseScheduler CourseScheduler::Sample() {
    size_t n = 8;
    std::vector<std::pair<uint32_t, uint32_t>> e = {
        {0, 2}, {1, 2}, {2, 3}, {2, 4}, {3, 5}, {4, 6}, {6, 7}
    };
    return CourseScheduler(n, e);
}

TaskDependencyManager::TaskDependencyManager(size_t task_count, const std::vector<std::pair<uint32_t, uint32_t>> &edges)
    : n_(task_count), edges_(edges) {}

DemoResult TaskDependencyManager::run(const std::string &algo) { return solve_demo(n_, edges_, algo); }

TaskDependencyManager TaskDependencyManager::Sample() {
    size_t n = 7;
    std::vector<std::pair<uint32_t, uint32_t>> e = {
        {0, 3}, {1, 3}, {1, 4}, {3, 5}, {4, 5}, {5, 6}
    };
    return TaskDependencyManager(n, e);
}

PackageResolver::PackageResolver(size_t pkg_count, const std::vector<std::pair<uint32_t, uint32_t>> &edges)
    : n_(pkg_count), edges_(edges) {}

DemoResult PackageResolver::run(const std::string &algo) { return solve_demo(n_, edges_, algo); }

PackageResolver PackageResolver::Sample() {
    size_t n = 6;
    std::vector<std::pair<uint32_t, uint32_t>> e = {
        {0, 2}, {1, 2}, {2, 3}, {2, 4}, {4, 5}
    };
    return PackageResolver(n, e);
}

SocialHierarchyAnalysis::SocialHierarchyAnalysis(size_t people, const std::vector<std::pair<uint32_t, uint32_t>> &edges)
    : n_(people), edges_(edges) {}

DemoResult SocialHierarchyAnalysis::run(const std::string &algo) { return solve_demo(n_, edges_, algo); }

SocialHierarchyAnalysis SocialHierarchyAnalysis::Sample() {
    size_t n = 7;
    std::vector<std::pair<uint32_t, uint32_t>> e = {
        {0, 1}, {0, 2}, {1, 3}, {2, 4}, {3, 5}, {4, 5}, {5, 6}
    };
    return SocialHierarchyAnalysis(n, e);
}
