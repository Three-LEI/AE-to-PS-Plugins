#pragma once
#include "blinds_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace blinds {
inline constexpr unsigned long magic=0x5A574231;
inline constexpr psui::PluginInfo plugin_info{"Venetian Blinds 百叶窗","0.1-local"};
inline constexpr auto title=L"Venetian Blinds 百叶窗";
inline constexpr auto binary_name=L"ZaYuVenetianBlinds.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
