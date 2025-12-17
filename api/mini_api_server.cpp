#include <iostream>

// Minimal placeholder API server. To enable, compile this file with a socket library
// and define ENABLE_MINI_API. This is a synchronous mock to demonstrate REST shape.
#ifdef ENABLE_MINI_API
#include <string>
#include <vector>
#include "../core/compressed_graph.hpp"
#include "../core/layout.hpp"
#include "../core/toposort.hpp"

int main() {
    std::cout << "Mini API stub. In production, use Drogon/Crow with POST /sort accepting JSON edges." << std::endl;
    return 0;
}
#else
int main() {
    std::cout << "ENABLE_MINI_API not defined. Build with -DENABLE_MINI_API to run stub." << std::endl;
    return 0;
}
#endif
