#pragma once
#include "repetile_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace repetile {
inline constexpr unsigned long magic = 0x5A525431;
inline constexpr psui::PluginInfo plugin_info{"RepeTile 重复拼贴", "0.2-local"};
inline constexpr auto title = L"RepeTile 重复拼贴";
inline constexpr auto binary_name = L"ZaYuRepeTile.8bf";
inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}
INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);
} // namespace repetile
