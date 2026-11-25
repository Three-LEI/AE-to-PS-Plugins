#pragma once

#include "pixelsort_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace pixelsort {

inline constexpr unsigned long magic = 0x5A505331;
inline constexpr psui::PluginInfo plugin_info{"Pixel Sort 像素排序", "0.1-local"};
inline constexpr auto title = L"Pixel Sort 像素排序";

inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}

INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);

} // namespace pixelsort
