#pragma once
#include "toner_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace toner {
inline constexpr unsigned long magic = 0x5A435431;
inline constexpr psui::PluginInfo plugin_info{"Toner 多色调", "0.1-local"};
inline constexpr auto title = L"Toner 多色调";
inline constexpr auto binary_name = L"ZaYuToner.8bf";
inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}
INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);
} // namespace toner
