#pragma once
#include "dither_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace dither {
inline constexpr unsigned long magic=0x5A445431;
inline constexpr psui::PluginInfo plugin_info{"Dither 像素抖动","0.1-local"};
inline constexpr auto title=L"Dither 像素抖动";
inline constexpr auto binary_name=L"ZaYuDither.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
