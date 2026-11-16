#include "energy_core.hpp"
#include "energy_internal.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <span>
namespace energy {
namespace {
constexpr std::size_t max_knots=16384,max_segments=262144,max_bytes=2*1024*1024;
bool finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::abs(p.x)<=1e9&&std::abs(p.y)<=1e9;}
Point add(Point a,Point b){return {a.x+b.x,a.y+b.y};}
Point sub(Point a,Point b){return {a.x-b.x,a.y-b.y};}
Point mul(Point a,double t){return {a.x*t,a.y*t};}
Point lerp(Point a,Point b,double t){return add(a,mul(sub(b,a),t));}
double length(Point a){return std::hypot(a.x,a.y);}
double distance(Point p,Point a,Point b,double* t=nullptr){const auto d=sub(b,a);const double n=d.x*d.x+d.y*d.y;const double u=n?std::clamp(((p.x-a.x)*d.x+(p.y-a.y)*d.y)/n,0.,1.):0;if(t)*t=u;return length(sub(p,lerp(a,b,u)));}
std::uint16_t be16(const std::uint8_t* p){return std::uint16_t((unsigned(p[0])<<8)|p[1]);}
std::uint32_t be32(const std::uint8_t* p){return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];}
void put32(std::vector<std::uint8_t>& b,std::uint32_t v){for(int k=3;k>=0;k--)b.push_back(std::uint8_t(v>>(k*8)));}
void put64(std::vector<std::uint8_t>& b,double d){auto v=std::bit_cast<std::uint64_t>(d);for(int k=7;k>=0;k--)b.push_back(std::uint8_t(v>>(k*8)));}
double read64(const std::uint8_t* p){std::uint64_t v=0;for(int k=0;k<8;k++)v=(v<<8)|p[k];return std::bit_cast<double>(v);}
bool cubic(Point a,Point b,Point c,Point d,double tolerance,int depth,std::vector<Segment>& out){
 if(out.size()>=max_segments)return false;
 // Distance to the finite chord also detects collinear overshoots and loops.
 if(depth>=20||std::max(distance(b,a,d),distance(c,a,d))<=tolerance){out.push_back({a,d});return true;}
 const auto ab=lerp(a,b,.5),bc=lerp(b,c,.5),cd=lerp(c,d,.5),abc=lerp(ab,bc,.5),bcd=lerp(bc,cd,.5),m=lerp(abc,bcd,.5);
 return cubic(a,ab,abc,m,tolerance,depth+1,out)&&cubic(m,bcd,cd,d,tolerance,depth+1,out);
}
bool blur(const std::vector<float>& src,int w,int h,int radius,std::vector<float>& out,const Cancel& cancel){
 radius=std::clamp(radius,0,65536);if(!radius){out=src;return true;}std::vector<float> temp(src.size());out.resize(src.size());const double norm=1./(2.*radius+1);
 if(!pscpu::parallel_ranges(h,src.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(auto y=begin;y<end;y++){if(stop())return false;double sum=0;for(int x=0;x<=std::min(w-1,radius);x++)sum+=src[y*w+x];for(int x=0;x<w;x++){temp[y*w+x]=float(sum*norm);if(x-radius>=0)sum-=src[y*w+x-radius];if(x+radius+1<w)sum+=src[y*w+x+radius+1];}}return true;}))return false;
 return pscpu::parallel_ranges(w,src.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(auto x=begin;x<end;x++){if(stop())return false;double sum=0;for(int y=0;y<=std::min(h-1,radius);y++)sum+=temp[y*w+x];for(int y=0;y<h;y++){out[std::size_t(y)*w+x]=float(sum*norm);if(y-radius>=0)sum-=temp[std::size_t(y-radius)*w+x];if(y+radius+1<h)sum+=temp[std::size_t(y+radius+1)*w+x];}}return true;});
}
}
bool Image::valid()const{return width>0&&height>0&&rgba.size()==std::size_t(width)*height*4;}
Parameters defaults(int w,int h,int l,int t){Parameters p;p.start_x=l+w*.2;p.end_x=l+w*.8;p.start_y=p.end_y=t+h*.5;return p;}
void reset_glow(Parameters& p){Parameters d;p.glow_model=0;p.halo_intensity=d.halo_intensity;p.halo_radius=d.halo_radius;p.glow_gamma=d.glow_gamma;p.glow_bias=d.glow_bias;p.glow_sizes=d.glow_sizes;p.glow_gains=d.glow_gains;}
Parameters calibrated_defaults(int w,int h,int l,int t){auto p=defaults(w,h,l,t);p.glow_model=1;p.width=6;p.glow_radius=184.2;p.color={.1,.49,1};return p;}
void reset_electric(Parameters& p){Parameters d;p.glow_detail=d.glow_detail;p.filament_count=d.filament_count;p.glow_seed=d.glow_seed;p.glow_distortion=d.glow_distortion;p.glow_noise_scale=d.glow_noise_scale;p.glow_width=d.glow_width;p.glow_texture=d.glow_texture;p.filament_strength=d.filament_strength;p.glow_evolution=d.glow_evolution;}
void reset_burning(Parameters& p){Parameters d;p.burn_detail=d.burn_detail;p.burn_seed=d.burn_seed;p.burn_width=d.burn_width;p.burn_turbulence=d.burn_turbulence;p.burn_scale=d.burn_scale;p.burn_density=d.burn_density;p.burn_filaments=d.burn_filaments;p.burn_wind=d.burn_wind;p.burn_evolution=d.burn_evolution;}
void reset_unified(Parameters& p){p.engine=0;p.core_noise=p.glow_noise=1;p.color_mix=0;}
Parameters unified_defaults(int w,int h,int l,int t){auto p=calibrated_defaults(w,h,l,t);p.engine=1;p.distortion=0;p.branches=0;p.glow_distortion=0;p.glow_texture=0;p.filament_strength=0;p.burn_wind=0;return p;}
bool valid(const Parameters& p){
 const double a[]={p.start_x,p.start_y,p.end_x,p.end_y,p.offset_x,p.offset_y};for(auto v:a)if(!std::isfinite(v)||std::abs(v)>1e7)return false;
 auto range=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 if(p.input<0||p.input>3||p.composite<0||p.composite>2||!range(p.width,.1,4096)||!range(p.brightness,0,100)||!range(p.core_white,0,100)||!range(p.glow_radius,0,4096)||!range(p.glow_intensity,0,100))return false;
 for(auto v:p.color)if(!range(v,0,1))return false;
 if(p.style<0||p.style>4||p.detail<1||p.detail>6||p.branches<0||p.branches>16||p.seed>0x7fffffffu||!range(p.alpha_threshold,.1,99.9)||!range(p.distortion,0,4096)||!range(p.wavelength,.05,8192)||!range(p.evolution,-360000,360000)||!range(p.branch_length,0,4096)||!range(p.fire_height,0,4096)||!range(p.fire_spread,0,4096)||!range(p.fire_direction,-360000,360000))return false;
 for(auto v:p.tip_color)if(!range(v,0,1))return false;
 if(p.glow_model<0||p.glow_model>1||!range(p.halo_intensity,0,20)||!range(p.halo_radius,0,4096)||!range(p.glow_gamma,.1,10)||!range(p.glow_bias,0,2))return false;
 for(auto v:p.glow_sizes)if(!range(v,0,4))return false;
 for(auto v:p.glow_gains)if(!range(v,0,20))return false;
 if((p.style>=3&&p.glow_model!=1)||p.glow_detail<1||p.glow_detail>6||p.filament_count<0||p.filament_count>6||p.glow_seed>0x7fffffffu||!range(p.glow_distortion,0,4096)||!range(p.glow_noise_scale,.0001,8192)||!range(p.glow_width,0,4096)||!range(p.glow_texture,0,10)||!range(p.filament_strength,0,10)||!range(p.glow_evolution,-360000,360000))return false;
 if(p.burn_detail<1||p.burn_detail>6||p.burn_seed>0x7fffffffu||!range(p.burn_width,0,4096)||!range(p.burn_turbulence,0,4096)||!range(p.burn_scale,.0001,8192)||!range(p.burn_density,0,10)||!range(p.burn_filaments,0,10)||!range(p.burn_wind,0,4096)||!range(p.burn_evolution,-360000,360000))return false;
 if(p.engine<0||p.engine>1||p.core_noise<0||p.core_noise>2||p.glow_noise<0||p.glow_noise>2||!range(p.color_mix,0,1)||(p.engine==1&&(p.style!=0||p.glow_model!=1)))return false;
 return range(p.trim_start,0,100)&&range(p.trim_end,0,100)&&range(p.taper_start,0,100)&&range(p.taper_end,0,100);
}
bool valid_path(const Path& p){std::size_t count=0;if(p.curves.size()>4096)return false;for(const auto& c:p.curves){if(c.knots.empty())return false;count+=c.knots.size();if(count>max_knots)return false;for(const auto& k:c.knots)if(!finite(k.before)||!finite(k.anchor)||!finite(k.after))return false;}return true;}
bool parse_path(const std::vector<std::uint8_t>& bytes,double w,double h,Path& out,std::string& error){
 auto fail=[&](const char* s){error=s;return false;};
 if(bytes.empty()||bytes.size()%26||bytes.size()>max_bytes||!std::isfinite(w)||!std::isfinite(h)||w<=0||h<=0||w>2e6||h>2e6)return fail("Invalid path-resource size or canvas");
 Path next;std::size_t knots=0;
 for(std::size_t i=0;i<bytes.size();){const auto* r=bytes.data()+i;const auto type=be16(r);i+=26;
  if(type==6||type==7||type==8)continue;
  if(type!=0&&type!=3)return fail("Unexpected path record");
  const auto count=be16(r+2);if(!count||knots+count>max_knots||std::size_t(count)>(bytes.size()-i)/26)return fail("Invalid subpath length");
  Curve c;c.closed=type==0;
  for(unsigned n=0;n<count;n++,i+=26){r=bytes.data()+i;const auto kind=be16(r);if(c.closed?(kind!=1&&kind!=2):(kind!=4&&kind!=5))return fail("Mismatched Bezier record");
   auto point=[&](int pos){return Point{std::bit_cast<std::int32_t>(be32(r+pos+4))/16777216.*w,std::bit_cast<std::int32_t>(be32(r+pos))/16777216.*h};};c.knots.push_back({point(2),point(10),point(18)});
  }knots+=count;next.curves.push_back(std::move(c));
 }
 if(next.curves.empty()||!valid_path(next))return fail("The path has no usable geometry");out=std::move(next);error.clear();return true;
}
std::vector<std::uint8_t> encode_path(const Path& p){
 if(!valid_path(p))return {};std::vector<std::uint8_t> b{'E','B','P','1'};put32(b,std::uint32_t(p.curves.size()));for(const auto& c:p.curves){put32(b,c.closed?1:0);put32(b,std::uint32_t(c.knots.size()));for(const auto& k:c.knots)for(auto a:{k.before,k.anchor,k.after}){put64(b,a.x);put64(b,a.y);}}return b;
}
bool decode_path(const std::vector<std::uint8_t>& b,Path& out){
 if(b.size()<8||b.size()>max_bytes||!std::equal(b.begin(),b.begin()+4,"EBP1"))return false;std::size_t pos=8,total=0;const auto count=be32(b.data()+4);if(count>4096)return false;Path next;
 for(std::uint32_t i=0;i<count;i++){if(b.size()-pos<8)return false;const auto closed=be32(b.data()+pos),n=be32(b.data()+pos+4);pos+=8;if(closed>1||!n||n>max_knots-total||std::size_t(n)>(b.size()-pos)/48)return false;Curve c;c.closed=closed!=0;
  for(std::uint32_t j=0;j<n;j++){Knot k;for(auto* point:{&k.before,&k.anchor,&k.after}){point->x=read64(b.data()+pos);point->y=read64(b.data()+pos+8);pos+=16;}c.knots.push_back(k);}total+=n;next.curves.push_back(std::move(c));
 }if(pos!=b.size()||!valid_path(next))return false;out=std::move(next);return true;
}
Path line_path(const Parameters& p){Point a{p.start_x,p.start_y},b{p.end_x,p.end_y};return {{{false,{{a,a,a},{b,b,b}}}}};}
bool flatten(const Path& p,double tolerance,std::vector<Segment>& out,const Cancel& cancel){
 if(!valid_path(p)||!std::isfinite(tolerance)||tolerance<=0)return false;std::vector<Segment> next;
 for(const auto& c:p.curves){if(cancel&&cancel())return false;const auto n=c.knots.size();const auto start=next.size();if(n==1)next.push_back({c.knots[0].anchor,c.knots[0].anchor});
  for(std::size_t i=0;i<(c.closed?n:n-1)&&n>1;i++){if(cancel&&cancel())return false;const auto& a=c.knots[i];const auto& b=c.knots[(i+1)%n];if(!cubic(a.anchor,a.after,b.before,b.anchor,tolerance,0,next))return false;}
  double sum=0;for(auto i=start;i<next.size();i++){next[i].u0=sum;sum+=length(sub(next[i].b,next[i].a));next[i].u1=sum;}for(auto i=start;i<next.size();i++){next[i].u0=sum?next[i].u0/sum:0;next[i].u1=sum?next[i].u1/sum:1;}
 }out=std::move(next);return true;
}
bool detail::rasterize(const Image& src,const Parameters& p,const std::vector<Segment>& segments,std::vector<float>& core,const Cancel& cancel){
 const int w=src.width,h=src.height;
 if(p.trim_start<p.trim_end&&p.brightness>0)for(const auto& s:segments){
  if(cancel&&cancel())return false;const double lo=std::max(s.u0,p.trim_start*.01),hi=std::min(s.u1,p.trim_end*.01);if(hi<lo)continue;
  const double span=s.u1-s.u0;Point a=lerp(s.a,s.b,span?(lo-s.u0)/span:0),b=lerp(s.a,s.b,span?(hi-s.u0)/span:1);a.x+=p.offset_x-src.left;a.y+=p.offset_y-src.top;b.x+=p.offset_x-src.left;b.y+=p.offset_y-src.top;
  const double radius=p.width*.5+.75;const int x0=int(std::clamp(std::floor(std::min(a.x,b.x)-radius),0.,double(w))),x1=int(std::clamp(std::ceil(std::max(a.x,b.x)+radius),0.,double(w))),y0=int(std::clamp(std::floor(std::min(a.y,b.y)-radius),0.,double(h))),y1=int(std::clamp(std::ceil(std::max(a.y,b.y)+radius),0.,double(h)));
  for(int y=y0;y<y1;y++){if(cancel&&cancel())return false;for(int x=x0;x<x1;x++){double t=0;const double d=distance({x+.5,y+.5},a,b,&t),u=lo+(hi-lo)*t;double taper=1;if(p.taper_start>0)taper=std::min(taper,(u-p.trim_start*.01)/(p.taper_start*.01));if(p.taper_end>0)taper=std::min(taper,(p.trim_end*.01-u)/(p.taper_end*.01));const double r=p.width*.5*std::clamp(taper,0.,1.);const float coverage=float(std::clamp(r+.5-d,0.,1.));core[std::size_t(y)*w+x]=std::max(core[std::size_t(y)*w+x],coverage);}}
 }
 return true;
}
bool detail::composite(const Image& src,const Parameters& p,const std::vector<float>& core,Image& out,const Cancel& cancel,const std::vector<float>* flame,const std::vector<float>* heat){
 if(p.glow_model==1)return layered_composite(src,p,core,out,cancel,flame,heat);
 const int w=src.width,h=src.height;const std::size_t size=std::size_t(w)*h;std::vector<float> halo(size);
 if(p.glow_radius>0&&p.glow_intensity>0){const double scales[]={.12,.45,1.2},weights[]={.45,.35,.2};for(int i=0;i<3;i++){std::vector<float> a=core,b;if(flame)for(std::size_t n=0;n<size;n++)a[n]+=(*flame)[n]*.6f;const int r=std::max(1,int(std::lround(p.glow_radius*scales[i]*.58)));for(int j=0;j<3;j++){if(!blur(a,w,h,r,b,cancel))return false;a.swap(b);}const double gain=weights[i]*(1.+.9*p.glow_radius*scales[i]/std::max(.5,p.width));for(std::size_t n=0;n<size;n++)halo[n]+=float(a[n]*gain);}}
 Image next{w,h,src.left,src.top,std::vector<float>(size*4)};
 if(!pscpu::parallel_ranges(h,size,cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){if(stop())return false;for(int x=0;x<w;x++){const auto n=y*w+x,i=n*4;const double sa=p.composite==0?std::clamp(double(src.rgba[i+3]),0.,1.):0;double emission[3],maximum=0;
    for(int k=0;k<3;k++){const double color=std::pow(p.color[k],2.2),white=p.core_white*.01;emission[k]=p.brightness*(core[n]*(color*(1-white)+white)*2+halo[n]*p.glow_intensity*color);if(flame){const double heat_value=std::clamp(double((*heat)[n]),0.,1.),tip=std::pow(p.tip_color[k],2.2);emission[k]+=p.brightness*((*flame)[n]*(tip+(color-tip)*heat_value)*3+halo[n]*p.glow_intensity*(tip-color)*.35);emission[k]=std::max(0.,emission[k]);}maximum=std::max(maximum,emission[k]);}
    if(maximum==0&&p.composite==0){std::copy_n(src.rgba.data()+i,4,next.rgba.data()+i);continue;}
    const double ea=std::clamp(std::pow(maximum,1/2.2),0.,1.),alpha=p.composite==2?1:sa+(1-sa)*ea;next.rgba[i+3]=float(alpha);
    for(int k=0;k<3;k++){double before=p.composite==0?double(src.rgba[i+k]):0;if(!std::isfinite(before))before=0;const double linear=std::pow(std::max(0.,before)*sa,2.2)+emission[k];const double encoded=std::pow(linear,1/2.2);next.rgba[i+k]=alpha?float(encoded/alpha):0;}
   }}return true;}))return false;
 out=std::move(next);return true;
}
bool render(const Image& src,const Parameters& p,const Path& path,Image& out,const Cancel& cancel){
 pscpu::ParallelScope parallel;
 if(!src.valid()||!valid(p)||!valid_path(path)||(p.input==1&&path.curves.empty()))return false;if(cancel&&cancel())return false;
 if(p.engine==1)return detail::unified_render(src,p,path,out,cancel);
 if(p.input>=2||p.style!=0)return detail::extended(src,p,path,out,cancel);
 std::vector<float> core(std::size_t(src.width)*src.height);std::vector<Segment> segments;
 if(!flatten(p.input?path:line_path(p),.2,segments,cancel)||!detail::rasterize(src,p,segments,core,cancel))return false;
 return detail::composite(src,p,core,out,cancel);
}
void reset_extension(Parameters& p){Parameters d;p.style=d.style;p.detail=d.detail;p.branches=d.branches;p.seed=d.seed;p.alpha_threshold=d.alpha_threshold;p.distortion=d.distortion;p.wavelength=d.wavelength;p.evolution=d.evolution;p.branch_length=d.branch_length;p.fire_height=d.fire_height;p.fire_spread=d.fire_spread;p.fire_direction=d.fire_direction;p.tip_color=d.tip_color;}
Parameters material_preset(int index,const Parameters& geometry){
 auto p=geometry;reset_extension(p);p.alpha_threshold=geometry.alpha_threshold;p.width=3;p.brightness=1;p.core_white=85;p.glow_radius=32;p.glow_intensity=1;p.color={.08,.5,1};
 if(index==1){p.style=1;p.width=2;p.detail=4;p.distortion=24;p.wavelength=90;p.branches=3;p.branch_length=48;p.glow_radius=28;p.color={.06,.42,1};}
 if(index==2){p.style=2;p.width=3;p.brightness=.8;p.glow_radius=24;p.glow_intensity=.8;p.color={1,.55,.04};p.fire_height=96;p.fire_spread=24;}
 if(index==0&&p.glow_model==1){p.width=6;p.glow_radius=184.2;p.color={.1,.49,1};}
 if(index==3){reset_electric(p);p.style=3;p.glow_model=1;p.width=2.5;p.core_white=95;p.color={.03,.65,1};p.distortion=6;p.wavelength=28;p.detail=5;p.branches=0;p.glow_radius=40;p.halo_radius=2.5;p.halo_intensity=1.2;p.glow_gamma=1;p.glow_bias=.5;p.glow_sizes={1,.5,.2,.06};p.glow_gains={.03,.14,.1,.01};}
 if(index==4){reset_burning(p);p.style=4;p.glow_model=1;p.width=4.8;p.brightness=.85;p.core_white=90;p.color={1,.4,.025};p.tip_color={.8,.02,.001};p.distortion=5;p.wavelength=48;p.detail=5;p.seed=5000;p.branches=0;p.glow_radius=70;p.glow_intensity=.8;p.halo_radius=2.2;p.halo_intensity=1.2;p.glow_gamma=1;p.glow_bias=.5;p.glow_sizes={1,.5,.2,.05};p.glow_gains={.15,.18,.14,.01};}
 return p;
}
Parameters draft_parameters(Parameters p,double l,double t,double s){p.start_x=(p.start_x-l)/s;p.end_x=(p.end_x-l)/s;p.start_y=(p.start_y-t)/s;p.end_y=(p.end_y-t)/s;p.offset_x/=s;p.offset_y/=s;p.width=std::max(.1,p.width/s);p.glow_radius/=s;p.halo_radius/=s;p.distortion/=s;p.wavelength=std::max(.05,p.wavelength/s);p.branch_length/=s;p.fire_height/=s;p.fire_spread/=s;p.glow_distortion/=s;p.glow_noise_scale=std::max(.0001,p.glow_noise_scale/s);p.glow_width/=s;p.burn_width/=s;p.burn_turbulence/=s;p.burn_scale=std::max(.0001,p.burn_scale/s);p.burn_wind/=s;return p;}
Path draft_path(Path p,double l,double t,double s){for(auto& c:p.curves)for(auto& k:c.knots)for(auto* a:{&k.before,&k.anchor,&k.after}){a->x=(a->x-l)/s;a->y=(a->y-t)/s;}return p;}
Parameters unified_preset(int index,const Parameters& g){
 auto p=unified_defaults(1,1);p.input=g.input;p.composite=g.composite;p.start_x=g.start_x;p.start_y=g.start_y;p.end_x=g.end_x;p.end_y=g.end_y;p.trim_start=g.trim_start;p.trim_end=g.trim_end;p.taper_start=g.taper_start;p.taper_end=g.taper_end;p.offset_x=g.offset_x;p.offset_y=g.offset_y;p.alpha_threshold=g.alpha_threshold;
 if(index==1||index==3){p.width=2.5;p.core_white=95;p.color={.03,.65,1};p.tip_color={.03,.3,1};p.distortion=6;p.wavelength=28;p.detail=5;p.glow_radius=40;p.halo_radius=2.5;p.halo_intensity=1.2;p.glow_sizes={1,.5,.2,.06};p.glow_gains={.03,.14,.1,.01};p.glow_distortion=24;p.glow_texture=1;p.filament_strength=.8;}
 if(index==2){p.width=4.8;p.brightness=.85;p.core_white=90;p.color={1,.4,.025};p.tip_color={.8,.02,.001};p.distortion=5;p.wavelength=48;p.detail=5;p.seed=5000;p.glow_radius=70;p.glow_intensity=.8;p.halo_radius=2.2;p.halo_intensity=1.2;p.glow_sizes={1,.5,.2,.05};p.glow_gains={.15,.18,.14,.01};p.glow_width=10;p.glow_distortion=24;p.glow_noise_scale=56;p.glow_detail=5;p.glow_texture=1;p.filament_strength=.7;p.color_mix=1;}
 if(index==3){p.color={.2,.5,1};p.tip_color={1,.08,.015};p.color_mix=.65;p.core_noise=2;p.glow_noise=2;p.glow_width=10;p.glow_noise_scale=48;p.glow_texture=.6;p.filament_strength=1.1;p.burn_wind=12;p.fire_direction=-45;p.glow_seed=7531;}
 if(index>=4&&index<unified_preset_count){
  p.width=2;p.brightness=.85;p.core_white=75;p.glow_radius=48;p.glow_intensity=.8;p.halo_radius=1.8;p.halo_intensity=1.3;p.glow_sizes={1,.45,.16,.04};p.glow_gains={.08,.18,.12,.015};p.wavelength=40;p.detail=4;
  switch(index){
   case 4: // A fine clean line with a narrow mint halo.
    p.width=1.25;p.brightness=.7;p.core_white=20;p.color={.05,1,.62};p.glow_radius=24;p.halo_radius=1.2;p.glow_gains={.03,.12,.12,.01};break;
   case 5: // Broad clean neon: shape is intentionally free of noise.
    p.width=3.5;p.brightness=.8;p.core_white=45;p.color={1,.045,.4};p.glow_radius=95;p.halo_radius=3;p.glow_gains={.3,.35,.14,.01};break;
   case 6: // Short branching arcs, with sharp core noise and fine outer threads.
    p.width=1.7;p.core_white=90;p.color={1,.72,.07};p.core_noise=2;p.distortion=12;p.wavelength=35;p.detail=5;p.branches=5;p.branch_length=30;p.seed=7319;p.glow_radius=30;p.glow_noise=2;p.glow_distortion=6;p.glow_width=3;p.glow_texture=.15;p.filament_strength=1.1;p.filament_count=2;break;
   case 7:
    p.width=1.1;p.core_white=92;p.color={.35,.85,1};p.tip_color={.08,.26,.8};p.color_mix=.4;p.core_noise=2;p.distortion=5;p.wavelength=18;p.detail=6;p.branches=8;p.branch_length=18;p.seed=1289;p.glow_noise=2;p.glow_distortion=15;p.glow_noise_scale=22;p.glow_width=5;p.glow_texture=.2;p.filament_strength=1.3;p.filament_count=5;break;
   case 8: // Soft cloud with a subdued core, not merely a hue change.
    p.width=.8;p.brightness=.65;p.core_white=0;p.color={.48,.14,1};p.tip_color={.12,.03,.5};p.color_mix=.75;p.glow_radius=90;p.halo_intensity=.25;p.glow_noise=0;p.glow_distortion=36;p.glow_noise_scale=94;p.glow_detail=3;p.glow_width=24;p.glow_texture=.9;p.glow_seed=8211;p.filament_strength=0;p.glow_gains={.14,.12,.05,.005};break;
   case 9:
    p.width=2;p.core_white=20;p.color={.42,1,.025};p.tip_color={.015,.22,.03};p.color_mix=1;p.distortion=8;p.wavelength=27;p.glow_noise=2;p.glow_distortion=32;p.glow_noise_scale=44;p.glow_width=12;p.glow_texture=1.1;p.filament_strength=.35;p.glow_seed=1919;p.burn_wind=8;p.fire_direction=-90;break;
   case 10:
    p.width=5.5;p.brightness=.75;p.core_white=10;p.color={1,.22,.015};p.tip_color={.5,.005,.001};p.color_mix=1;p.distortion=5;p.wavelength=34;p.core_noise=0;p.glow_noise=2;p.glow_distortion=16;p.glow_noise_scale=24;p.glow_detail=5;p.glow_width=6;p.glow_texture=.8;p.filament_strength=1.2;p.filament_count=5;p.glow_radius=60;p.glow_seed=9341;break;
   case 11:
    p.width=2.7;p.brightness=.9;p.core_white=70;p.color={1,.67,.12};p.tip_color={1,.045,.002};p.color_mix=1;p.distortion=3;p.wavelength=70;p.glow_noise=1;p.glow_distortion=42;p.glow_noise_scale=62;p.glow_detail=5;p.glow_width=22;p.glow_texture=1.2;p.filament_strength=.75;p.filament_count=4;p.glow_radius=100;p.glow_gains={.12,.18,.08,.01};p.glow_seed=6151;break;
   case 12:
    p.width=1.6;p.core_white=85;p.color={.55,.12,1};p.tip_color={.015,.65,1};p.color_mix=.85;p.core_noise=2;p.distortion=10;p.wavelength=24;p.detail=6;p.seed=1637;p.glow_noise=2;p.glow_distortion=24;p.glow_noise_scale=30;p.glow_width=8;p.glow_texture=.1;p.filament_strength=1.8;p.filament_count=6;p.glow_seed=9173;break;
   case 13:
    p.width=1.5;p.core_white=35;p.color={.1,.8,1};p.tip_color={.015,.2,.3};p.color_mix=.65;p.core_noise=0;p.distortion=3;p.wavelength=85;p.glow_noise=0;p.glow_distortion=12;p.glow_noise_scale=75;p.glow_detail=2;p.glow_width=9;p.glow_texture=.4;p.filament_strength=1;p.filament_count=3;p.burn_wind=42;p.fire_direction=-35;p.glow_seed=3419;break;
   case 14:
    p.width=.7;p.brightness=.45;p.core_white=10;p.color={.65,.95,.8};p.tip_color={.12,.4,.36};p.color_mix=.9;p.glow_radius=110;p.halo_intensity=.35;p.glow_noise=0;p.glow_distortion=18;p.glow_noise_scale=120;p.glow_detail=2;p.glow_width=17;p.glow_texture=.45;p.filament_strength=.08;p.glow_gains={.16,.12,.04,.002};p.burn_wind=15;p.fire_direction=-90;p.glow_seed=2213;break;
   case 15:
    p.width=1.7;p.brightness=.7;p.core_white=10;p.color={.15,1,.55};p.tip_color={.45,.06,1};p.color_mix=.95;p.core_noise=0;p.distortion=4;p.wavelength=110;p.glow_noise=0;p.glow_distortion=30;p.glow_noise_scale=90;p.glow_detail=3;p.glow_width=20;p.glow_texture=.6;p.filament_strength=.55;p.filament_count=4;p.burn_wind=30;p.fire_direction=-90;p.glow_radius=85;p.glow_seed=7529;break;
  }
 }
 return p;
}
Parameters to_unified(const Parameters& old){
 if(old.engine==1)return old;
 auto p=old;reset_unified(p);p.engine=1;p.style=0;p.glow_model=1;p.burn_wind=0;
 if(old.style<3){p.glow_distortion=0;p.glow_texture=0;p.filament_strength=0;if(old.style==0){p.distortion=0;p.branches=0;}if(old.style==2){p.distortion=0;p.branches=0;p.glow_width=std::max(3.,old.fire_spread*.35);p.glow_distortion=old.fire_spread;p.glow_noise_scale=old.wavelength;p.glow_detail=old.detail;p.glow_seed=old.seed;p.glow_texture=1;p.filament_strength=.7;p.burn_wind=old.fire_height*.25;p.color_mix=1;}}
 if(old.style==4){p.glow_width=old.burn_width;p.glow_distortion=old.burn_turbulence;p.glow_noise_scale=old.burn_scale;p.glow_detail=old.burn_detail;p.glow_seed=old.burn_seed;p.glow_evolution=old.burn_evolution;p.glow_texture=old.burn_density;p.filament_strength=old.burn_filaments;p.filament_count=3;p.burn_wind=old.burn_wind;p.color_mix=1;}
 return p;
}
}

#include "alpha_contours.inc"
#include "energy_materials.inc"
#include "energy_glow.inc"
#include "energy_electric.inc"
#include "energy_burning.inc"
#include "energy_unified.inc"
