#pragma once
#include "creative_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace creative {
#if CREATIVE_KIND==1
inline constexpr Kind effect_kind=Kind::sweep;
inline constexpr psui::PluginInfo plugin_info{"Light Sweep 扫光","0.1"};
inline constexpr auto title=L"Light Sweep 扫光";
inline constexpr auto binary_name=L"ZaYuLightSweep.8bf";
inline constexpr unsigned long magic=0x5A4C5331;
#elif CREATIVE_KIND==2
inline constexpr Kind effect_kind=Kind::kaleida;
inline constexpr psui::PluginInfo plugin_info{"Kaleida 万花筒","0.1"};
inline constexpr auto title=L"Kaleida 万花筒";
inline constexpr auto binary_name=L"ZaYuKaleida.8bf";
inline constexpr unsigned long magic=0x5A4B4131;
#else
inline constexpr Kind effect_kind=Kind::turbulent;
inline constexpr psui::PluginInfo plugin_info{"Turbulent Displace 湍流置换","0.1"};
inline constexpr auto title=L"Turbulent Displace 湍流置换";
inline constexpr auto binary_name=L"ZaYuTurbulentDisplace.8bf";
inline constexpr unsigned long magic=0x5A544431;
#endif
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
