#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.resolution=std::max(1.,q.resolution/s.step);q.block_shift/=s.step;q.mask_blur/=s.step;return q;}
