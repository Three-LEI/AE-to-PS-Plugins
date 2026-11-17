#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <vector>
namespace dither {
using Color=std::array<double,3>;
struct Parameters {
 int algorithm=0,pattern=1,palette=0,levels=2; // levels is the menu index: 0 = 2 levels.
 bool linear_light=false,serpentine=true;
 double pixel_size=2,amount=100,brightness=0,contrast=100,gamma=1,saturation=100,shift_x=0,shift_y=0,mix=100;
 std::array<Color,4> colors{{{0,0,0},{1,1,1},{.12,.35,.65},{.85,.95,1}}};
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int);
std::vector<Color> palette_colors(const Parameters&);
// Subpixel blocks are a view-only area approximation. Native host parameters
// require pixel_size >= 1, producing crisp palette-valued output at mix=100.
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={});
}
