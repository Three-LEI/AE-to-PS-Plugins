#pragma once
#include "library_panel.hpp"
#include <cmath>
namespace psui {
// Compact point and native Photoshop colour picker controls for generators.
// These add no HWND children or drawing surface, and share the panel's theme/DPI.
inline bool point_control(LibraryPanel& ui,const char* label,double& x,double& y,bool enabled=true){
 bool changed=false;ImGui::PushID(label);ImGui::BeginDisabled(!enabled);
 if(ImGui::BeginTable("point",3,ImGuiTableFlags_SizingStretchProp)){
  ImGui::TableSetupColumn("label",ImGuiTableColumnFlags_WidthStretch,.43f);
  ImGui::TableSetupColumn("x",ImGuiTableColumnFlags_WidthStretch,.285f);
  ImGui::TableSetupColumn("y",ImGuiTableColumnFlags_WidthStretch,.285f);
  ImGui::TableNextColumn();changed|=ui.point_label(label,x,y);
  ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);changed|=ui.numeric_value("##x",x,-10000000.,10000000.,1);ui.mark((std::string(label)+".x").c_str());
  ImGui::TableNextColumn();ImGui::SetNextItemWidth(-1);changed|=ui.numeric_value("##y",y,-10000000.,10000000.,1);ui.mark((std::string(label)+".y").c_str());
  ImGui::EndTable();
 }
 if(changed){if(!std::isfinite(x))x=0;if(!std::isfinite(y))y=0;x=std::clamp(x,-10000000.,10000000.);y=std::clamp(y,-10000000.,10000000.);}
 ImGui::EndDisabled();ImGui::PopID();return changed;
}
inline bool color_control(LibraryPanel& ui,FilterRecord* host,const char* label,double rgb[3]){
 bool changed=false;ImGui::PushID(label);
 if(ImGui::BeginTable("colour",2,ImGuiTableFlags_SizingStretchProp)){
  ImGui::TableSetupColumn("label",ImGuiTableColumnFlags_WidthStretch,.43f);ImGui::TableSetupColumn("value",ImGuiTableColumnFlags_WidthStretch,.57f);
  ImGui::TableNextColumn();changed|=ui.parameter_label(label,rgb,3*sizeof(double));ImGui::TableNextColumn();
  changed|=ui.color_swatch(label,host,rgb,{60*ui.scale(),21*ui.scale()});
  ImGui::EndTable();
 }ImGui::PopID();return changed;
}
}
