#pragma once
#include "chromatic_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace chromatic {
inline constexpr psui::PluginInfo plugin_info{"Chromatic Dispersion 色差色散","0.8"};
inline constexpr auto title=L"Chromatic Dispersion 色差色散";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,
                   const std::vector<psui::HostLayer>&,Parameters&,Image&);
}
