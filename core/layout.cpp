#include "layout.hpp"

#include <cmath>
#include <sstream>

std::vector<uint32_t> compute_layers(const GraphInterface &g, const std::vector<uint32_t> &topo) {
    size_t n = g.node_count();
    std::vector<uint32_t> layer(n, 0);
    for (uint32_t u : topo) {
        auto span = g.neighbor_span(u);
        for (auto it = span.first; it != span.second; ++it) {
            uint32_t v = *it;
            uint32_t cand = layer[u] + 1;
            if (cand > layer[v]) layer[v] = cand;
        }
    }
    return layer;
}

std::vector<LayoutPoint> make_layered_layout(const GraphInterface &g,
                                            const std::vector<uint32_t> &topo,
                                            float layer_gap,
                                            float radius_base,
                                            float radius_step) {
    size_t n = g.node_count();
    std::vector<uint32_t> layer = compute_layers(g, topo);
    uint32_t max_layer = 0;
    for (auto v : layer) if (v > max_layer) max_layer = v;

    std::vector<uint32_t> per_layer(max_layer + 1, 0);
    for (auto l : layer) per_layer[l]++;

    std::vector<uint32_t> offset(max_layer + 1, 0);
    for (uint32_t l = 1; l <= max_layer; ++l) offset[l] = offset[l - 1] + per_layer[l - 1];

    std::vector<uint32_t> seq_count(max_layer + 1, 0);
    std::vector<LayoutPoint> pts;
    pts.reserve(n);
    for (uint32_t v : topo) {
        uint32_t l = layer[v];
        uint32_t idx_in_layer = seq_count[l]++;
        uint32_t count = per_layer[l] == 0 ? 1 : per_layer[l];
        float angle = static_cast<float>(idx_in_layer) / static_cast<float>(count) * 6.2831853f;
        float radius = radius_base + radius_step * static_cast<float>(l);
        float x = radius * std::cos(angle);
        float y = radius * std::sin(angle);
        float z = -layer_gap * static_cast<float>(l);
        pts.push_back(LayoutPoint{v, x, y, z, l});
    }
    return pts;
}

std::string layout_to_json(const std::vector<LayoutPoint> &pts) {
    std::ostringstream oss;
    oss << '[';
    for (size_t i = 0; i < pts.size(); ++i) {
        const auto &p = pts[i];
        if (i) oss << ',';
        oss << "{\"id\":" << p.id
            << ",\"x\":" << p.x
            << ",\"y\":" << p.y
            << ",\"z\":" << p.z
            << ",\"layer\":" << p.layer << '}';
    }
    oss << ']';
    return oss.str();
}
