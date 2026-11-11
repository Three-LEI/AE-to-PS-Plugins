#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace chromatic {
enum class Mode:int {rgb,spectrum};
enum class Geometry:int {directional,radial,tangential};
enum class Edge:int {transparent,repeat,reflect,wrap};
enum class Alpha:int {expand,preserve,black};
struct Channel {
 double amount=0,x=0,y=0,scale=100,rotation=0,blur=0;
 bool operator==(const Channel&) const = default;
};
struct Parameters {
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
 bool operator==(const Parameters&) const = default;
};
struct Image {
 int width=0,height=0,left=0,top=0;
 std::vector<float> rgba; // native document RGB, unassociated alpha
 bool valid() const;
};
using Cancel=std::function<bool()>;
Parameters preset(int index); // 0 lens, 1 RGB, 2 prism, 3 highlight glass
bool valid(const Parameters&);
// Spectrum is an RGB-based artistic approximation, not spectral reconstruction.
// Sampling and softening use associated colour/coverage in one transfer space.
bool render(const Image&,const Parameters&,Image&,const Image* mask=nullptr,const Cancel& cancel={});
}
