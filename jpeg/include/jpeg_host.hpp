#pragma once
#include "jpeg_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace jpegfx {
inline constexpr psui::PluginInfo plugin_info{"JPEG Damage JPEG 损坏","0.1"};
inline constexpr auto title=L"JPEG Damage JPEG 损坏";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,const std::vector<psui::HostLayer>&,Parameters&,Image&);
}
