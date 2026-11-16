#pragma once
#include "energy-material-fixtures.hpp"
inline energy::Parameters glow_fixture(int index,int w,int h){
 auto p=energy_fixture(index,w,h);p.glow_model=1;
 p.glow_radius=39+index*3;p.halo_radius=5.5+index*.6;p.halo_intensity=.65+index*.13;p.glow_gamma=.85+index*.09;p.glow_bias=.3+index*.07;
 p.glow_sizes={1.1,.43,.17,.035};p.glow_gains={1.2,.57,.21,.035};
 return p;
}
