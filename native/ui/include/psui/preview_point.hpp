#pragma once
#include "library_panel.hpp"
#include <cmath>
namespace psui {
// Right-button drag leaves the shared left-button preview pan untouched.
inline bool preview_point(LibraryPanel& ui,const PreviewView& view,double& x,double& y){
 const auto r=ui.items.find("image");if(r==ui.items.end()||ui.picking_color())return false;const auto a=r->second;
 const ImVec2 lo{a.x,a.y},hi{a.z,a.w},mouse=ImGui::GetMousePos();const double zoom=view.percent*.01;
 bool changed=false;if(zoom>0&&ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect(lo,hi)&&ImGui::IsMouseDown(ImGuiMouseButton_Right)){
  const double nx=std::clamp(view.center_x()+(mouse.x-(a.x+a.z)*.5)/zoom,-1e7,1e7),ny=std::clamp(view.center_y()+(mouse.y-(a.y+a.w)*.5)/zoom,-1e7,1e7);
  changed=x!=nx||y!=ny;x=nx;y=ny;
 }
 const ImVec2 p{float((a.x+a.z)*.5+(x-view.center_x())*zoom),float((a.y+a.w)*.5+(y-view.center_y())*zoom)};
 auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(lo,hi,true);const auto color=IM_COL32(57,169,255,255);const float radius=7*ui.scale();
 draw->AddCircle(p,radius,color,20,1.5f*ui.scale());draw->AddLine({p.x-radius-4,p.y},{p.x+radius+4,p.y},color);draw->AddLine({p.x,p.y-radius-4},{p.x,p.y+radius+4},color);draw->PopClipRect();
 ui.items["effect.center"]={p.x,p.y,float(x),float(y)};return changed;
}
} // namespace psui
