#pragma once

#include "texturewrap_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/image_bounds.hpp"
#include "psui/plugin_info.hpp"
#include <optional>

namespace texturewrap {

#ifdef ZAYU_CYLINDER
inline constexpr auto effect_shape = Shape::cylinder;
inline constexpr unsigned long magic = 0x5A435931;
inline constexpr psui::PluginInfo plugin_info{"Cylinder 圆柱贴图", "0.6-local"};
inline constexpr auto title = L"Cylinder 圆柱贴图";
inline constexpr auto binary_name = L"ZaYuCylinder.8bf";
#else
inline constexpr auto effect_shape = Shape::sphere;
inline constexpr unsigned long magic = 0x5A535031;
inline constexpr psui::PluginInfo plugin_info{"Sphere 球面贴图", "0.5-local"};
inline constexpr auto title = L"Sphere 球面贴图";
inline constexpr auto binary_name = L"ZaYuSphere.8bf";
#endif

struct Inputs {
    Bounds canvas;
    std::optional<Bounds> texture;
    Image reference;
    std::optional<Bounds> current_texture;

    Mapping mapping() const {
        return {canvas, texture.value_or(Bounds{}), reference.valid() ? &reference : nullptr, current_texture};
    }
};

inline Bounds bounds(VRect rect) { return {rect.left, rect.top, rect.right, rect.bottom}; }

inline Bounds canvas_bounds(const FilterRecord* host) {
    return host->documentInfo ? bounds(host->documentInfo->bounds) : Bounds{};
}

inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}

OSErr load_texture_source(FilterRecord*, const std::vector<psui::HostLayer>&, std::uint64_t, Inputs&);
INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, const std::vector<psui::HostLayer>&, Parameters&, Inputs&);

} // namespace texturewrap
