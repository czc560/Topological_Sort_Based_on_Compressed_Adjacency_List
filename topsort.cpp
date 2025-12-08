#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>

#include "core/graph_clean.hpp"

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string algo = "dfs";
    std::string format = "text";
    if (argc >= 2) algo = argv[1];
    if (argc >= 3) format = argv[2];

    int n, m;
    if (!(std::cin >> n >> m)) {
        std::cerr << "读取 n m 失败，请按格式输入顶点数和 list 长度 (或 n m 后跟边列表)。\n";
        return 1;
    }

    std::vector<int> rest;
    int x;
    while (std::cin >> x) rest.push_back(x);

    std::vector<int> h;
    std::vector<int> list;
    bool parsed_compressed = false;

    if (static_cast<int>(rest.size()) >= n + 1) {
        int possible_list_size = rest[n];
        if (possible_list_size >= 0 && possible_list_size == static_cast<int>(rest.size()) - (n + 1)) {
            h.assign(rest.begin(), rest.begin() + (n + 1));
            list.assign(rest.begin() + (n + 1), rest.end());
            parsed_compressed = true;
        }
    }
    if (!parsed_compressed) {
        if (static_cast<int>(rest.size()) == 2 * m) {
            std::vector<std::vector<int>> adj(n);
            for (int i = 0; i < m; ++i) {
                int u = rest[2 * i];
                int v = rest[2 * i + 1];
                if (u < 0 || u >= n || v < 0 || v >= n) {
                    std::cerr << "边端点超出范围: " << u << " -> " << v << "\n";
                    return 1;
                }
                adj[u].push_back(v);
            }
            build_compressed(adj, h, list);
        } else {
            std::cerr << "输入格式无法识别。期望紧缩邻接表 (n m then h[0..n] and list[m]) 或边列表 (n m then m pairs)。\n";
            return 1;
        }
    }

    auto run_and_output = [&](const std::string &which_algo) {
        std::vector<int> topo;
        bool has_cycle = false;
        using clk = std::chrono::high_resolution_clock;
        auto t0 = clk::now();
        if (which_algo == "dfs") {
            has_cycle = topsort_dfs(n, h, list, topo);
        } else if (which_algo == "kahn") {
            has_cycle = topsort_kahn(n, h, list, topo);
        }
        auto t1 = clk::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        if (format == "json") {
            std::stringstream ss;
            ss << "{\"algorithm\":\"" << which_algo << "\",\"has_cycle\":" << (has_cycle ? "true" : "false")
               << ",\"time_ms\":" << ms << ",\"h\":" << to_json_array(h) << ",\"list\":" << to_json_array(list) << ",\"topo\":";
            if (has_cycle) ss << "null";
            else ss << to_json_array(topo);
            ss << "}";
            std::cout << ss.str() << "\n";
        } else {
            if (which_algo == "dfs" || which_algo == "kahn") std::cerr << "Algorithm: " << which_algo << "  Time(ms): " << ms << "\n";
            std::cout << "h:";
            for (int v : h) std::cout << ' ' << v;
            std::cout << '\n';
            std::cout << "list:";
            for (int v : list) std::cout << ' ' << v;
            std::cout << '\n';
            if (has_cycle) std::cout << "Graph has a cycle; topological order does not exist.\n";
            else {
                std::cout << "Topological order:";
                for (int v : topo) std::cout << ' ' << v;
                std::cout << '\n';
            }
        }
    };

    if (algo == "both") {
        run_and_output("dfs");
        run_and_output("kahn");
    } else {
        run_and_output(algo);
    }

    return 0;
}


