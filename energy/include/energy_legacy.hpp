#pragma once
#include "energy_core.hpp"
#include <cstddef>
#include <cstring>
#include <type_traits>
namespace energy::legacy {
struct ParametersV1 {
 int input=0,composite=0;
 double start_x=64,start_y=128,end_x=448,end_y=128;
 double width=3,brightness=1,core_white=85,glow_radius=32,glow_intensity=1;
 std::array<double,3> color{.08,.5,1};
 double trim_start=0,trim_end=100,taper_start=0,taper_end=0,offset_x=0,offset_y=0;
};
struct SavedV1 {unsigned long magic=0x5A454231;ParametersV1 values;};
struct ParametersV2 {
 int input=0,composite=0;
 double start_x=64,start_y=128,end_x=448,end_y=128;
 double width=3,brightness=1,core_white=85,glow_radius=32,glow_intensity=1;
 std::array<double,3> color{.08,.5,1};
 double trim_start=0,trim_end=100,taper_start=0,taper_end=0,offset_x=0,offset_y=0;
 int style=0,detail=3,branches=3;std::uint32_t seed=0;
 double alpha_threshold=50,distortion=24,wavelength=96,evolution=0;
 double branch_length=48,fire_height=96,fire_spread=24,fire_direction=-90;
 std::array<double,3> tip_color{1,.06,.01};
};
struct SavedV2 {unsigned long magic=0x5A454231;ParametersV2 values;};
struct ParametersV3 {
 ParametersV2 prefix;
 int glow_model=0;
 double halo_intensity=2,halo_radius=10,glow_gamma=1,glow_bias=.5;
 std::array<double,4> glow_sizes{1,19.9/60,8./60,2.9/60};
 std::array<double,4> glow_gains{2.6182,.61563,.12917,.001};
};
struct SavedV3 {unsigned long magic=0x5A454231;ParametersV3 values;};
struct ParametersV4 {
 ParametersV3 prefix;
 int glow_detail=4,filament_count=3;std::uint32_t glow_seed=5000;
 double glow_distortion=24,glow_noise_scale=40,glow_width=8;
 double glow_texture=1,filament_strength=.8,glow_evolution=0;
};
struct SavedV4 {unsigned long magic=0x5A454231;ParametersV4 values;};
struct ParametersV5 {
 ParametersV4 prefix;
 int burn_detail=5;std::uint32_t burn_seed=5000;
 double burn_width=10,burn_turbulence=24,burn_scale=56,burn_density=1;
 double burn_filaments=.7,burn_wind=0,burn_evolution=0;
};
struct SavedV5 {unsigned long magic=0x5A454231;ParametersV5 values;};
static_assert(std::is_trivially_copyable_v<Parameters>);
static_assert(sizeof(ParametersV1)==offsetof(Parameters,style));
static_assert(sizeof(ParametersV2)==offsetof(Parameters,glow_model));
static_assert(offsetof(ParametersV3,prefix)==0&&sizeof(ParametersV3)==offsetof(Parameters,glow_detail));
static_assert(offsetof(ParametersV4,prefix)==0&&sizeof(ParametersV4)==offsetof(Parameters,burn_detail));
static_assert(offsetof(ParametersV5,prefix)==0&&sizeof(ParametersV5)==offsetof(Parameters,engine));
#define ENERGY_V5_OFFSET(name) static_assert(offsetof(ParametersV5,name)==offsetof(Parameters,name))
ENERGY_V5_OFFSET(burn_detail);ENERGY_V5_OFFSET(burn_seed);ENERGY_V5_OFFSET(burn_width);ENERGY_V5_OFFSET(burn_turbulence);ENERGY_V5_OFFSET(burn_scale);ENERGY_V5_OFFSET(burn_density);ENERGY_V5_OFFSET(burn_filaments);ENERGY_V5_OFFSET(burn_wind);ENERGY_V5_OFFSET(burn_evolution);
#undef ENERGY_V5_OFFSET
#define ENERGY_V4_OFFSET(name) static_assert(offsetof(ParametersV4,name)==offsetof(Parameters,name))
ENERGY_V4_OFFSET(glow_detail);ENERGY_V4_OFFSET(filament_count);ENERGY_V4_OFFSET(glow_seed);ENERGY_V4_OFFSET(glow_distortion);ENERGY_V4_OFFSET(glow_noise_scale);ENERGY_V4_OFFSET(glow_width);ENERGY_V4_OFFSET(glow_texture);ENERGY_V4_OFFSET(filament_strength);ENERGY_V4_OFFSET(glow_evolution);
#undef ENERGY_V4_OFFSET
#define ENERGY_V3_OFFSET(name) static_assert(offsetof(ParametersV3,name)==offsetof(Parameters,name))
ENERGY_V3_OFFSET(glow_model);ENERGY_V3_OFFSET(halo_intensity);ENERGY_V3_OFFSET(halo_radius);ENERGY_V3_OFFSET(glow_gamma);ENERGY_V3_OFFSET(glow_bias);ENERGY_V3_OFFSET(glow_sizes);ENERGY_V3_OFFSET(glow_gains);
#undef ENERGY_V3_OFFSET
#define ENERGY_OFFSET(name) static_assert(offsetof(ParametersV1,name)==offsetof(Parameters,name))
ENERGY_OFFSET(input);ENERGY_OFFSET(composite);ENERGY_OFFSET(start_x);ENERGY_OFFSET(start_y);ENERGY_OFFSET(end_x);ENERGY_OFFSET(end_y);ENERGY_OFFSET(width);ENERGY_OFFSET(brightness);ENERGY_OFFSET(core_white);ENERGY_OFFSET(glow_radius);ENERGY_OFFSET(glow_intensity);ENERGY_OFFSET(color);ENERGY_OFFSET(trim_start);ENERGY_OFFSET(trim_end);ENERGY_OFFSET(taper_start);ENERGY_OFFSET(taper_end);ENERGY_OFFSET(offset_x);ENERGY_OFFSET(offset_y);
#undef ENERGY_OFFSET
#define ENERGY_V2_OFFSET(name) static_assert(offsetof(ParametersV2,name)==offsetof(Parameters,name))
ENERGY_V2_OFFSET(input);ENERGY_V2_OFFSET(composite);ENERGY_V2_OFFSET(start_x);ENERGY_V2_OFFSET(start_y);ENERGY_V2_OFFSET(end_x);ENERGY_V2_OFFSET(end_y);ENERGY_V2_OFFSET(width);ENERGY_V2_OFFSET(brightness);ENERGY_V2_OFFSET(core_white);ENERGY_V2_OFFSET(glow_radius);ENERGY_V2_OFFSET(glow_intensity);ENERGY_V2_OFFSET(color);ENERGY_V2_OFFSET(trim_start);ENERGY_V2_OFFSET(trim_end);ENERGY_V2_OFFSET(taper_start);ENERGY_V2_OFFSET(taper_end);ENERGY_V2_OFFSET(offset_x);ENERGY_V2_OFFSET(offset_y);ENERGY_V2_OFFSET(style);ENERGY_V2_OFFSET(detail);ENERGY_V2_OFFSET(branches);ENERGY_V2_OFFSET(seed);ENERGY_V2_OFFSET(alpha_threshold);ENERGY_V2_OFFSET(distortion);ENERGY_V2_OFFSET(wavelength);ENERGY_V2_OFFSET(evolution);ENERGY_V2_OFFSET(branch_length);ENERGY_V2_OFFSET(fire_height);ENERGY_V2_OFFSET(fire_spread);ENERGY_V2_OFFSET(fire_direction);ENERGY_V2_OFFSET(tip_color);
#undef ENERGY_V2_OFFSET
inline bool upgrade(const void* data,std::size_t size,Parameters& out){
 if(size==sizeof(SavedV5)){SavedV5 old;std::memcpy(&old,data,sizeof(old));if(old.magic!=0x5A454231||old.values.prefix.prefix.prefix.style>4)return false;Parameters next;std::memcpy(&next,&old.values,sizeof(old.values));reset_unified(next);if(!valid(next))return false;out=next;return true;}
 if(size==sizeof(SavedV4)){SavedV4 old;std::memcpy(&old,data,sizeof(old));if(old.magic!=0x5A454231||old.values.prefix.prefix.style>3)return false;Parameters next;std::memcpy(&next,&old.values,sizeof(old.values));reset_burning(next);if(!valid(next))return false;out=next;return true;}
 if(size==sizeof(SavedV3)){SavedV3 old;std::memcpy(&old,data,sizeof(old));if(old.magic!=0x5A454231||old.values.prefix.style>2)return false;Parameters next;std::memcpy(&next,&old.values,sizeof(old.values));reset_electric(next);reset_burning(next);if(!valid(next))return false;out=next;return true;}
 if(size==sizeof(SavedV2)){SavedV2 old;std::memcpy(&old,data,sizeof(old));if(old.magic!=0x5A454231||old.values.style>2)return false;Parameters next;std::memcpy(&next,&old.values,sizeof(old.values));reset_glow(next);reset_electric(next);reset_burning(next);if(!valid(next))return false;out=next;return true;}
 if(size!=sizeof(SavedV1))return false;SavedV1 old;std::memcpy(&old,data,sizeof(old));if(old.magic!=0x5A454231)return false;
 Parameters next;std::memcpy(&next,&old.values,sizeof(old.values));reset_extension(next);reset_glow(next);reset_electric(next);reset_burning(next);if(!valid(next)||next.input>1)return false;out=next;return true;
}
}
