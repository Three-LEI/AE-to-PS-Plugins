#pragma once
#include "energy_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
namespace energy {
inline constexpr auto title=L"Saber 能量光束";
inline constexpr unsigned long magic=0x5A454231;
inline constexpr psui::PluginInfo plugin_info{"Saber 能量光束","0.7-local"};
struct Inputs {Path path;std::string name;};
struct HostPath {int index=0;std::string name;};
inline Image from_host(psui::LayerPixels&& p){return {p.width,p.height,p.bounds.left,p.bounds.top,std::move(p.rgba)};}
std::vector<HostPath> host_paths(FilterRecord*);
bool import_path(FilterRecord*,int,Inputs&,std::string&);
OSErr read_settings(FilterRecord*,Parameters&,Inputs&,bool& interactive);
OSErr write_settings(FilterRecord*,const Parameters&,const Inputs&);
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&,Inputs&);
}
