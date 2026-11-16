#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.softness/=s.step;q.height/=s.step;q.light_x=s.x(q.light_x);q.light_y=s.y(q.light_y);return q;}
