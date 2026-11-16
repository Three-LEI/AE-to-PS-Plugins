#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.border/=s.step;q.scale=std::max(10.,q.scale/s.step);q.offset_x=s.x(q.offset_x);q.offset_y=s.y(q.offset_y);return q;}
