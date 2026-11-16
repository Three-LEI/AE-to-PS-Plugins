#include "blinds_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace blinds {namespace {
constexpr double eps=1e-7;
double positive(double x){return std::max(0.,x);}
struct Mask {
 double nx,ny,a,b,period,gap,visible;
 explicit Mask(const Parameters& p){
  const double angle=std::remainder(p.direction,360.)*std::numbers::pi/180;
  nx=std::cos(angle);ny=std::sin(angle);if(std::abs(nx)<eps)nx=0;if(std::abs(ny)<eps)ny=0;
  a=std::abs(nx);b=std::abs(ny);period=p.width;gap=period*p.completion*.01;visible=period-gap;
 }
 double first(double u)const{const double k=std::floor(u/period),r=u-k*period;return k*visible+positive(r-gap);}
 double second(double u)const{const double k=std::floor(u/period),r=u-k*period,v=positive(r-gap);return visible*period*k*(k-1)*.5+k*visible*visible*.5+k*visible*r+v*v*.5;}
 double square_cdf(double t)const{
  const double half=(a+b)*.5;if(t<=-half)return 0;if(t>=half)return 1;
  if(a<eps||b<eps)return std::clamp(t/std::max(a,b)+.5,0.,1.);
  auto sq=[](double v){v=positive(v);return v*v;};
  return std::clamp((sq(t+(a+b)*.5)-sq(t+(a-b)*.5)-sq(t+(-a+b)*.5)+sq(t-(a+b)*.5))/(2*a*b),0.,1.);
 }
 double hard(double phase)const{
  phase-=std::floor(phase/period)*period;
  if(period<=2*(a+b)){
   if(a<eps||b<eps){const double w=std::max(a,b);return std::clamp((first(phase+w*.5)-first(phase-w*.5))/w,0.,1.);}
   return std::clamp((second(phase+(a+b)*.5)-second(phase+(a-b)*.5)-second(phase+(-a+b)*.5)+second(phase-(a+b)*.5))/(a*b),0.,1.);
  }
  const double half=(a+b)*.5;
  if(phase>gap+half&&phase<period-half)return 1;
  if(phase>half&&phase<gap-half)return 0;
  double result=0;for(int k=-1;k<=1;k++)result+=square_cdf((k+1)*period-phase)-square_cdf(k*period+gap-phase);
  return std::clamp(result,0.,1.);
 }
 double sample(double u)const{if(gap<=0)return 1;if(visible<=0)return 0;return hard(u);}

};
}
Parameters defaults(int w,int h,int l,int t){Parameters p;p.cx=l+w*.5;p.cy=t+h*.5;return p;}
bool valid(const Parameters& p){
 return std::isfinite(p.completion)&&p.completion>=0&&p.completion<=100&&std::isfinite(p.direction)&&std::abs(p.direction)<=1e9&&std::isfinite(p.width)&&p.width>=.0001&&p.width<=32000&&std::isfinite(p.feather)&&p.feather>=0&&p.feather<=32000&&std::isfinite(p.cx)&&std::isfinite(p.cy)&&std::abs(p.cx)<=1e7&&std::abs(p.cy)<=1e7;
}
double coverage(double x,double y,const Parameters& p){if(!valid(p))return 0;const Mask mask(p);return mask.sample((x-p.cx)*mask.nx+(y-p.cy)*mask.ny);}
namespace {
bool box_pass(const std::vector<float>& source,int w,int h,int radius,double fraction,bool vertical,std::vector<float>& out,const Cancel& cancel){
 out.resize(source.size());const int lines=vertical?w:h,length=vertical?h:w;const double norm=2.*radius+1+2*fraction;
 return pscpu::parallel_ranges(lines,source.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(auto line=first;line<last;line++){
   if(stop())return false;
   auto get=[&](int at){at=std::clamp(at,0,length-1);return double(source[vertical?std::size_t(at)*w+line:line*w+at]);};
   double sum=(radius+1.)*get(0)+std::max(0,radius-length+1)*get(length-1);
   for(int j=1;j<=std::min(radius,length-1);j++)sum+=get(j);
   for(int at=0;at<length;at++){
    const auto index=vertical?std::size_t(at)*w+line:line*w+at;
    out[index]=float(std::clamp((sum+fraction*(get(at-radius-1)+get(at+radius+1)))/norm,0.,1.));
    sum+=get(at+radius+1)-get(at-radius);
   }
  }return true;
 });
}
}
bool render(const Image& source,const Parameters& p,Image& out,const Cancel& cancel){
 pscpu::ParallelScope parallel;
 if(!source.valid()||!valid(p)||(cancel&&cancel()))return false;
 if(p.completion==0){out=source;return true;}
 const Mask mask(p);std::vector<float> alpha(std::size_t(source.width)*source.height);
 // Measured native raster convention: -1/16 pixel at exact strip boundaries.
 constexpr double pixel_center=-1./16;
 std::vector<float> columns;if(mask.ny==0){columns.resize(source.width);for(int x=0;x<source.width;x++)columns[x]=float(mask.sample((source.left+x+pixel_center-p.cx)*mask.nx));}
 if(!pscpu::parallel_ranges(source.height,alpha.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(auto y=first;y<last;y++){if(stop())return false;const double row=(source.top+double(y)+pixel_center-p.cy)*mask.ny;const float value=mask.nx==0?float(mask.sample(row)):0;
   for(int x=0;x<source.width;x++)alpha[y*source.width+x]=mask.nx==0?value:mask.ny==0?columns[x]:float(mask.sample(row+(source.left+x+pixel_center-p.cx)*mask.nx));
  }return true;
 }))return false;
 if(p.feather>0&&p.completion<100){
  // Three fractional box passes independently fit to the native feather profile.
  // Clamp each pass at the input bounds, including wide feathers near the frame.
  const double sigma=.38575*p.feather+.3735,variance=sigma*sigma/3;
  const int radius=std::max(0,int(std::floor((std::sqrt(1+12*variance)-1)*.5)));
  const double fraction=std::clamp((variance*(2*radius+1)-double(radius)*(radius+1)*(2*radius+1)/3)/(2*((radius+1.)*(radius+1.)-variance)),0.,1.);
  std::vector<float> temporary;
  for(int pass=0;pass<3;pass++){
   if(!box_pass(alpha,source.width,source.height,radius,fraction,false,temporary,cancel))return false;
   if(!box_pass(temporary,source.width,source.height,radius,fraction,true,alpha,cancel))return false;
  }
 }
 Image next=source;
 if(!pscpu::parallel_ranges(source.height,alpha.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(auto y=first;y<last;y++){if(stop())return false;for(int x=0;x<source.width;x++){const auto i=y*source.width+x;next.rgba[i*4+3]=source.rgba[i*4+3]*alpha[i];}}return true;
 }))return false;
 out=std::move(next);return true;
}
}
