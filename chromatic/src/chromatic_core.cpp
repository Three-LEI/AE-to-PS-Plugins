#include "chromatic_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace chromatic {
namespace {
using Pixel=std::array<float,4>;
constexpr double pi=3.14159265358979323846;
float unit(float x){return std::clamp(std::isfinite(x)?x:0.f,0.f,1.f);}
float finite(float x){return std::isfinite(x)?std::clamp(x,-65504.f,65504.f):0.f;}
bool stop(const Cancel& c){return c&&c();}
double modulo(double x,double n){return x-std::floor(x/n)*n;}
int border(int p,int length,Edge edge){
 if(p>=0&&p<length)return p;
 if(edge==Edge::transparent)return -1;
 if(edge==Edge::repeat)return std::clamp(p,0,length-1);
 if(edge==Edge::wrap)return (p%length+length)%length;
 if(length==1)return 0;
 const int period=2*(length-1),v=(p%period+period)%period;return v<length?v:period-v;
}
// Associated working pixels. Decode once at the end, including coverage, so
// transparent and opaque-black material has consistent interpolation brightness.
Pixel sample(const Image& im,double x,double y,Edge edge){
 if(edge==Edge::repeat){x=std::clamp(x,0.,double(im.width-1));y=std::clamp(y,0.,double(im.height-1));}
 else if(edge==Edge::wrap){x=modulo(x,im.width);y=modulo(y,im.height);}
 else if(edge==Edge::reflect){
  x=im.width==1?0:modulo(x,2*(im.width-1));if(x>im.width-1)x=2*(im.width-1)-x;
  y=im.height==1?0:modulo(y,2*(im.height-1));if(y>im.height-1)y=2*(im.height-1)-y;
 }else if(x<=-1||y<=-1||x>=im.width||y>=im.height)return {};
 const int ix=int(std::floor(x)),iy=int(std::floor(y));const float fx=float(x-ix),fy=float(y-iy);Pixel out{};
 for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){const int xx=border(ix+dx,im.width,edge),yy=border(iy+dy,im.height,edge);if(xx<0||yy<0)continue;
  const float w=(dx?fx:1-fx)*(dy?fy:1-fy);const auto i=(std::size_t(yy)*im.width+xx)*4;
  for(int k=0;k<4;++k)out[k]+=im.rgba[i+k]*w;
 }
 return out;
}
double transfer(double x,double gamma){return gamma==1?x:std::copysign(std::pow(std::abs(x),gamma),x);}
double gate(const Image& src,std::size_t i,const Parameters& p){
 if(!p.highlights_only)return 1;
 const double value=std::max(0.,.2126*finite(src.rgba[i])+.7152*finite(src.rgba[i+1])+.0722*finite(src.rgba[i+2]));
 const double threshold=p.threshold*.01,knee=p.knee*.01;
 if(knee==0)return value>=threshold?1:0;
 const double t=std::clamp((value-threshold+knee)/(2*knee),0.,1.);return t*t*(3-2*t);
}
bool box(Image& image,double radius,bool vertical,Edge edge,const Cancel& cancel){
 if(radius<1e-6)return true;
 const int n=vertical?image.height:image.width,lines=vertical?image.width:image.height;
 if(n==1&&edge!=Edge::transparent)return true;
 const int r=int(std::floor(radius)),padding=r+2;
 const double f=radius-r,divisor=2*radius+1;
 std::vector<float> next(image.rgba.size());std::vector<double> prefix(std::size_t(n+padding*2+1)*4);
 for(int line=0;line<lines;++line){if(stop(cancel))return false;
  auto pos=[&](int i){return (vertical?std::size_t(i)*image.width+line:std::size_t(line)*image.width+i)*4;};
  for(int k=0;k<4;++k)prefix[k]=0;
  for(int i=-padding;i<n+padding;++i){const int source=border(i,n,edge);const auto j=std::size_t(i+padding)*4;
   for(int k=0;k<4;++k)prefix[j+4+k]=prefix[j+k]+(source>=0?image.rgba[pos(source)+k]:0);
  }
  for(int i=0;i<n;++i)for(int k=0;k<4;++k){
   const int lo=i-r+padding,hi=i+r+padding;const double a=prefix[std::size_t(lo)*4+k]-prefix[std::size_t(lo-1)*4+k],b=prefix[std::size_t(hi+2)*4+k]-prefix[std::size_t(hi+1)*4+k];
   const double sum=prefix[std::size_t(hi+1)*4+k]-prefix[std::size_t(lo)*4+k]+f*(a+b);
   next[pos(i)+k]=float(sum/divisor);
  }
 }
 image.rgba.swap(next);return true;
}
bool blur(Image& image,double sigma,Edge edge,const Cancel& cancel){
 const double r=(std::sqrt(4*sigma*sigma+1)-1)*.5;
 for(int pass=0;pass<3;++pass)if(!box(image,r,false,edge,cancel)||!box(image,r,true,edge,cancel))return false;
 return true;
}
struct Node {double shift;std::array<double,3> weight;};
std::array<double,3> hue_rotate(std::array<double,3> rgb,double degrees,double saturation){
 const double hi=std::max({rgb[0],rgb[1],rgb[2]}),lo=std::min({rgb[0],rgb[1],rgb[2]}),delta=hi-lo;
 double hue=delta==0?0:hi==rgb[0]?(rgb[1]-rgb[2])/delta:hi==rgb[1]?(rgb[2]-rgb[0])/delta+2:(rgb[0]-rgb[1])/delta+4;
 hue=modulo(hue+degrees/60,6);const double chroma=delta,second=chroma*(1-std::abs(modulo(hue,2)-1));std::array<double,3> out{};
 const int sector=int(hue);switch(sector){case 0:out={chroma,second,0};break;case 1:out={second,chroma,0};break;case 2:out={0,chroma,second};break;case 3:out={0,second,chroma};break;case 4:out={second,0,chroma};break;default:out={chroma,0,second};break;}
 for(auto& v:out)v+=lo;const double mean=(out[0]+out[1]+out[2])/3;
 for(auto& v:out)v=std::max(0.,mean+(v-mean)*saturation*.01);
 return out;
}
std::vector<Node> spectrum(const Parameters& p){
 std::vector<Node> nodes;std::array<double,3> totals{};
 const double reference=std::pow(550.,-p.dispersion_power),denominator=std::pow(380.,-p.dispersion_power)-reference;
 for(int n=0;n<p.samples;++n){const double wavelength=380.+320.*(n+.5)/p.samples;
  auto gaussian=[&](double center,double sigma){const double x=(wavelength-center)/sigma;return std::exp(-.5*x*x);};
  auto weight=hue_rotate({gaussian(615,43),gaussian(540,34),gaussian(455,36)},p.hue,p.saturation);
  nodes.push_back({-(std::pow(wavelength,-p.dispersion_power)-reference)/denominator,weight});
  for(int k=0;k<3;++k)totals[k]+=weight[k];
 }
 for(auto& node:nodes)for(int k=0;k<3;++k)node.weight[k]/=totals[k];
 return nodes;
}
bool identity(const Parameters& p){
 if(p.mix==0)return true;
 if(p.amount!=0)return false;
 if(p.mode==Mode::spectrum)return p.softness==0;
 for(const auto& c:p.channels)if(c.x!=0||c.y!=0||c.scale!=100||c.rotation!=0||c.blur!=0)return false;
 return true;
}
}
bool Image::valid() const {return width>0&&height>0&&width<=2000000&&height<=2000000&&std::abs(std::int64_t(left))<=2000000&&std::abs(std::int64_t(top))<=2000000&&std::size_t(width)*height<=std::size_t(16)*1024*1024&&rgba.size()==std::size_t(width)*height*4;}
bool valid(const Parameters& p){
 const double numbers[]={p.amount,p.angle,p.mix,p.center_x,p.center_y,p.protect,p.falloff_power,p.softness,p.dispersion_power,p.hue,p.saturation,p.gamma,p.threshold,p.knee};
 for(double v:numbers)if(!std::isfinite(v))return false;
 if(int(p.mode)<0||int(p.mode)>1||int(p.geometry)<0||int(p.geometry)>2||int(p.edge)<0||int(p.edge)>3||int(p.alpha)<0||int(p.alpha)>2||p.mask_mode<0||p.mask_mode>1||std::abs(p.amount)>4096||std::abs(p.angle)>360000||p.mix<0||p.mix>100||p.center_x<0||p.center_x>1||p.center_y<0||p.center_y>1||p.protect<0||p.protect>=100||p.falloff_power<.1||p.falloff_power>8||p.softness<0||p.softness>50||p.dispersion_power<.5||p.dispersion_power>4||std::abs(p.hue)>360000||p.saturation<0||p.saturation>200||p.samples<8||p.samples>64||p.gamma<1||p.gamma>3||p.threshold<0||p.threshold>1000||p.knee<0||p.knee>100)return false;
 for(const auto& c:p.channels)if(!std::isfinite(c.amount)||!std::isfinite(c.x)||!std::isfinite(c.y)||!std::isfinite(c.scale)||!std::isfinite(c.rotation)||!std::isfinite(c.blur)||std::abs(c.amount)>4||std::abs(c.x)>4096||std::abs(c.y)>4096||c.scale<1||c.scale>1000||std::abs(c.rotation)>360000||c.blur<0||c.blur>50)return false;
 return true;
}
Parameters preset(int index){
 Parameters p;
 if(index==0){p.mode=Mode::rgb;p.geometry=Geometry::radial;p.amount=6;p.angle=0;p.falloff=true;p.protect=5;p.falloff_power=1.8;p.softness=0;p.edge=Edge::repeat;}
 else if(index==1){p.mode=Mode::rgb;p.amount=8;p.angle=0;p.softness=0;}
 else if(index==2){p.amount=42;p.angle=28;p.softness=.8;}
 else if(index==3){p.amount=90;p.angle=-35;p.softness=1.6;p.saturation=130;p.samples=32;p.highlights_only=true;p.threshold=35;p.knee=20;}
 return p;
}
bool render(const Image& src,const Parameters& p,Image& output,const Image* mask,const Cancel& cancel){
 if(!src.valid()||!valid(p)||(mask&&!mask->valid())||stop(cancel))return false;
 if(identity(p)&&p.alpha==Alpha::expand){output=src;return true;}
 const double gamma=p.linear_light?p.gamma:1;
 Image seed{src.width,src.height,src.left,src.top,{}};seed.rgba.resize(src.rgba.size());
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& check){
 for(int y=int(first);y<int(last);++y){if(check())return false;for(int x=0;x<src.width;++x){const auto i=(std::size_t(y)*src.width+x)*4;const double a=unit(src.rgba[i+3])*gate(src,i,p);
  for(int k=0;k<3;++k)seed.rgba[i+k]=float(transfer(finite(src.rgba[i+k])*a,gamma));seed.rgba[i+3]=float(std::pow(a,gamma));
 }}
 return true;}))return false;
 std::array<Image,3> softened;std::array<const Image*,3> inputs{&seed,&seed,&seed};
 if(p.mode==Mode::spectrum&&p.softness>0){softened[0]=seed;if(!blur(softened[0],p.softness,p.edge,cancel))return false;inputs[0]=&softened[0];}
 if(p.mode==Mode::rgb)for(int k=0;k<3;++k)if(p.channels[k].blur>0){softened[k]=seed;if(!blur(softened[k],p.channels[k].blur,p.edge,cancel))return false;inputs[k]=&softened[k];}
 const auto nodes=p.mode==Mode::spectrum?spectrum(p):std::vector<Node>{};
 const double cx=p.center_x*src.width-.5,cy=p.center_y*src.height-.5,diagonal=.5*std::hypot(src.width,src.height),angle=p.angle*pi/180,cs=std::cos(angle),sn=std::sin(angle),amount=p.amount*(p.reverse?-1:1);
 std::array<double,3> cosines{},sines{},scales{};
 for(int k=0;k<3;++k){const double a=p.channels[k].rotation*pi/180;cosines[k]=std::cos(a);sines[k]=std::sin(a);scales[k]=100/p.channels[k].scale;}
 Image result{src.width,src.height,src.left,src.top,{}};result.rgba.resize(src.rgba.size());
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& check){
 for(int y=int(first);y<int(last);++y){if(check())return false;for(int x=0;x<src.width;++x){
  if((x&127)==0&&check())return false;
  const auto i=(std::size_t(y)*src.width+x)*4;double mix=p.mix*.01;
  if(mask){const auto q=sample(*mask,(x+.5)*mask->width/src.width-.5,(y+.5)*mask->height/src.height-.5,Edge::repeat);
   double m=p.mask_mode==0?unit(q[3]):unit(.2126f*q[0]+.7152f*q[1]+.0722f*q[2])*unit(q[3]);mix*=p.invert_mask?1-m:m;}
  if(mix==0){std::copy_n(src.rgba.data()+i,4,result.rgba.data()+i);continue;}
  const double dx=x-cx,dy=y-cy,distance=std::hypot(dx,dy);
  double vx=cs,vy=sn,falloff=1;
  if(p.geometry!=Geometry::directional){const double ux=distance>1e-12?dx/distance:0,uy=distance>1e-12?dy/distance:0;
   vx=ux*cs-uy*sn;vy=ux*sn+uy*cs;if(p.geometry==Geometry::tangential){const double old=vx;vx=-vy;vy=old;}}
  if(p.falloff){const double inner=p.protect*.01,t=std::clamp((distance/diagonal-inner)/(1-inner),0.,1.);falloff=std::pow(t*t*(3-2*t),p.falloff_power);if(p.invert_falloff)falloff=1-falloff;}
  vx*=amount*falloff;vy*=amount*falloff;
  std::array<double,3> color{},alpha{};
  if(p.mode==Mode::rgb){for(int k=0;k<3;++k){const auto& c=p.channels[k];const double xx=x-cx-c.x-vx*c.amount,yy=y-cy-c.y-vy*c.amount;
    const auto q=sample(*inputs[k],cx+(xx*cosines[k]+yy*sines[k])*scales[k],cy+(-xx*sines[k]+yy*cosines[k])*scales[k],p.edge);color[k]=q[k];alpha[k]=std::max(0.f,q[3]);}}
  else for(const auto& node:nodes){const auto q=sample(*inputs[0],x-vx*node.shift,y-vy*node.shift,p.edge);for(int k=0;k<3;++k){color[k]+=node.weight[k]*q[k];alpha[k]+=node.weight[k]*std::max(0.f,q[3]);}}
  for(int k=0;k<3;++k){color[k]=transfer(color[k],1/gamma);alpha[k]=std::pow(alpha[k],1/gamma);}
  const double original_alpha=unit(src.rgba[i+3]),remainder=p.highlights_only?1-gate(src,i,p):0;
  const double affected_alpha=std::min(1.,original_alpha*remainder+std::max({alpha[0],alpha[1],alpha[2]}));
  for(int k=0;k<3;++k)color[k]+=finite(src.rgba[i+k])*original_alpha*remainder;
  double final_alpha=affected_alpha;
  if(p.alpha==Alpha::preserve){for(auto& c:color)c=affected_alpha>1e-12?c/affected_alpha*original_alpha:0;final_alpha=original_alpha;}
  else if(p.alpha==Alpha::black)final_alpha=1;
  const double out_alpha=original_alpha*(1-mix)+final_alpha*mix;
  for(int k=0;k<3;++k){const double value=finite(src.rgba[i+k])*original_alpha*(1-mix)+color[k]*mix;result.rgba[i+k]=finite(float(out_alpha>1e-12?value/out_alpha:0));}
  result.rgba[i+3]=unit(float(out_alpha));
 }}
 return true;}))return false;
 if(stop(cancel))return false;output=std::move(result);return true;
}
}
