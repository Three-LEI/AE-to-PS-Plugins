#pragma once
#include "dither_core.hpp"
#include <algorithm>
#include <cmath>
namespace dither::preview {
using RGBA=std::array<float,4>;
inline RGBA sample_area(const Image& image,double left,double top,double right,double bottom,bool premultiplied=true){
 const double l=std::clamp(left,0.,double(image.width)),t=std::clamp(top,0.,double(image.height)),r=std::clamp(right,0.,double(image.width)),b=std::clamp(bottom,0.,double(image.height));
 if(r<=l||b<=t)return {};double sum[4]{},total=0;
 for(int y=int(std::floor(t));y<int(std::ceil(b));y++)for(int x=int(std::floor(l));x<int(std::ceil(r));x++){
  const double weight=std::max(0.,std::min(r,x+1.)-std::max(l,double(x)))*std::max(0.,std::min(b,y+1.)-std::max(t,double(y)));const auto i=(std::size_t(y)*image.width+x)*4;const double a=std::isfinite(image.rgba[i+3])?std::clamp(double(image.rgba[i+3]),0.,1.):0;
  total+=weight;sum[3]+=a*weight;for(int k=0;k<3;k++)sum[k]+=(std::isfinite(image.rgba[i+k])?double(image.rgba[i+k]):0)*weight*(premultiplied?a:1);
 }
 const double divisor=premultiplied?sum[3]:total;return {divisor>0?float(sum[0]/divisor):0,divisor>0?float(sum[1]/divisor):0,divisor>0?float(sum[2]/divisor):0,total>0?float(sum[3]/total):0};
}
}
