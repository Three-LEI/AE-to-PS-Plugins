#pragma once
#include "halftone_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace halftone {
inline constexpr unsigned long magic=0x5A485431;
inline constexpr psui::PluginInfo plugin_info{"Halftone 网点印刷","0.1-local"};
inline constexpr auto title=L"Halftone 网点印刷";
inline constexpr auto binary_name=L"ZaYuHalftone.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
