#pragma once
#include "lens_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace lens {
#ifdef LENS_CC
inline constexpr Kind effect_kind=Kind::cc;
inline constexpr psui::PluginInfo plugin_info{"Lens 镜头","0.3"};
inline constexpr auto title=L"Lens 镜头";
#else
inline constexpr Kind effect_kind=Kind::optics;
inline constexpr psui::PluginInfo plugin_info{"Optics Compensation 光学补偿","0.2"};
inline constexpr auto title=L"Optics Compensation 光学补偿";
#endif
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
