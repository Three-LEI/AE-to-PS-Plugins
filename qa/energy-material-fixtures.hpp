#pragma once
#include "energy_core.hpp"
inline energy::Parameters energy_fixture(int index,int w,int h){
 auto p=energy::defaults(w,h);p.composite=1;p.width=2.5;p.glow_radius=18;p.brightness=.85;p.core_white=78;p.glow_intensity=.9;p.start_x=30;p.end_x=290;p.start_y=p.end_y=175;
 p.seed=12345u+std::uint32_t(index)*3571u;p.evolution=38.5+index*17;p.detail=4;p.distortion=19;p.wavelength=73;p.branches=4;p.branch_length=39;p.alpha_threshold=43;
 if(index==0)p.input=2;
 if(index==1)p.input=3;
 if(index==2){p.input=0;p.style=1;}
 if(index==3){p.input=2;p.style=1;p.branches=5;}
 if(index==4||index==5||index==7){p.input=index==4?0:index==5?3:2;p.style=2;p.color={1,.48,.035};p.tip_color={.95,.035,.008};p.fire_height=83;p.fire_spread=23;p.fire_direction=-83;p.glow_radius=21;}
 if(index==6){p.input=1;p.style=1;p.composite=0;p.trim_start=7;p.trim_end=94;p.taper_start=8;p.taper_end=15;p.offset_x=2;p.offset_y=-1;}
 return p;
}
