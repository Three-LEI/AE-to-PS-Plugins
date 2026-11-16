#include "lightfx_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
namespace lightfx {namespace {
constexpr double pi=std::numbers::pi;
bool stop(const Cancel& c){return c&&c();}
double unit(double v){return std::clamp(v,0.,1.);}
float finite(float v){return std::isfinite(v)?v:0.f;}
float narrow(double v){const double bound=std::numeric_limits<float>::max();return float(std::clamp(v,-bound,bound));}
double smooth(double x){x=unit(x);return x*x*(3-2*x);}
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
double noise(double x,double y,std::uint32_t seed){
 x=std::clamp(x,-1e8,1e8);y=std::clamp(y,-1e8,1e8);const int ix=int(std::floor(x)),iy=int(std::floor(y));const double u=smooth(x-ix),v=smooth(y-iy);
 auto h=[&](int a,int b){return double(hash(std::uint32_t(a)*0x9e3779b9u^std::uint32_t(b)*0x85ebca6bu^seed))/4294967295.;};
 return std::lerp(std::lerp(h(ix,iy),h(ix+1,iy),u),std::lerp(h(ix,iy+1),h(ix+1,iy+1),u),v);
}
Color mapped(const Parameters& p,int map,double amount){
 const auto& colors=p.colors[std::clamp(map,0,2)];if(p.map_mode==0)return colors[4];
 const double x=unit(amount)*(p.map_mode==1?2:4);const int lo=std::min(int(x),p.map_mode==1?1:3),a=p.map_mode==1?lo*2:lo,b=p.map_mode==1?(lo+1)*2:lo+1;
 Color out;for(int k=0;k<3;k++)out[k]=std::lerp(colors[a][k],colors[b][k],x-lo);return out;
}
Color emission(const Image& e,const Parameters& p,int map,std::size_t i){
 if(p.source_color)return {e.rgba[i],e.rgba[i+1],e.rgba[i+2]};const double amount=e.rgba[i+3];auto color=mapped(p,map,amount);for(auto& c:color)c*=amount;return color;
}
Color sample(const Image& a,double x,double y){
 Color out{};if(x< -1||y< -1||x>a.width||y>a.height)return out;const int ix=int(std::floor(x)),iy=int(std::floor(y));const double u=x-ix,v=y-iy;
 for(int j=0;j<2;j++)for(int i=0;i<2;i++){const int xx=ix+i,yy=iy+j;if(xx<0||yy<0||xx>=a.width||yy>=a.height)continue;const auto at=(std::size_t(yy)*a.width+xx)*4;const double w=(i?u:1-u)*(j?v:1-v);for(int k=0;k<3;k++)out[k]+=a.rgba[at+k]*w;}return out;
}
bool star(const Image& src,const Image& seed,const Parameters& p,Image& rays,const Cancel& cancel){
 const int w=src.width,h=src.height;
 for(int direction=0;direction<8;direction++){
  if(stop(cancel))return false;const double length=p.length*p.lengths[direction]*.01;if(length<=0)continue;
  const double angle=(p.rotation+45*direction)*pi/180;const double vx=std::cos(angle),vy=std::sin(angle),major=std::max(std::abs(vx),std::abs(vy));
  // Length controls reach; intensity controls brightness independently.
  const double q=std::exp(-3*src.step/(major*std::max(.001,length/p.falloff))),gain=.5*p.boost;
  const int dx=vx<0?-1:1,dy=vy<0?-1:1;
  const bool horizontal=std::abs(vy)<1e-9,vertical=std::abs(vx)<1e-9,diagonal=std::abs(std::abs(vx)-std::abs(vy))<1e-9;
  if(horizontal||vertical||diagonal){
   const int lines=horizontal?h:vertical?w:w+h-1;
   if(!pscpu::parallel_ranges(lines,std::size_t(w)*h,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
    for(auto j=lo;j<hi;j++){
     if((j&15)==0&&cancelled())return false;int x=horizontal?(dx>0?0:w-1):vertical?int(j):int(j)<w?int(j):(dx>0?0:w-1);
     int y=horizontal?int(j):vertical?(dy>0?0:h-1):int(j)<w?(dy>0?0:h-1):(dy>0?int(j)-w+1:h-2-(int(j)-w));
     Color state{};while(x>=0&&y>=0&&x<w&&y<h){const auto i=(std::size_t(y)*w+x)*4;auto a=emission(seed,p,p.maps[direction],i);for(int k=0;k<3;k++){rays.rgba[i+k]=narrow(rays.rgba[i+k]+q*state[k]*gain);state[k]=a[k]+q*state[k];}x+=vertical?0:dx;y+=horizontal?0:dy;}
    }return true;
   }))return false;
  }else{
   // Shear into straight scan lines, then resample once on entry and exit.
   // Unlike recursively interpolating adjacent lines, the footprint does not
   // diffuse wider as the ray travels. Slabs bound temporary memory.
   const bool xmajor=std::abs(vx)>std::abs(vy);const int count=xmajor?h:w,steps=xmajor?w:h,sign=xmajor?dx:dy;
   const double slope=xmajor?vy/vx:vx/vy;
   const int first=int(std::floor(std::min(0.,-slope*(steps-1))))-1;
   const int end=int(std::ceil(count+std::max(0.,-slope*(steps-1))))+1;
   const int slab=std::clamp(1048576/(steps*3)-1,1,32);std::vector<float> lines(std::size_t(slab+1)*steps*3);
   auto interval=[&](double low,double high){
    const double a=low/slope,b=high/slope;return std::pair{std::max(0,int(std::ceil(std::min(a,b)))),std::min(steps-1,int(std::floor(std::max(a,b))))};
   };
   auto incoming=[&](int u,double v){
    const int at=int(std::floor(v));const double t=v-at;Color result{};
    for(int j=0;j<2;j++){const int minor=at+j;if(minor<0||minor>=count)continue;
     const int x=xmajor?u:minor,y=xmajor?minor:u;const auto c=emission(seed,p,p.maps[direction],(std::size_t(y)*w+x)*4);
     for(int k=0;k<3;k++)result[k]+=c[k]*(j?t:1-t);
    }return result;
   };
   for(int base=first;base<end;base+=slab){
    if(stop(cancel))return false;
    for(int line=0;line<=slab;line++){
     const auto [lo,hi]=interval(-1.-base-line,double(count)-base-line);
     Color state{};for(int j=lo;j<=hi;j++){
      const int u=sign>0?j:hi-(j-lo);const auto c=incoming(u,base+line+slope*u);const auto i=(std::size_t(line)*steps+u)*3;
      for(int k=0;k<3;k++){lines[i+k]=narrow(q*state[k]*gain);state[k]=c[k]+q*state[k];}
     }
    }
    const auto [lo,hi]=interval(-double(base)-slab,double(count)-base);
    for(int u=lo;u<=hi;u++){
     const double shift=slope*u;const int low=std::max(0,int(std::ceil(base+shift))),high=std::min(count,int(std::ceil(base+slab+shift)));
     for(int v=low;v<high;v++){
      const double at=v-shift-base;const int line=std::clamp(int(std::floor(at)),0,slab-1);const double t=unit(at-line);
      const int x=xmajor?u:v,y=xmajor?v:u;const auto to=(std::size_t(y)*w+x)*4,from=(std::size_t(line)*steps+u)*3;
      for(int k=0;k<3;k++)rays.rgba[to+k]=narrow(rays.rgba[to+k]+std::lerp(double(lines[from+k]),double(lines[from+std::size_t(steps)*3+k]),t));
     }
    }
   }
  }
 }
 if(p.boost>0)for(std::size_t i=0;i<rays.rgba.size();i+=4){
  const double energy=std::max({0.f,rays.rgba[i],rays.rgba[i+1],rays.rgba[i+2]});
  if(energy>0){const double scale=p.boost*(-std::expm1(-energy/p.boost))/energy;for(int k=0;k<3;k++)rays.rgba[i+k]=narrow(rays.rgba[i+k]*scale);}
 }
 return true;
}
bool shine(const Image& src,const Image& seed,const Parameters& p,Image& rays,const Cancel& cancel){
 if(p.length<=0)return true;Image a=seed,b=seed;
 for(std::size_t i=0;i<a.rgba.size();i+=4){auto color=emission(seed,p,0,i);for(int k=0;k<3;k++)a.rgba[i+k]=float(color[k]);}
 const double unit_length=p.length/p.samples,decay=std::exp(-p.falloff*unit_length),cx=(p.cx-src.left)/src.step-.5,cy=(p.cy-src.top)/src.step-.5;
 // Doubling in log-radius space approximates 32/64/128 ray samples in 5/6/7
 // full-image passes. Cost is independent of the selected ray length.
 for(int span=1;span<p.samples;span*=2){
  const double scale=std::exp(-unit_length*span),weight=std::pow(decay,span);
  if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
   for(auto y=lo;y<hi;y++){if((y&7)==0&&cancelled())return false;for(int x=0;x<src.width;x++){const auto i=(y*src.width+x)*4;const auto value=sample(a,cx+(x-cx)*scale,cy+(double(y)-cy)*scale);for(int k=0;k<3;k++)b.rgba[i+k]=narrow(a.rgba[i+k]+weight*value[k]);}}return true;
  }))return false;
  a.rgba.swap(b.rgba);
 }
 const double sum=std::abs(decay-1)<1e-10?p.samples:(1-std::pow(decay,p.samples))/(1-decay),gain=p.boost*4/sum;
 for(std::size_t i=0;i<a.rgba.size();i+=4)for(int k=0;k<3;k++)rays.rgba[i+k]=narrow(a.rgba[i+k]*gain);return !stop(cancel);
}
} // namespace
bool Image::valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&std::isfinite(step)&&step>=1&&rgba.size()==std::size_t(width)*height*4;}
Parameters defaults(Kind kind,int width,int height,int left,int top){
 Parameters p;p.kind=kind;p.cx=left+width*.5;p.cy=top+height*.5;p.mask_cx=p.cx;p.mask_cy=p.cy;p.mask_radius=std::min(width,height)*.4;p.mask_feather=std::min(width,height)*.1;p.length=kind==Kind::shine?1:std::clamp(std::min(width,height)*.075,40.,240.);
 for(auto& map:p.colors)for(auto& color:map)color={1,1,1};return p;
}
Parameters preset(const Parameters& value,int index){
 auto p=value;p.source_color=false;p.map_mode=0;p.lengths.fill(100);p.maps.fill(0);p.rotation=0;p.shimmer=0;
 for(auto& map:p.colors)for(auto& color:map)color={1,1,1};
 if(p.kind==Kind::starglow){if(index==1)for(int i=0;i<8;i++)p.lengths[i]=i%2?0:100;else if(index==2)for(int i=0;i<8;i++)p.lengths[i]=i%4?0:200;else if(index==3){p.map_mode=2;const Map colors{{{1,.05,.15},{1,.6,.05},{.1,1,.25},{.05,.5,1},{.65,.1,1}}};p.colors.fill(colors);}else if(index==4){p.colors[0].fill({1,.55,.15});p.shimmer=40;}}
 else{if(index==1)p.colors[0].fill({1,.65,.25});else if(index==2){p.colors[0].fill({.25,.65,1});p.length=1.5;}else if(index==3){p.source_color=true;p.length=2;}else if(index==4){p.shimmer=65;p.detail=80;p.length=2;}}
 return p;
}
bool valid(const Parameters& p){
 auto range=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 if((p.kind!=Kind::starglow&&p.kind!=Kind::shine)||p.input<0||p.input>5||p.blend<0||p.blend>2||p.map_mode<0||p.map_mode>2||(p.samples!=32&&p.samples!=64&&p.samples!=128))return false;
 if(!range(p.threshold,0,16)||!range(p.softness,0,16)||!range(p.boost,0,100)||!range(p.length,0,p.kind==Kind::shine?10:10000)||!range(p.rotation,-360000,360000)||!range(p.falloff,.05,20))return false;
 if(!range(p.cx,-1e7,1e7)||!range(p.cy,-1e7,1e7)||!range(p.mask_cx,-1e7,1e7)||!range(p.mask_cy,-1e7,1e7)||!range(p.mask_radius,0,1e7)||!range(p.mask_feather,0,1e7)||!range(p.shimmer,0,100)||!range(p.detail,.1,1000)||!range(p.phase,-1e9,1e9)||!range(p.loops,1,100)||!range(p.noise_x,-1e7,1e7)||!range(p.noise_y,-1e7,1e7))return false;
 if(!range(p.source_opacity,0,100)||!range(p.effect_opacity,0,100)||!range(p.mix,0,100))return false;
 for(auto v:p.lengths)if(!range(v,0,1000))return false;for(auto v:p.maps)if(v<0||v>2)return false;for(const auto& map:p.colors)for(const auto& color:map)for(auto v:color)if(!range(v,0,1))return false;return true;
}
Image downsample(const Image& src,int max_side,const Cancel& cancel,bool emissive){
 if(!src.valid()||max_side<1||stop(cancel))return {};const int scale=std::max(1,int(std::ceil(double(std::max(src.width,src.height))/max_side)));if(scale==1)return src;
 Image out{(src.width+scale-1)/scale,(src.height+scale-1)/scale,src.left,src.top,src.step*scale,{}};out.rgba.resize(std::size_t(out.width)*out.height*4);
 for(int y=0;y<out.height;y++){if(stop(cancel))return {};for(int x=0;x<out.width;x++){
  std::array<double,4> sum{};int count=0;for(int yy=y*scale;yy<std::min(src.height,(y+1)*scale);yy++)for(int xx=x*scale;xx<std::min(src.width,(x+1)*scale);xx++){const auto i=(std::size_t(yy)*src.width+xx)*4;const double a=emissive?finite(src.rgba[i+3]):unit(finite(src.rgba[i+3]));for(int k=0;k<3;k++)sum[k]+=finite(src.rgba[i+k])*(emissive?1:a);sum[3]+=a;++count;}
  const auto i=(std::size_t(y)*out.width+x)*4;for(int k=0;k<3;k++)out.rgba[i+k]=emissive?float(sum[k]/count):sum[3]>0?float(sum[k]/sum[3]):0;out.rgba[i+3]=float(sum[3]/count);
 }}return out;
}
bool same_extraction(const Parameters& a,const Parameters& b){return a.input==b.input&&a.threshold==b.threshold&&a.softness==b.softness&&a.use_mask==b.use_mask&&(!a.use_mask||(a.mask_cx==b.mask_cx&&a.mask_cy==b.mask_cy&&a.mask_radius==b.mask_radius&&a.mask_feather==b.mask_feather))&&a.shimmer==b.shimmer&&a.detail==b.detail&&a.phase==b.phase&&a.cycle==b.cycle&&a.loops==b.loops&&a.seed==b.seed&&a.noise_x==b.noise_x&&a.noise_y==b.noise_y;}
std::shared_ptr<const Image> prepare(const Image& src,const Parameters& p,const Cancel& cancel){
 if(!src.valid()||!valid(p)||stop(cancel))return {};auto out=std::make_shared<Image>(src);const double period=p.loops*360,phase=std::remainder(p.phase,period)*2*pi/period;
 if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if((y&7)==0&&cancelled())return false;for(int x=0;x<src.width;x++){
   const auto i=(y*src.width+x)*4;const double r=std::max(0.f,finite(src.rgba[i])),g=std::max(0.f,finite(src.rgba[i+1])),b=std::max(0.f,finite(src.rgba[i+2])),a=unit(finite(src.rgba[i+3]));
   const double v=p.input==0?(std::max({r,g,b})+std::min({r,g,b}))*.5:p.input==1?.2126*r+.7152*g+.0722*b:p.input==5?a:p.input==2?r:p.input==3?g:b;
   const double knee=p.softness,soft=knee>0?std::pow(std::clamp(v-p.threshold+knee,0.,2*knee),2)/(4*knee):0;double strength=std::min(v,std::max(v-p.threshold,soft));if(p.kind==Kind::starglow)strength/=std::max(.05,1-p.threshold);if(p.input!=5)strength*=a;
   const double xx=src.left+(x+.5)*src.step,yy=src.top+(double(y)+.5)*src.step;
   if(p.use_mask){const double distance=std::hypot(xx-p.mask_cx,yy-p.mask_cy);strength*=p.mask_feather>0?1-smooth((distance-p.mask_radius)/p.mask_feather):(distance<=p.mask_radius?1:0);}
   if(p.shimmer>0){const double frequency=p.detail*.001,tx=p.cycle?std::cos(phase)*2:p.phase*.01,ty=p.cycle?std::sin(phase)*2:p.phase*.00617;const double n=noise((xx+p.noise_x)*frequency+tx,(yy+p.noise_y)*frequency+ty,p.seed);strength*=std::lerp(1.,n*2,p.shimmer*.01);}
   for(int k=0;k<3;k++)out->rgba[i+k]=narrow(v>0?std::max(0.f,finite(src.rgba[i+k]))*strength/v:0);out->rgba[i+3]=narrow(strength);
  }}return true;
 }))return {};
 return out;
}
bool render_prepared(const Image& src,const Image& e,const Parameters& p,Image& out,const Cancel& cancel){
 pscpu::ParallelScope parallel;
 if(!src.valid()||!e.valid()||!valid(p)||src.width!=e.width||src.height!=e.height||src.left!=e.left||src.top!=e.top||src.step!=e.step||stop(cancel))return false;
 if(p.mix==0||((p.boost==0||p.effect_opacity==0||p.length==0)&&p.source_opacity==100)){out=src;return true;}
 Image rays{src.width,src.height,src.left,src.top,src.step,std::vector<float>(src.rgba.size())};if(!(p.kind==Kind::starglow?star(src,e,p,rays,cancel):shine(src,e,p,rays,cancel)))return false;
 const double source=p.source_opacity*.01,effect=p.effect_opacity*.01,mix=p.mix*.01;
 if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if((y&15)==0&&cancelled())return false;for(int x=0;x<src.width;x++){
   const auto i=(y*src.width+x)*4;const double before=unit(finite(src.rgba[i+3])),base=before*source,energy=std::max({0.f,rays.rgba[i],rays.rgba[i+1],rays.rgba[i+2]})*effect;
   const double halo=1-std::exp(-std::min(80.,energy)),alpha=p.preserve_alpha?before:unit(base+(1-base)*halo),final_alpha=std::lerp(before,alpha,mix);
   for(int k=0;k<3;k++){const double original=finite(src.rgba[i+k]),light=rays.rgba[i+k]*effect;double value=original*base+light;
    if(p.blend==1){const double a=original*base;value=1-(1-unit(a))*(1-unit(light))+std::max(0.,a-1)+std::max(0.,light-1)+std::min(0.,a);}
    else if(p.blend==2)value=original*base*(1-halo)+light;
    rays.rgba[i+k]=narrow(final_alpha>0?std::lerp(original*before,value,mix)/final_alpha:original);
   }rays.rgba[i+3]=float(final_alpha);
  }}return true;
 }))return false;
 out=std::move(rays);return true;
}
bool render(const Image& src,const Parameters& p,Image& out,const Cancel& cancel){
 pscpu::ParallelScope parallel;
 if(!valid(p)||!src.valid()||stop(cancel))return false;if(p.mix==0){out=src;return true;}auto e=prepare(src,p,cancel);return e&&render_prepared(src,*e,p,out,cancel);
}
} // namespace lightfx
