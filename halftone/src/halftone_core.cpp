#include "halftone_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace halftone {
namespace {
double unit(double x){return std::clamp(x,0.,1.);}
double finite(float x){return std::isfinite(x)?double(x):0.;}
bool stop(const Cancel& cancel){return cancel&&cancel();}
bool box(std::vector<float>& field,int w,int h,int radius,const Cancel& cancel){
 std::vector<float> tmp(field.size());
 auto at=[&](int x,int y){return field[std::size_t(std::clamp(y,0,h-1))*w+std::clamp(x,0,w-1)];};
 if(!pscpu::parallel_ranges(h,field.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if(cancelled())return false;double sum=0;for(int j=-radius;j<=radius;j++)sum+=at(j,int(y));for(int x=0;x<w;x++){tmp[y*w+x]=float(sum/(2*radius+1));sum+=at(x+radius+1,int(y))-at(x-radius,int(y));}}return true;
 }))return false;
 auto horizontal=[&](int x,int y){return tmp[std::size_t(std::clamp(y,0,h-1))*w+x];};
 return pscpu::parallel_ranges(w,field.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto x=lo;x<hi;x++){if(cancelled())return false;double sum=0;for(int j=-radius;j<=radius;j++)sum+=horizontal(int(x),j);for(int y=0;y<h;y++){field[std::size_t(y)*w+x]=float(sum/(2*radius+1));sum+=horizontal(int(x),y+radius+1)-horizontal(int(x),y-radius);}}return true;
 });
}
struct Screen {
 const Parameters& p;double cs,sn,sx,sy;
 std::array<double,1025> circle{};
 explicit Screen(const Parameters& value):p(value){
  const double a=std::remainder(p.angle,360.)*std::numbers::pi/180;
  cs=std::cos(a);sn=std::sin(a);sx=p.spacing*std::sqrt(p.aspect);sy=p.spacing/std::sqrt(p.aspect);
  // Area rank of a circle clipped to its square cell, including merged dots
  // and shrinking paper holes. The lookup avoids inverse trig per sample.
  for(std::size_t i=0;i<circle.size();i++){
   double r=std::sqrt(.5)*i/(circle.size()-1),area=std::numbers::pi*r*r;
   if(r>.5)area-=4*(r*r*std::acos(.5/r)-.5*std::sqrt(r*r-.25));
   circle[i]=unit(area);
  }
  circle.front()=0;circle.back()=1;
 }
 double rank(double x,double y)const{
  x-=p.shift_x;y-=p.shift_y;
  double u=(x*cs+y*sn)/sx,v=(-x*sn+y*cs)/sy;
  u=std::abs(u-std::floor(u+.5));v=std::abs(v-std::floor(v+.5));
  if(p.shape==1){double r=2*std::max(u,v);return r*r;}
  if(p.shape==2){double r=u+v;return r<=.5?2*r*r:1-2*(1-r)*(1-r);}
  if(p.shape==3)return 2*u;
  const double index=std::min(1024.,std::hypot(u,v)/std::sqrt(.5)*1024);int at=std::min(1023,int(index));return std::lerp(circle[at],circle[at+1],index-at);
 }
 double coverage(double x,double y,double ink)const{
  if(ink<=0)return 0;if(ink>=1)return 1;
  // A narrow axis alone must not erase stretched dots or wide-spaced lines.
  const double period=p.shape==3?sx:std::max(sx,sy);
  if(period<=1)return ink;
  const int samples=(p.shape==3?sx:std::min(sx,sy))<4?4:2;double sum=0;
  for(int j=0;j<samples;j++)for(int i=0;i<samples;i++){
   double delta=ink-rank(x+(i+.5)/samples,y+(j+.5)/samples),edge=p.softness*.0025;
   double value=edge>0?unit(.5+delta/(2*edge)):(delta>=0?1:0);sum+=value*value*(3-2*value);
  }
  return std::lerp(sum/(samples*samples),ink,unit(2-period));
 }
};
}
bool valid(const Parameters& p){
 auto range=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};
 return p.shape>=0&&p.shape<4&&p.polarity>=0&&p.polarity<2&&p.background>=0&&p.background<2&&range(p.spacing,1e-6,1000)&&range(p.angle,-1e9,1e9)&&range(p.aspect,.1,10)&&range(p.softness,0,100)&&range(p.smooth,0,200)&&range(p.brightness,-100,100)&&range(p.contrast,0,400)&&range(p.gamma,.1,5)&&range(p.shift_x,-1e7,1e7)&&range(p.shift_y,-1e7,1e7)&&range(p.mix,0,100)&&std::all_of(p.dark.begin(),p.dark.end(),[&](double x){return range(x,0,1);})&&std::all_of(p.light.begin(),p.light.end(),[&](double x){return range(x,0,1);});
}
Parameters preset(int index){
 Parameters p;
 if(index==1){p.spacing=8;p.softness=0;p.contrast=130;}
 if(index==2){p.spacing=16;p.angle=20;p.dark={.16,.1,.25};p.light={1,.87,.65};p.contrast=115;p.smooth=4;}
 if(index==3){p.polarity=1;p.dark={.015,.035,.08};p.light={.25,.85,1};p.spacing=10;p.brightness=8;}
 if(index==4){p.shape=3;p.spacing=7;p.angle=45;p.softness=0;}
 if(index==5){p.background=1;p.spacing=14;p.contrast=120;p.smooth=3;}
 return p;
}
bool render(const Image& src,const Parameters& p,Image& out,const Cancel& cancel){
 pscpu::ParallelScope pool;
 if(!src.valid()||!valid(p)||stop(cancel))return false;
 if(p.mix==0){out=src;return true;}
 const int w=src.width,h=src.height;const auto size=std::size_t(w)*h;
 std::vector<float> tone(size),weight;
 if(p.smooth>0)weight.resize(size);
 if(!pscpu::parallel_ranges(h,size,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if(cancelled())return false;for(int x=0;x<w;x++){auto n=y*w+x,i=n*4;double v=unit(.2126*finite(src.rgba[i])+.7152*finite(src.rgba[i+1])+.0722*finite(src.rgba[i+2]));if(!weight.empty()){weight[n]=float(unit(finite(src.rgba[i+3])));v*=weight[n];}tone[n]=float(v);}}return true;
 }))return false;
 if(!weight.empty()){
  int radius=int(std::lround(p.smooth*.5));
  if(radius>0)for(int pass=0;pass<3;pass++)if(!box(tone,w,h,radius,cancel)||!box(weight,w,h,radius,cancel))return false;
 }
 Screen screen(p);Image result{w,h,src.left,src.top,std::vector<float>(src.rgba.size())};
 const auto& ink=p.polarity?p.light:p.dark;const auto& paper=p.polarity?p.dark:p.light;const double mix=p.mix*.01;
 if(!pscpu::parallel_ranges(h,size,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if(cancelled())return false;for(int x=0;x<w;x++){
   auto n=y*w+x,i=n*4;double v=tone[n];if(!weight.empty())v=weight[n]>1e-12?v/weight[n]:0;
   v=std::pow(unit((v-.5)*p.contrast*.01+.5+p.brightness*.01),1/p.gamma);
   const double dots=screen.coverage(double(src.left)+x,src.top+double(y),p.polarity?v:1-v),a=unit(finite(src.rgba[i+3])),effect_a=p.background?a*dots:a,alpha=std::lerp(a,effect_a,mix);
   for(int k=0;k<3;k++){double c=p.background?ink[k]:std::lerp(paper[k],ink[k],dots);result.rgba[i+k]=alpha>0?float((finite(src.rgba[i+k])*a*(1-mix)+c*effect_a*mix)/alpha):0;}
   result.rgba[i+3]=float(alpha);
  }}return true;
 }))return false;
 if(stop(cancel))return false;out=std::move(result);return true;
}
}
