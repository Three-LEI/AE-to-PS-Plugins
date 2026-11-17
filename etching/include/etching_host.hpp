#pragma once
#include "etching_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace etching {
inline constexpr unsigned long magic=0x5A455431;
inline constexpr psui::PluginInfo plugin_info{"Etching 蚀刻","0.1-local"};
inline constexpr auto title=L"Etching 蚀刻";
inline constexpr auto binary_name=L"ZaYuEtching.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
