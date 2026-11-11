#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>
namespace psui {
// Outline pipette: rounded bulb/collar, long hollow barrel and tapered tip.
// Coordinates are defined along the pipette axis, then rotated 45 degrees.
// Keeping this vector-based preserves theme colour and high-DPI antialiasing.
inline void draw_eyedropper_icon(ImDrawList* draw,ImVec2 center,float size,ImU32 ink){
 const float unit=size/24.f,stroke=std::max(1.f,1.35f*unit);
 const ImVec2 origin{center.x-12*unit,center.y-12*unit};
 const auto p=[&](float u,float v){return ImVec2(origin.x+(3+(u+v)*.70710678f)*unit,
                                              origin.y+(21+(-u+v)*.70710678f)*unit);};
 // The slim neck flows into a round rubber bulb; no solid, blocky cap.
 draw->PathLineTo(p(18.5f,-1));draw->PathLineTo(p(20.1f,-1));
 draw->PathBezierCubicCurveTo(p(20.5f,-2.8f),p(23.0f,-3.4f),p(24.3f,-1.8f));
 draw->PathBezierCubicCurveTo(p(26.0f,.3f),p(23.7f,3.3f),p(21.7f,2.5f));
 draw->PathBezierCubicCurveTo(p(20.8f,2.2f),p(20.2f,1.5f),p(20.1f,1));
 draw->PathLineTo(p(18.5f,1));draw->PathStroke(ink,ImDrawFlags_None,stroke);
 // Rotated collar with rounded corners, matching the reference silhouette.
 constexpr float lo=15.5f,hi=18.7f,half=3.5f,r=.75f,k=.55228475f;
 draw->PathLineTo(p(lo+r,-half));draw->PathLineTo(p(hi-r,-half));
 draw->PathBezierCubicCurveTo(p(hi-r+k*r,-half),p(hi,-half+r-k*r),p(hi,-half+r));
 draw->PathLineTo(p(hi,half-r));
 draw->PathBezierCubicCurveTo(p(hi,half-r+k*r),p(hi-r+k*r,half),p(hi-r,half));
 draw->PathLineTo(p(lo+r,half));
 draw->PathBezierCubicCurveTo(p(lo+r-k*r,half),p(lo,half-r+k*r),p(lo,half-r));
 draw->PathLineTo(p(lo,-half+r));
 draw->PathBezierCubicCurveTo(p(lo,-half+r-k*r),p(lo+r-k*r,-half),p(lo+r,-half));
 draw->PathStroke(ink,ImDrawFlags_Closed,stroke);
 // Leave the barrel open at the collar; the nib has a rounded, tapered end.
 draw->PathLineTo(p(lo,-2.1f));draw->PathLineTo(p(4.5f,-2.1f));
 draw->PathLineTo(p(.9f,-.6f));
 draw->PathBezierCubicCurveTo(p(-.4f,-.1f),p(-.4f,.5f),p(.9f,.8f));
 draw->PathLineTo(p(4.5f,2.1f));draw->PathLineTo(p(lo,2.1f));
 draw->PathStroke(ink,ImDrawFlags_None,stroke);
}
} // namespace psui
