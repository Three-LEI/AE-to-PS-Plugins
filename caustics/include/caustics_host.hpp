#pragma once
#include "caustics_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace caustics {
inline constexpr unsigned long magic=0x5A435531;
inline constexpr psui::PluginInfo plugin_info{"Caustics 焦散","0.2-local"};
inline constexpr auto title=L"Caustics 焦散";
inline constexpr auto binary_name=L"ZaYuCaustics.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,const std::vector<psui::HostLayer>&,Parameters&,Image&);
}
