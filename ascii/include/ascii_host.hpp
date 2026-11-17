#pragma once
#include "ascii_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace asciifx {
inline constexpr unsigned long magic=0x5A415331;
inline constexpr psui::PluginInfo plugin_info{"ASCII Dither 字符抖动","0.1-local"};
inline constexpr auto title=L"ASCII Dither 字符抖动";
inline constexpr auto binary_name=L"ZaYuASCIIDither.8bf";
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&);
}
