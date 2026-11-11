#pragma once
#include "displacement_core.hpp"
#include "psui/host_layers.hpp"
namespace displacement {
inline Image from_host(psui::LayerPixels&& p){
  Image result;result.left=p.bounds.left;result.top=p.bounds.top;
  result.width=p.width;result.height=p.height;
  result.step=double(p.bounds.right-p.bounds.left)/p.width;
  result.midpoint=p.depth==8?128./255:.5;
  result.rgba=std::move(p.rgba);return result;
}
inline VRect layer_bounds(const psui::HostLayer& l){return l.rgb[0]->bounds;}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const std::vector<psui::HostLayer>&,
                    const Image&,Parameters&,Image&);
}
