#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.cx=s.x(q.cx);q.cy=s.y(q.cy);q.amount/=s.step;if(q.kind==Kind::turbulent)q.size=std::max(2.,q.size/s.step);q.width/=s.step;q.thickness/=s.step;return q;}
