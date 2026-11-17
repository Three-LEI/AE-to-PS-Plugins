#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <vector>
namespace halftone {
struct Parameters {
 int shape=0,polarity=0,background=0;
 double spacing=12,angle=30,aspect=1,softness=10,smooth=2;
 double brightness=0,contrast=100,gamma=1,shift_x=0,shift_y=0,mix=100;
 std::array<double,3> dark{0,0,0},light{1,1,1};
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;
 std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int);
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={});
}
