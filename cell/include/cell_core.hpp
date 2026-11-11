#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <array>
namespace cell {
enum class Mode:int {bubbles,crystals,plates,static_plates,crystallize,pillow,crystals_hq,plates_hq,static_plates_hq,crystallize_hq,mixed_crystals,tubular,count};
enum class Overflow:int {clip,soft_clamp,wrap_back};
struct Parameters {
 Mode mode=Mode::bubbles;Overflow overflow=Overflow::clip;
 double contrast=100,disperse=1,size=60,offset_x=0,offset_y=0,evolution=0;
 int cells_x=4,cells_y=4,cycle=1,seed=0;
 bool invert=false,tiling=false,cycle_evolution=false;
 bool operator==(const Parameters&) const=default;
};
using Pixel=std::array<float,4>;
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&rgba.size()==std::size_t(width)*height*4;}
};
struct Region {double x=0,y=0,step=1;};
// Source coordinates are document coordinates, independent of output tile/preview ROI.
struct Source {const Image* image=nullptr;double x=0,y=0,step_x=1,step_y=1;};
using Cancel=std::function<bool()>;
Parameters defaults(double width,double height,double left=0,double top=0);
bool valid(const Parameters&);
bool samples_source(Mode);
bool sharpness_mode(Mode);
bool high_quality(Mode);
bool render(const Parameters&,Region,int width,int height,Image&,Source source={},const Cancel& cancel={},bool locked_alpha=false);
}
