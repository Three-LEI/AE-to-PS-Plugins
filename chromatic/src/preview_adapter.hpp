#pragma once
// Preview-only unit conversion; native host parameters remain untouched.
inline Parameters draft_parameters(const Parameters& p,const psui::PreviewSpace& s){auto q=p;q.amount/=s.step;q.softness/=s.step;q.defocus_radius/=s.step;for(auto& endpoint:q.warp){endpoint.x/=s.step;endpoint.y/=s.step;}for(auto& channel:q.channels){channel.x/=s.step;channel.y/=s.step;channel.blur/=s.step;}return q;}
