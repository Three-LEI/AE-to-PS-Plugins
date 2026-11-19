#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace filmdamage {
struct Parameters {
 double grain=10,grain_size=1,grain_color=35,grain_darks=20,grain_lights=0;
 double saturation=92,contrast=100,exposure=0,fade=3;
 std::array<double,3> tint{1,.98,.94},stain_color{.22,.10,.025};
 int dust=45,hairs=2,scratches=6,stains=3,variation=0,edge=0;
 double dust_size=2,dust_opacity=80,dust_white=10;
 double hair_width=.8,hair_length=140,hair_curl=35,hair_opacity=85;
 double scratch_width=.7,scratch_length=85,scratch_opacity=75,scratch_white=15;
 double scratch_roughness=1.5,scratch_gaps=28,scratch_angle=0,scratch_center=0,scratch_spread=100;
 double stain_size=70,stain_opacity=25;
 double shake=0,tilt=0,defocus=0,flicker=0;
 double vignette=12,vignette_radius=75,vignette_softness=55;
 double leak=0,leak_angle=0,mix=100;
 std::uint32_t seed=123;bool grain_mono=false,flip=false,preserve_alpha=true;
 double grain_softness=0,halation=0,halation_radius=12,halation_threshold=70;
 int damage_variation=0;
 double stain_texture=0;
 double stain_edge_roughness=0;
 double stain_edge_size=100;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
struct RenderView {double left=0,top=0,width=0,height=0,step=1;};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int);
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={},RenderView view={});
}
