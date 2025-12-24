#pragma once

#include "compressed_graph.hpp"
#include "layout.hpp"
#include "toposort.hpp"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

struct DemoResult {
    bool has_cycle{false};
    std::vector<uint32_t> order;
    std::vector<LayoutPoint> layout;
};

class CourseScheduler {
public:
    CourseScheduler(size_t course_count, const std::vector<std::pair<uint32_t, uint32_t>> &prereq_edges);
    DemoResult run(const std::string &algo);
    static CourseScheduler Sample();
private:
    size_t n_;
    std::vector<std::pair<uint32_t, uint32_t>> edges_;
};

class TaskDependencyManager {
public:
    TaskDependencyManager(size_t task_count, const std::vector<std::pair<uint32_t, uint32_t>> &edges);
    DemoResult run(const std::string &algo);
    static TaskDependencyManager Sample();
private:
    size_t n_;
    std::vector<std::pair<uint32_t, uint32_t>> edges_;
};

class PackageResolver {
public:
    PackageResolver(size_t pkg_count, const std::vector<std::pair<uint32_t, uint32_t>> &edges);
    DemoResult run(const std::string &algo);
    static PackageResolver Sample();
private:
    size_t n_;
    std::vector<std::pair<uint32_t, uint32_t>> edges_;
};

class SocialHierarchyAnalysis {
public:
    SocialHierarchyAnalysis(size_t people, const std::vector<std::pair<uint32_t, uint32_t>> &edges);
    DemoResult run(const std::string &algo);
    static SocialHierarchyAnalysis Sample();
private:
    size_t n_;
    std::vector<std::pair<uint32_t, uint32_t>> edges_;
};
