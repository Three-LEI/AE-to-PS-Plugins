#pragma once
#include "surface_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace surface {
struct MapBinding {int32 layer_id=0;std::string name;int32 revision=0;};
#ifdef SURFACE_BLOB
inline constexpr Kind effect_kind=Kind::blob;
inline constexpr psui::PluginInfo plugin_info{"Blobbylize 胶状化","0.2"};
inline constexpr auto title=L"Blobbylize 胶状化";
#else
inline constexpr Kind effect_kind=Kind::glass;
inline constexpr psui::PluginInfo plugin_info{"Glass 玻璃","0.2"};
inline constexpr auto title=L"Glass 玻璃";
#endif
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,const std::vector<psui::HostLayer>&,Parameters&,Image&,MapBinding* =nullptr);
}
