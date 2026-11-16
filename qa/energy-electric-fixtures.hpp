#pragma once
#include "energy-glow-fixtures.hpp"
inline energy::Parameters electric_fixture(int index,int w,int h){
 auto p=energy::material_preset(3,glow_fixture(index,w,h));
 p.seed=12345+index*3571;p.evolution=17+index*23;p.alpha_threshold=43;
 p.glow_seed=54321+index*7919;p.glow_detail=2+index%4;p.filament_count=index%7;
 p.glow_distortion=12+index*2;p.glow_noise_scale=31+index*6;p.glow_width=4.5+index*.7;
 p.glow_texture=.6+index*.11;p.filament_strength=.4+index*.13;p.glow_evolution=13+index*11;
 if(index==2)p.distortion=0;
 if(index==3)p.glow_distortion=0;
 if(index==4)p.glow_texture=0;
 return p;
}
