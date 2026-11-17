#pragma once
#include "flare_core.hpp"
#include "flare_design.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace flare {
inline constexpr unsigned long magic=0x5A4C4631;
inline constexpr psui::PluginInfo plugin_info{"Lens Flare 镜头耀斑","0.3-local"};
inline constexpr auto title=L"Lens Flare 镜头耀斑";
inline constexpr auto binary_name=L"ZaYuLensFlare.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,const std::vector<psui::HostLayer>&,Parameters&,Image&,Design* design=nullptr);
}
