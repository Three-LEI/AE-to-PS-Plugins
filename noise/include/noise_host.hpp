#pragma once
#include "noise_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace noise {
inline constexpr unsigned long magic = 0x5A4E5331;
inline constexpr psui::PluginInfo plugin_info{"Noise 综合杂色", "0.2.1-local"};
inline constexpr auto title = L"Noise 综合杂色";
inline constexpr auto binary_name = L"ZaYuNoise.8bf";

inline Image from_host(psui::LayerPixels&& pixels) {
    return {pixels.width, pixels.height, pixels.bounds.left, pixels.bounds.top, std::move(pixels.rgba)};
}

INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);
}
