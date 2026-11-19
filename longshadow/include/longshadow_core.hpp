#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace longshadow {
struct Parameters {
 double angle=45,length=120,opacity=72,expansion=0,fade=0;
 std::array<double,3> color{0.,0.,0.};
 bool shadow_only=false,crop=false,invert_mask=false;
 double crop_left=0,crop_right=100;
 int mask=0; // Off, rectangle, ellipse
 double mask_x=50,mask_y=50,mask_width=100,mask_height=100,mask_feather=0;
 int color_mode=0; // Solid, source stretch
 double source_brightness=100,gap=0;
 double blur_near=0,blur_far=0;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;
 std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::uint64_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int);
// step is the document-pixel size of a draft pixel.
bool render(const Image&,const Parameters&,Image&,const Cancel& ={},double step=1);
}
