#pragma once
#include "filmdamage_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace filmdamage {
inline constexpr unsigned long magic=0x5A464431;
inline constexpr psui::PluginInfo plugin_info{"Film Damage 胶片损伤","0.5-local"};
inline constexpr auto title=L"Film Damage 胶片损伤";
inline constexpr auto binary_name=L"ZaYuFilmDamage.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
