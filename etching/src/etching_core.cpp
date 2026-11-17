#include "etching_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <numbers>
namespace etching {namespace {
constexpr double tau=2*std::numbers::pi;
double unit(double x){return std::clamp(x,0.,1.);}
double clean(float x){return std::isfinite(x)?double(x):0.;}
double smooth(double a,double b,double x){const auto t=unit((x-a)/(b-a));return t*t*(3-2*t);}
bool box(std::vector<float>& field,int w,int h,int radius,const Cancel& cancel){
 if(radius<=0)return true;std::vector<float> tmp(field.size());
 if(!pscpu::parallel_ranges(h,field.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){
  for(auto y=lo;y<hi;y++){if(stop())return false;auto at=[&](int x){return field[y*w+std::clamp(x,0,w-1)];};double sum=0;for(int j=-radius;j<=radius;j++)sum+=at(j);for(int x=0;x<w;x++){tmp[y*w+x]=float(sum/(2*radius+1));sum+=at(x+radius+1)-at(x-radius);}}return true;
 }))return false;
 return pscpu::parallel_ranges(w,field.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){
  for(auto x=lo;x<hi;x++){if(stop())return false;auto at=[&](int y){return tmp[std::size_t(std::clamp(y,0,h-1))*w+x];};double sum=0;for(int j=-radius;j<=radius;j++)sum+=at(j);for(int y=0;y<h;y++){field[std::size_t(y)*w+x]=float(sum/(2*radius+1));sum+=at(y+radius+1)-at(y-radius);}}return true;
 });
}
bool blur(std::vector<float>& values,const std::vector<float>& alpha,int w,int h,double sigma,const Cancel& cancel){
 const int radius=int(std::lround(sigma*.5));if(radius<=0)return true;auto weight=alpha;
 for(std::size_t i=0;i<values.size();i++)values[i]*=weight[i];
 for(int i=0;i<3;i++)if(!box(values,w,h,radius,cancel)||!box(weight,w,h,radius,cancel))return false;
 for(std::size_t i=0;i<values.size();i++)values[i]=weight[i]>1e-12f?values[i]/weight[i]:.5f;return true;
}
bool dilate(std::vector<float>& image,int w,int h,int radius,const Cancel& cancel){
 if(radius<=0)return true;std::vector<float> tmp(image.size());
 auto pass=[&](bool vertical){const int lines=vertical?w:h,length=vertical?h:w;return pscpu::parallel_ranges(lines,image.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){std::deque<std::pair<int,float>> queue;
  for(auto row=lo;row<hi;row++){if(stop())return false;queue.clear();for(int i=-radius;i<length+radius;i++){const auto index=vertical?std::size_t(std::clamp(i,0,h-1))*w+row:row*w+std::clamp(i,0,w-1);const float value=vertical?tmp[index]:image[index];while(!queue.empty()&&queue.back().second<=value)queue.pop_back();queue.emplace_back(i,value);while(queue.front().first<i-2*radius)queue.pop_front();const int x=i-radius;if(x>=0&&x<length){const auto at=vertical?std::size_t(x)*w+row:row*w+x;if(vertical)image[at]=queue.front().second;else tmp[at]=queue.front().second;}}}return true;
 });};return pass(false)&&pass(true);
}
// Exact integral of a periodic stripe: no fixed sample budget can miss thin ink.
double stripe(double phase,double duty,double footprint,double sharpness){
 if(duty<=0)return 0;if(duty>=1)return 1;if(footprint>=2)return duty;
 const double soft=.03/(sharpness+.15),width=std::max(1e-7,std::hypot(footprint,soft));
 auto primitive=[&](double x){const double n=std::floor(x+.5),fraction=x+.5-n;return n*duty+std::clamp(fraction-(1-duty)*.5,0.,duty);};
 // Reduce before integration, retaining precision for large document offsets.
 phase=std::remainder(phase,1.);return unit((primitive(phase+width*.5)-primitive(phase-width*.5))/width);
}
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
double grain(double x,double y,std::uint32_t seed){const auto ix=std::int64_t(std::floor(x)),iy=std::int64_t(std::floor(y));const double a=x-std::floor(x),b=y-std::floor(y);auto n=[&](std::int64_t u,std::int64_t v){return hash(std::uint32_t(u)*0x8da6b343u^std::uint32_t(v)*0xd8163841u^seed)/4294967296.;};return std::lerp(std::lerp(n(ix,iy),n(ix+1,iy),a),std::lerp(n(ix,iy+1),n(ix+1,iy+1),a),b);}
}
bool valid(const Parameters& p){
 auto range=[](double x,double a,double b){return std::isfinite(x)&&x>=a&&x<=b;};
 for(auto value:{p.angle,p.angle1,p.angle2})if(!range(value,-1e9,1e9))return false;
 for(auto value:{p.shift_x,p.shift_y})if(!range(value,-1e7,1e7))return false;
 for(auto value:{p.frequency1,p.frequency2})if(!range(value,0,8))return false;
 for(const auto& color:{p.dark,p.light})for(auto value:color)if(!range(value,0,1))return false;
 return p.mode>=0&&p.mode<=2&&p.background>=0&&p.background<=1&&range(p.frequency,0,1000)&&range(p.sharpness,0,30)&&range(p.add_width,-100,100)&&range(p.smooth_source,0,200)&&range(p.wave_amp,0,.5)&&range(p.wave_frequency,0,30)&&range(p.warp_amp,-.5,.5)&&range(p.warp_smooth,0,.5)&&range(p.edge_strength,0,3)&&range(p.edge_threshold,0,2)&&range(p.edge_width,0,200)&&range(p.edge_sharpness,0,30)&&range(p.brightness,-100,100)&&range(p.contrast,0,400)&&range(p.gamma,.1,5)&&range(p.cross_amount,0,100)&&range(p.cross_start,0,.99)&&range(p.source_color,0,100)&&range(p.paper_grain,0,100)&&range(p.paper_scale,.5,100)&&range(p.mix,0,100);
}
Parameters preset(int i){Parameters p;
 if(i==1){p.frequency=85;p.mode=2;p.smooth_source=.8;p.wave_amp=.045;p.warp_amp=.035;p.cross_start=.2;p.dark={.025,.16,.10};p.light={.94,.94,.78};p.paper_grain=12;p.contrast=112;}
 if(i==2){p.frequency=65;p.mode=2;p.dark={.22,.115,.055};p.light={.96,.87,.67};p.paper_grain=24;p.wave_amp=.07;p.smooth_source=1.2;}
 if(i==3){p.mode=1;p.frequency=75;p.angle1=15;p.wave_amp=.035;p.warp_amp=.025;p.edge_strength=.3;p.contrast=115;}
 if(i==4){p.mode=2;p.frequency=80;p.wave_amp=0;p.warp_amp=0;p.angle1=45;p.angle2=-45;p.edge_strength=.35;}
 if(i==5){p.frequency=70;p.dark={.025,.09,.28};p.light={.94,.97,1};p.source_color=12;p.wave_amp=.065;p.paper_grain=8;}
 return p;
}
bool render(const Image& src,const Parameters& p,Image& out,const Cancel& cancel,RenderView view){
 pscpu::ParallelScope pool;if(!src.valid()||!valid(p)||(cancel&&cancel()))return false;if(p.mix==0){out=src;return true;}
 if(view.width==0&&view.height==0)view={double(src.left),double(src.top),double(src.width),double(src.height),1};
 for(auto value:{view.left,view.top,view.width,view.height,view.step})if(!std::isfinite(value))return false;if(view.width<=0||view.height<=0||view.step<1||view.step>2e6)return false;
 const int w=src.width,h=src.height;const auto count=std::size_t(w)*h;const double dimension=std::min(view.width,view.height);
 std::vector<float> tone(count),alpha(count),warped,edges;
 if(!pscpu::parallel_ranges(h,count,cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){for(auto y=lo;y<hi;y++){if(stop())return false;for(int x=0;x<w;x++){const auto n=y*w+x,i=n*4;alpha[n]=float(unit(clean(src.rgba[i+3])));tone[n]=float(unit(.2126*clean(src.rgba[i])+.7152*clean(src.rgba[i+1])+.0722*clean(src.rgba[i+2])));}}return true;}))return false;
 if(!blur(tone,alpha,w,h,p.smooth_source/view.step,cancel))return false;
 if(p.warp_amp!=0){warped=tone;if(!blur(warped,alpha,w,h,p.warp_smooth*dimension/view.step,cancel))return false;}
 auto sample=[&](const std::vector<float>& field,int x,int y,int center){const auto n=std::size_t(std::clamp(y,0,h-1))*w+std::clamp(x,0,w-1);return alpha[n]>1e-8f?double(field[n]):double(field[center]);};
 if(p.edge_strength>0){edges.resize(count);if(!pscpu::parallel_ranges(h,count,cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){for(auto y=lo;y<hi;y++){if(stop())return false;for(int x=0;x<w;x++){const auto n=y*w+x;const double dx=(sample(tone,x+1,int(y),int(n))-sample(tone,x-1,int(y),int(n)))*2/view.step,dy=(sample(tone,x,int(y)+1,int(n))-sample(tone,x,int(y)-1,int(n)))*2/view.step;edges[n]=float(unit(p.edge_strength*2*(1-std::exp(-std::max(0.,std::hypot(dx,dy)-p.edge_threshold)*(1+p.edge_sharpness)))));}}return true;}))return false;
  if(!dilate(edges,w,h,int(std::ceil(p.edge_width/view.step)),cancel))return false;
 }
 struct Lines {double cs,sn,frequency;};std::array<Lines,2> lines;
 for(int i=0;i<2;i++){const auto a=std::remainder(p.angle+(i?p.angle2:p.angle1),360.)*std::numbers::pi/180;lines[i]={std::cos(a),std::sin(a),p.frequency*(i?p.frequency2:p.frequency1)};}
 const bool first=lines[0].frequency>0,second=p.mode!=1&&lines[1].frequency>0;const bool coincident=std::abs(std::remainder(p.angle1-p.angle2,360.))<1e-10&&p.frequency1==p.frequency2;
 Image result{w,h,src.left,src.top,std::vector<float>(src.rgba.size())};const double mix=p.mix*.01;
 if(!pscpu::parallel_ranges(h,count,cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){for(auto y=lo;y<hi;y++){if(stop())return false;for(int x=0;x<w;x++){
  const auto n=y*w+x,i=n*4;const double t=std::pow(unit((tone[n]-.5)*p.contrast*.01+.5+p.brightness*.01),1/p.gamma),ink=unit(1-t+p.add_width*.01);
  double duty2=second?ink:0;if(first&&second&&!coincident){duty2=p.mode==2?ink*.5*p.cross_amount*.01*smooth(p.cross_start,1,ink):1-std::pow(std::max(0.,1-ink),.5*p.cross_amount*.01);}else if(first)duty2=0;
  const double duty1=first?(duty2>=1?1:unit((ink-duty2)/(1-duty2))):0;
  const double px=(view.left+(x+.5)*view.step-p.shift_x)/dimension,py=(view.top+(y+.5)*view.step-p.shift_y)/dimension;
  double warp=0,wx=0,wy=0;if(!warped.empty()){warp=p.warp_amp*(warped[n]-.5);wx=p.warp_amp*(sample(warped,x+1,int(y),int(n))-sample(warped,x-1,int(y),int(n)))*dimension/(2*view.step);wy=p.warp_amp*(sample(warped,x,int(y)+1,int(n))-sample(warped,x,int(y)-1,int(n)))*dimension/(2*view.step);}
  double coverage[2]{};for(int layer=0;layer<2;layer++){const auto& line=lines[layer];const double duty=layer?duty2:duty1;if(line.frequency<=0||duty<=0)continue;const double u=px*line.cs+py*line.sn,v=-px*line.sn+py*line.cs,phase=tau*p.wave_frequency*v,slope=p.wave_amp*tau*p.wave_frequency*std::cos(phase);const double dx=line.cs-slope*line.sn+wx,dy=line.sn+slope*line.cs+wy,footprint=line.frequency/dimension*view.step*(std::abs(dx)+std::abs(dy));coverage[layer]=stripe((u+p.wave_amp*std::sin(phase)+warp)*line.frequency,duty,footprint,p.sharpness);}
  double covered=1-(1-coverage[0])*(1-coverage[1]);if(!edges.empty())covered=1-(1-covered)*(1-edges[n]);
  const double a=alpha[n],effect_a=p.background?a*covered:a,output_a=std::lerp(a,effect_a,mix),paper_noise=p.paper_grain>0?p.paper_grain*.0025*(grain((view.left+(x+.5)*view.step)/p.paper_scale,(view.top+(y+.5)*view.step)/p.paper_scale,p.seed)-.5):0;
  for(int k=0;k<3;k++){const double original=clean(src.rgba[i+k]),dark=std::lerp(p.dark[k],unit(original),p.source_color*.01),paper=unit(p.light[k]+paper_noise),color=p.background?dark:std::lerp(paper,dark,covered);result.rgba[i+k]=output_a>0?float((original*a*(1-mix)+color*effect_a*mix)/output_a):0;}
  result.rgba[i+3]=float(output_a);
 }}return true;}))return false;
 if(cancel&&cancel())return false;out=std::move(result);return true;
}
}
