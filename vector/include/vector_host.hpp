#pragma once
#include "vector_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace vectorblur {
inline constexpr psui::PluginInfo plugin_info{"Vector Blur 矢量模糊","0.7"};
inline constexpr auto title=L"Vector Blur 矢量模糊";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,const std::vector<psui::HostLayer>&,Parameters&,Image&);
}
