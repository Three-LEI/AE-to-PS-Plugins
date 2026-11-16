#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.amount/=s.step;q.map_softness/=s.step;q.ridge=std::min(100.,q.ridge*s.step);return q;}
