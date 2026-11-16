#pragma once
#include "lightfx_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace lightfx {
#if LIGHTFX_KIND==1
inline constexpr Kind effect_kind=Kind::shine;
inline constexpr auto title=L"Shine 放射光束";
inline constexpr auto binary_name=L"ZaYuShine.8bf";
inline constexpr psui::PluginInfo plugin_info{"Shine 放射光束","0.1-local"};
inline constexpr unsigned long magic=0x5A534831;
#else
inline constexpr Kind effect_kind=Kind::starglow;
inline constexpr auto title=L"Starglow 星芒";
inline constexpr auto binary_name=L"ZaYuStarglow.8bf";
inline constexpr psui::PluginInfo plugin_info{"Starglow 星芒","0.1-local"};
inline constexpr unsigned long magic=0x5A535431;
#endif
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,1,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
