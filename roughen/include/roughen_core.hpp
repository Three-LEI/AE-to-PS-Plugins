#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace roughen {
struct Parameters {
 int type=0;double border=8,sharpness=1,influence=1,scale=100,stretch=0;
 double offset_x=0,offset_y=0,complexity=2,evolution=0,cycles=1,seed=0;
 std::array<double,3> color{.6,.2,0};bool cycle=false;double mix=100;
 bool operator==(const Parameters&)const=default;
};
struct Image {int width=0,height=0,left=0,top=0;std::vector<float> rgba;bool valid()const{return width>0&&height>0&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}};
// Per-invocation cache. The source image must remain immutable while reused.
struct Prepared {const float* source=nullptr;int width=0,height=0,left=0,top=0,fw=0,fh=0;double border=-1;std::vector<float> field;};
using Cancel=std::function<bool()>;
Parameters defaults();
bool valid(const Parameters&);
bool render(const Image&,const Parameters&,Image&,Prepared* cache=nullptr,const Cancel& cancel={});
}
