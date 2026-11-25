#pragma once

#include "weather_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace weather {

inline constexpr unsigned long magic = 0x5A575431;
inline constexpr psui::PluginInfo plugin_info{"Rain & Snow 雨雪模拟", "0.2-local"};
inline constexpr auto title = L"Rain & Snow 雨雪模拟";

inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}

INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);

} // namespace weather
