#include "lens_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <array>
#include <algorithm>
#include <cmath>
namespace lens {
Parameters defaults(Kind kind,int w,int h,int left,int top){Parameters p;p.kind=kind;p.cx=left+w*.5;p.cy=top+h*.5;return p;}
bool valid(const Parameters& p){return (p.kind==Kind::cc||p.kind==Kind::optics)&&std::isfinite(p.fov)&&p.fov>=0&&p.fov<180&&std::isfinite(p.cx)&&std::isfinite(p.cy)&&std::abs(p.cx)<=1e7&&std::abs(p.cy)<=1e7&&std::isfinite(p.size)&&p.size>=0&&p.size<=500&&std::isfinite(p.convergence)&&p.convergence>=-200&&p.convergence<=100&&p.axis>=0&&p.axis<3;}
namespace {
// Empirical fit of AE's Optimal Pixels zoom (normal direction). It is not a
// public Adobe formula; ordinary/reverse projection does not use this table.
double optimal_zoom(double q){
 constexpr double x[]={0,.07179677,.11218245,.19943547,.33333333,.52083333,.92592593,1,1.5625,2.77777778,3,4.6875,8.33333333};
 constexpr double y[]={1,1.03586188,1.05453913,1.09322408,1.14981052,1.22072612,1.3526835,1.37431642,1.51721748,1.74709708,1.78095239,1.99065854,2.27789521};
 for(int i=1;i<13;++i)if(q<=x[i])return y[i-1]+(y[i]-y[i-1])*(q-x[i-1])/(x[i]-x[i-1]);
 return 1+.5*q/std::pow(1+.5*q,.717);
}
std::array<float,4> sample(const Image& im,double x,double y){
 if(!std::isfinite(x)||!std::isfinite(y)||x<=-1||y<=-1||x>=im.width||y>=im.height)return {};
 const int ix=int(std::floor(x)),iy=int(std::floor(y));const double fx=x-ix,fy=y-iy;std::array<double,4> sum{};
 for(int j=0;j<2;++j)for(int i=0;i<2;++i){const int xx=ix+i,yy=iy+j;if(xx<0||yy<0||xx>=im.width||yy>=im.height)continue;
  const auto n=(std::size_t(yy)*im.width+xx)*4;const double weight=(i?fx:1-fx)*(j?fy:1-fy),alpha=std::clamp(std::isfinite(im.rgba[n+3])?double(im.rgba[n+3]):0.,0.,1.);
  for(int k=0;k<3;++k)sum[k]+=weight*alpha*(std::isfinite(im.rgba[n+k])?im.rgba[n+k]:0);sum[3]+=weight*alpha;
 }
 std::array<float,4> out{};if(sum[3]>1e-12)for(int k=0;k<3;++k)out[k]=float(sum[k]/sum[3]);out[3]=float(sum[3]);return out;
}
}
bool render(const Image& src,const Parameters& p,Image& output,const Cancel& cancel){
 if(!src.valid()||!valid(p)||(cancel&&cancel()))return false;
 if(p.kind==Kind::optics&&p.fov==0){output=src;return true;}
 Image out{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};
 const bool cc=p.kind==Kind::cc;const double cx=p.cx-src.left-(cc?0:.5),cy=p.cy-src.top-(cc?0:.5);
 const double diagonal=std::hypot(src.width,src.height),radius=diagonal*p.size/200;
 const double dimension=p.axis==0?src.width:p.axis==1?src.height:diagonal;
 const double focal=cc?1:dimension*.5/std::tan(p.fov*3.14159265358979323846/360);
 const double q=(diagonal*.5/focal)*(diagonal*.5/focal);
 const double a=p.optimal?(p.reverse?1/(1+q):optimal_zoom(q)):1;
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(int y=int(first);y<int(last);++y){if(stop())return false;for(int x=0;x<src.width;++x){
   const double dx=x-cx,dy=y-cy,r2=dx*dx+dy*dy;double scale=1,coverage=1;
   if(cc){if(radius<=0)continue;coverage=std::clamp((radius-std::sqrt(r2))*std::sqrt(.5),0.,1.);if(coverage==0)continue;scale=1-p.convergence*.01*r2/(radius*radius);}
   else {const double v=a+(p.reverse?1:-1)*r2/(focal*focal);if(v<=0)continue;scale=1/std::sqrt(v);}
   const auto colour=sample(src,cx+dx*scale,cy+dy*scale);const auto n=(std::size_t(y)*src.width+x)*4;
   for(int k=0;k<3;++k)out.rgba[n+k]=colour[k];out.rgba[n+3]=float(colour[3]*coverage);
  }}return true;
 }))return false;
 output=std::move(out);return true;
}
}
