#pragma once
#include "glow_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace glow {
#ifdef ZAYU_DEEP_GLOW
inline constexpr Kind effect_kind=Kind::deep;
inline constexpr psui::PluginInfo plugin_info{"Deep Glow 深度发光","0.10"};
inline constexpr auto title=L"Deep Glow 深度发光";
#else
inline constexpr Kind effect_kind=Kind::classic;
inline constexpr psui::PluginInfo plugin_info{"Glow 发光","0.9"};
inline constexpr auto title=L"Glow 发光";
#endif
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,
                   const std::vector<psui::HostLayer>&,Parameters&,Image&,Image&);
}
