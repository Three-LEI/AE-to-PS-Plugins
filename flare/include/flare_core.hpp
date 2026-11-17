#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace flare {
using Color=std::array<double,3>;
struct Parameters {
 double hx=256,hy=180,px=384,py=384;
 double brightness=1,scale=100,aspect=1,gamma=1,saturation=100,hue=0;
 Color color{1,1,1},hot_color{.72,.85,1},ghost_color{1,1,1};
 double hot_bright=1,hot_size=1,glow=1;
 double ray_bright=.45,ray_length=1,ray_thickness=1,rotation=0;
 int rays=12;
 double ghost_bright=.55,ghost_size=1,ghost_spacing=1,ghost_softness=.18;
 int ghosts=8,blades=6;
 double ring_bright=.4,ring_size=1,chromatic=35;
 double streak_bright=.4,streak_length=1,streak_width=1;
 double atmosphere=0,noise_scale=100;std::uint32_t seed=123;
 double edge_width=0,edge_bright=1.5,center_radius=0,center_bright=1.5;
 int occlusion=0;std::uint64_t map_key=0;bool invert_occlusion=false;
 double occlusion_softness=12;
 int blend=0;double background=100,mix=100;bool preserve_alpha=false;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
// Native document geometry stays constant during reduced-resolution previews.
struct RenderView {double left=0,top=0,width=0,height=0,step=1;};
using Cancel=std::function<bool()>;
struct Design;
Parameters defaults(int width,int height,int left=0,int top=0);
Parameters preset(int index,int width,int height,int left=0,int top=0);
bool valid(const Parameters&);
double visibility(const Image&,const Parameters&);
bool render(const Image&,const Parameters&,Image&,const Image* occluder=nullptr,const Cancel& cancel={},RenderView view={},const Design* design=nullptr);
}
