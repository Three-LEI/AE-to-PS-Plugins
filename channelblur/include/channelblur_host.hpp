#pragma once

#include "channelblur_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"

namespace channelblur {

inline constexpr unsigned long magic = 0x5A434231;
inline constexpr psui::PluginInfo plugin_info{"Channel Blur 通道模糊", "0.1.1-local"};
inline constexpr auto title = L"Channel Blur 通道模糊";

inline Image from_host(psui::LayerPixels&& p) {
    return {p.width, p.height, p.bounds.left, p.bounds.top, std::move(p.rgba)};
}

INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord*, const Image&, Parameters&);

} // namespace channelblur
