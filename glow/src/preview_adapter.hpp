#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.radius/=s.step;q.chromatic_shift/=s.step;return q;}
