#pragma once
#include "longshadow_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace longshadow {
inline constexpr unsigned long magic=0x5A4C5331;
inline constexpr psui::PluginInfo info{"Long Shadow 长投影","0.3-local"};
inline constexpr auto title=L"Long Shadow 长投影";
inline constexpr auto binary_name=L"ZaYuLongShadow.8bf";
inline Image from_host(psui::LayerPixels&& source){return {source.width,source.height,source.bounds.left,source.bounds.top,std::move(source.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
