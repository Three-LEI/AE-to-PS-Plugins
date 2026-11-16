#pragma once
#include "energy-glow-fixtures.hpp"
inline energy::Parameters presets_fixture(int index,int w,int h){
 constexpr int selected[]={4,6,7,8,10,11,13,15};
 return energy::unified_preset(selected[index],glow_fixture(index,w,h));
}
