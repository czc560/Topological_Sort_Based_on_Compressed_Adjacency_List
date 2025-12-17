#pragma once

#include "compressed_graph.hpp"

#include <string>
#include <vector>

struct LayoutPoint {
    uint32_t id;
    float x;
    float y;
    float z;
    uint32_t layer;
};

// Compute layer depths from a given topological order: layer[v] = max layer[u]+1 over incoming edges.
std::vector<uint32_t> compute_layers(const GraphInterface &g, const std::vector<uint32_t> &topo);

// Map layers to 3D coordinates (radial per layer, depth on z axis). Suitable for 3D/VR visualization.
std::vector<LayoutPoint> make_layered_layout(const GraphInterface &g,
                                            const std::vector<uint32_t> &topo,
                                            float layer_gap = 1.5f,
                                            float radius_base = 2.0f,
                                            float radius_step = 1.0f);

// Serialize layout points to JSON: [{"id":0,"x":..,"y":..,"z":..,"layer":0}, ...]
std::string layout_to_json(const std::vector<LayoutPoint> &pts);
