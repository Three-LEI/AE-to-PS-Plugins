#pragma once
#include "chromatic_core.hpp"
#include <cstddef>
#include <cstring>
namespace chromatic {
// Frozen 0.8 memory layout; never append fields here.
struct ParametersV1 {
 Mode mode=Mode::spectrum;Geometry geometry=Geometry::directional;
 double amount=24,angle=25,mix=100,center_x=.5,center_y=.5;
 bool reverse=false,falloff=false,invert_falloff=false;
 double protect=0,falloff_power=1.5;
 std::array<Channel,3> channels{{{1,0,0,100,0,0},{0,0,0,100,0,0},{-1,0,0,100,0,0}}};
 double softness=.6,dispersion_power=2,hue=0,saturation=100;
 int samples=24;
 bool linear_light=true,highlights_only=false;
 double gamma=2.2,threshold=50,knee=20;
 Edge edge=Edge::transparent;Alpha alpha=Alpha::expand;
 std::uint64_t mask_key=0;
 int mask_mode=1;bool invert_mask=false;
 bool operator==(const ParametersV1&) const = default;
};
struct SavedV1 {unsigned long magic=0x5A434431;ParametersV1 values;};
static_assert(sizeof(ParametersV1)==offsetof(Parameters,warp));
static_assert(offsetof(ParametersV1,channels)==offsetof(Parameters,channels));
static_assert(offsetof(ParametersV1,invert_mask)==offsetof(Parameters,invert_mask));
inline bool upgrade_parameters(const void* data,int size,Parameters& p){
 if(!data||size!=sizeof(SavedV1))return false;SavedV1 old;std::memcpy(&old,data,sizeof(old));if(old.magic!=0x5A434431||int(old.values.mode)<0||int(old.values.mode)>1)return false;
 Parameters next;std::memcpy(&next,&old.values,sizeof(old.values));if(!valid(next))return false;p=next;return true;
}
}
