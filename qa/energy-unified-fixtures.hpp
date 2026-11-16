#pragma once
#include "energy-glow-fixtures.hpp"
inline energy::Parameters unified_fixture(int index,int w,int h){
 const auto geometry=glow_fixture(index,w,h);auto p=energy::unified_preset(index%4,geometry);
 p.seed=17000+index*837;p.evolution=17+index*26;p.alpha_threshold=43;p.glow_seed=27000+index*7919;p.glow_evolution=13+index*11;
 if(index==0){p.width=2.5;p.brightness=.8;p.glow_radius=24;p.halo_radius=3;}
 if(index==4){p.glow_texture=1.1;p.filament_strength=.8;p.glow_distortion=14;p.glow_radius=32;}
 if(index==5){p.core_noise=2;p.distortion=7;p.glow_noise=0;p.glow_texture=0;p.filament_strength=1.1;p.burn_wind=10;p.fire_direction=0;p.color_mix=.4;}
 if(index==6){auto old=energy::material_preset(4,geometry);old.engine=0;p=energy::to_unified(old);}
 if(index==7)p.color_mix=.6;
 return p;
}
