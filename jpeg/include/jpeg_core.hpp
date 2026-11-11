#pragma once
#include <cstdint>
#include <functional>
#include <vector>
namespace jpegfx {
struct Parameters {
 double quality=10,resolution=1,aspect=1;
 int chroma=2;double chroma_damage=100;
 double all_freq=1,x_freq=1,y_freq=1,low_freq=1,mid_freq=1,high_freq=1,affect_luma=1,affect_chroma=.5;
 double error_rate=0,density=75,error_amp=1,coherence=1,block_shift=16,color_error=30;
 int error_mode=0,seed=123,variation=0;bool flip_noise=false;
 double lights=1,darks=0,saturation=1,mix=100;bool preserve_hdr=true;
 std::uint64_t mask_key=0;int mask_mode=0;double mask_blur=12;bool invert_mask=false;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int index);
bool render(const Image&,const Parameters&,Image&,const Image* mask=nullptr,const Cancel& cancel={});
}
