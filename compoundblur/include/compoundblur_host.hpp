#pragma once

#include "compoundblur_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
#include <memory>

namespace compoundblur {

inline constexpr unsigned long magic = 0x5A434D31;
inline constexpr psui::PluginInfo plugin_info{"Compound Blur 复合模糊", "0.1.1-local"};
inline constexpr auto title = L"Compound Blur 复合模糊";

struct Inputs {
    Rect domain;
    std::shared_ptr<const Image> reference;
};

inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}

inline Rect document_bounds(FilterRecord* host) {
    const auto bounds = host->documentInfo->bounds;
    return {bounds.left, bounds.top, bounds.right, bounds.bottom};
}

inline bool unique_layer(const std::vector<psui::HostLayer>& layers, std::uint64_t key) {
    return !key || std::count_if(layers.begin(), layers.end(), [key](const auto& layer) { return layer.name_key == key; }) == 1;
}

OSErr load_reference(FilterRecord*, const std::vector<psui::HostLayer>&, std::uint64_t key,
                     std::shared_ptr<const Image>&);
INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, const std::vector<psui::HostLayer>&,
                    Parameters&, Inputs&);

} // namespace compoundblur
