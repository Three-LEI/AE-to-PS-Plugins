#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace etching {
struct Parameters {
 int mode=0,background=0;
 double frequency=50,frequency1=1,frequency2=1,angle=0,angle1=30,angle2=-20;
 double shift_x=0,shift_y=0,sharpness=4,add_width=0,smooth_source=0;
 double wave_amp=.1,wave_frequency=2,warp_amp=.04,warp_smooth=.044;
 double edge_strength=.5,edge_threshold=.3,edge_width=0,edge_sharpness=3;
 double brightness=0,contrast=100,gamma=1,cross_amount=100,cross_start=.25;
 double source_color=0,paper_grain=0,paper_scale=2,mix=100;
 std::array<double,3> dark{0,0,0},light{1,1,1};std::uint32_t seed=123;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
// Original geometry is retained when the input is a reduced preview proxy.
struct RenderView {double left=0,top=0,width=0,height=0,step=1;};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int);
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={},RenderView view={});
}
