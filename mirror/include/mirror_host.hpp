#pragma once
#include "mirror_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace mirror {
inline constexpr unsigned long magic = 0x5A4D5231;
inline constexpr psui::PluginInfo plugin_info{"Mirror 镜像", "0.1-local"};
inline constexpr auto title = L"Mirror 镜像";
inline constexpr auto binary_name = L"ZaYuMirror.8bf";
inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}
INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);
} // namespace mirror
