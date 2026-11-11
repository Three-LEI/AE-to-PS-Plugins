#pragma once
#include "roughen_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace roughen {
inline constexpr psui::PluginInfo plugin_info{"Roughen Edges 毛边","0.1"};
inline constexpr auto title=L"Roughen Edges 毛边";
inline constexpr auto binary_name=L"ZaYuRoughenEdges.8bf";
inline constexpr unsigned long magic=0x5A524531;
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&,Prepared&);
}
