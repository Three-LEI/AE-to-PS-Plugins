#pragma once
#include "../../native/common/pixel_blend.hpp"
#include <functional>
#include <vector>
namespace patterns {
enum class Kind:int {grid,checker};
enum class SizeFrom:int {corner,width,width_height};
using Blend=pscore::Blend;
using Pixel=pscore::RGBA;
struct Parameters {
 double anchor_x=0,anchor_y=0,corner_x=0,corner_y=0,width=32,height=24;
 double border=5,feather_x=0,feather_y=0,opacity=100,rotation=0;
 double color[3]={1,1,1};
 SizeFrom size_from=SizeFrom::corner;Blend blend=Blend::none;bool invert=false;
 bool operator==(const Parameters&) const = default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid() const {return width>0&&height>0&&rgba.size()==std::size_t(width)*height*4;}
};
struct Region {double x=0,y=0,step=1;};
using Cancel=std::function<bool()>;
Parameters defaults(Kind,double width,double height,double left=0,double top=0);
Parameters preset(Kind,int,double width,double height);
bool valid(const Parameters&);
// Pixel centres use integer document coordinates, matching the AE generators.
double coverage(Kind,const Parameters&,double x,double y);
Pixel pixel(Kind,const Parameters&,double x,double y,Pixel source={},double footprint=1,bool locked_alpha=false);
bool render(Kind,const Parameters&,Region,int width,int height,Image&,const std::vector<float>* source=nullptr,const Cancel& cancel={},bool locked_alpha=false);
}
