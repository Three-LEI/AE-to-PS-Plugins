#pragma once
#include "energy-glow-fixtures.hpp"
inline energy::Parameters burning_fixture(int index,int w,int h){
 auto p=energy::material_preset(4,glow_fixture(index,w,h));
 p.seed=17000+index*837;p.evolution=17+index*26;p.alpha_threshold=43;
 p.burn_seed=27000+index*7919;p.burn_detail=2+index%5;
 p.burn_width=5.5+index*.7;p.burn_turbulence=12+index*3;p.burn_scale=41+index*7;
 p.burn_density=.6+index*.11;p.burn_filaments=.4+index*.15;p.burn_wind=index?8+index*2:0;
 p.burn_evolution=13+index*11;p.fire_direction=-120+index*25;
 if(index==2)p.burn_density=0;
 if(index==3)p.burn_filaments=0;
 if(index==4)p.burn_turbulence=0;
 if(index==5)p.burn_wind=0;
 return p;
}
