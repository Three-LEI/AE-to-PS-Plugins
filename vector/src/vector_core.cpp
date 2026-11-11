#include "vector_core.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>
namespace vectorblur {
namespace {
constexpr float pi=3.14159265358979323846f;
bool stopped(const Cancel& cancel){return cancel&&cancel();}
float finite(float v){return std::isfinite(v)?v:0.f;}
float signed_pow(float x,float p){return std::copysign(std::pow(std::abs(x),p),x);}
float clamp01(float v){return std::clamp(finite(v),0.f,1.f);}
int coordinate(int x,int n,Edge edge){
 if(x>=0&&x<n)return x;
 if(edge==Edge::transparent)return -1;
 if(edge==Edge::repeat)return std::clamp(x,0,n-1);
 if(edge==Edge::wrap){x%=n;return x<0?x+n:x;}
 if(n==1)return 0;
 const int period=2*(n-1);x%=period;if(x<0)x+=period;return x<n?x:period-x;
}
struct RGBA {float r=0,g=0,b=0,a=0;};
RGBA sample(const Image& image,float x,float y,Edge edge){
 const int ix=int(std::floor(x)),iy=int(std::floor(y));const float fx=x-ix,fy=y-iy;RGBA out;
 for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx){
  const int px=coordinate(ix+xx,image.width,edge),py=coordinate(iy+yy,image.height,edge);if(px<0||py<0)continue;
  const auto i=(std::size_t(py)*image.width+px)*4;const float w=(xx?fx:1-fx)*(yy?fy:1-fy);
  out.r+=image.rgba[i]*w;out.g+=image.rgba[i+1]*w;out.b+=image.rgba[i+2]*w;out.a+=image.rgba[i+3]*w;
 }
 return out;
}
// Sliding box passes approximate a Gaussian in O(pixels), independent of radius.
bool soften(std::vector<float>& a,int w,int h,double sigma,const Cancel& cancel){
 if(sigma<.01)return true;std::vector<float> b(a.size());
 const double ideal=std::sqrt(4*sigma*sigma+1);int low=int(std::floor(ideal));if(!(low&1))--low;low=std::max(1,low);
 const int high=low+2,count=std::clamp(int(std::lround((12*sigma*sigma-3*low*low-12*low-9)/(-4.*low-4))),0,3);
 for(int pass=0;pass<3;++pass){const int r=((pass<count?low:high)-1)/2;if(!r)continue;const float norm=1.f/(2*r+1);
  for(int y=0;y<h;++y){if(stopped(cancel))return false;double sum=0;auto value=[&](int x){return a[std::size_t(y)*w+std::clamp(x,0,w-1)];};
   for(int x=-r;x<=r;++x)sum+=value(x);
   for(int x=0;x<w;++x){b[std::size_t(y)*w+x]=float(sum*norm);sum+=value(x+r+1)-value(x-r);}
  }
  for(int x=0;x<w;++x){if(stopped(cancel))return false;double sum=0;auto value=[&](int y){return b[std::size_t(std::clamp(y,0,h-1))*w+x];};
   for(int y=-r;y<=r;++y)sum+=value(y);
   for(int y=0;y<h;++y){a[std::size_t(y)*w+x]=float(sum*norm);sum+=value(y+r+1)-value(y-r);}
  }
 }return true;
}
float property(RGBA c,Property p){
 if(p==Property::alpha)return c.a;
 if(p==Property::red)return c.r;if(p==Property::green)return c.g;if(p==Property::blue)return c.b;
 const float hi=std::max({c.r,c.g,c.b}),lo=std::min({c.r,c.g,c.b}),d=hi-lo;
 if(p==Property::luminance)return .2126f*c.r+.7152f*c.g+.0722f*c.b;
 if(p==Property::lightness)return (hi+lo)*.5f;
 if(p==Property::saturation)return hi>1.e-8f?d/hi:0;
 if(d<=1.e-8f)return 0;float hue=hi==c.r?(c.g-c.b)/d:hi==c.g?2+(c.b-c.r)/d:4+(c.r-c.g)/d;
 hue/=6;return hue-std::floor(hue);
}
template<class Row> bool rows(int w,int h,const Cancel& cancel,Row row){
 // Cancellation remains on the calling thread: Photoshop's abortProc is not a worker API.
 if(std::size_t(w)*h<65536){for(int y=0;y<h;++y){if(stopped(cancel))return false;if(!row(y,[&]{return stopped(cancel);}))return false;}return true;}
 std::atomic<int> next{0},done{0};std::atomic<bool> stop{false};
 const int count=std::clamp(int(std::thread::hardware_concurrency())-1,1,8);std::vector<std::jthread> workers;workers.reserve(count);
 for(int n=0;n<count;++n)workers.emplace_back([&]{while(!stop.load(std::memory_order_relaxed)){const int y=next.fetch_add(1);if(y>=h)break;if(!row(y,[&]{return stop.load(std::memory_order_relaxed);})){stop=true;break;}}++done;});
 while(done.load()!=count){if(stopped(cancel))stop=true;std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 return !stop.load();
}
}
bool Image::valid() const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::uint64_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
bool valid(const Parameters& p){
 auto bounded=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 return p.kernel_version==2&&int(p.type)>=0&&int(p.type)<5&&int(p.property)>=0&&int(p.property)<8&&int(p.edge)>=0&&int(p.edge)<4&&p.quality>=0&&p.quality<4&&
  bounded(p.amount,-2000,2000)&&bounded(p.angle,-360000,360000)&&bounded(p.ridge,.05,100)&&bounded(p.revolutions,-100,100)&&bounded(p.map_softness,0,500)&&bounded(p.mix,0,100);
}
Parameters preset(int index){Parameters p;
 if(index==1){p.amount=65;p.map_softness=8;p.ridge=.6;}
 if(index==2){p.type=Type::perpendicular;p.amount=90;p.map_softness=12;p.ridge=.4;}
 if(index==3){p.type=Type::direction_fading;p.amount=100;p.revolutions=1.5;p.map_softness=20;}
 if(index==4){p.type=Type::direction_center;p.amount=95;p.revolutions=3;p.property=Property::hue;p.map_softness=6;}
 return p;
}
Vec Field::at(float x,float y) const{
 if(vectors.empty()||!std::isfinite(x)||!std::isfinite(y))return {};x=std::clamp(x,0.f,float(width-1));y=std::clamp(y,0.f,float(height-1));
 const int ix=int(x),iy=int(y),nx=std::min(ix+1,width-1),ny=std::min(iy+1,height-1);const float fx=x-ix,fy=y-iy;Vec out;
 for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx){const auto v=vectors[std::size_t(yy?ny:iy)*width+(xx?nx:ix)];const float weight=(xx?fx:1-fx)*(yy?fy:1-fy);out.x+=v.x*weight;out.y+=v.y*weight;}return out;
}
bool make_field(const Image& source,const Parameters& p,Field& result,const Image* map,const Cancel& cancel){
 result={};if(!source.valid()||!valid(p)||(map&&!map->valid())||stopped(cancel))return false;
 const auto& input=map?*map:source;const int w=source.width,h=source.height;const auto size=std::size_t(w)*h;
 std::vector<float> scalar(size),sine,cosine,coverage;const bool hue=p.property==Property::hue;
 if(hue){sine.resize(size);cosine.resize(size);coverage.resize(size);}
 for(int y=0;y<h;++y){if(stopped(cancel))return false;for(int x=0;x<w;++x){
  auto c=sample(input,float(source.left+x-input.left),float(source.top+y-input.top),Edge::transparent);c.a=clamp01(c.a);
  // Map properties use display-range values; source HDR is retained separately.
  if(c.a>1.e-8f){c.r=clamp01(c.r);c.g=clamp01(c.g);c.b=clamp01(c.b);}else c.r=c.g=c.b=0;
  float value=property(c,p.property);if(p.property!=Property::alpha&&!hue)value*=c.a;
  if(p.invert_map)value=1-value;const auto i=std::size_t(y)*w+x;scalar[i]=finite(value);
  if(hue){sine[i]=std::sin(2*pi*value)*c.a;cosine[i]=std::cos(2*pi*value)*c.a;coverage[i]=c.a;}
 }}
 // Calibrated against same-source AE renders. The UI softness is not Gaussian sigma.
 const double sigma=p.map_softness*.4;
 if(hue){if(!soften(sine,w,h,sigma,cancel)||!soften(cosine,w,h,sigma,cancel)||!soften(coverage,w,h,sigma,cancel))return false;
  for(std::size_t i=0;i<size;++i){const float angle=coverage[i]>1.e-8f?std::atan2(sine[i],cosine[i])/(2*pi):0;scalar[i]=angle-std::floor(angle);}
 }else if(!soften(scalar,w,h,sigma,cancel))return false;
 Field field;field.width=w;field.height=h;field.vectors.resize(size);
 const float angle=float(std::remainder(p.angle,360.)*pi/180),cs=std::cos(angle),sn=std::sin(angle);
 auto at=[&](int x,int y){return scalar[std::size_t(std::clamp(y,0,h-1))*w+std::clamp(x,0,w-1)];};
 auto difference=[&](float a,float b){float d=a-b;if(hue)d-=std::round(d);return d;};
 for(int y=0;y<h;++y){if(stopped(cancel))return false;for(int x=0;x<w;++x){Vec v;
  if(p.type==Type::direction_center||p.type==Type::direction_fading){const float a=float(pi*.5+2*pi*p.revolutions*(at(x,y)-.5f));v={std::cos(a),std::sin(a)};}
  else{
   // Backward differences matter: a centred 3x3 derivative shifts the ridges.
   // Normalise the height-field slope with a Z component, rather than tanh/clamping.
   const float gx=difference(at(x,y),at(x-1,y))*510;
   const float gy=difference(at(x,y),at(x,y-1))*510;
   const float normal=std::hypot(std::hypot(gx,gy),float(p.ridge));v={gx/normal,gy/normal};
   if(p.type==Type::perpendicular)v={-v.y,v.x};
  }
  field.vectors[std::size_t(y)*w+x]={v.x*cs-v.y*sn,v.x*sn+v.y*cs};
 }}result=std::move(field);return true;
}
bool render(const Image& source,const Parameters& p,Image& output,const Image* map,const Cancel& cancel,Field* diagnostic){
 output={};if(!source.valid()||!valid(p)||(map&&!map->valid())||stopped(cancel))return false;
 if((p.amount==0||p.mix==0)&&!diagnostic){output=source;return true;}
 // Sanitize and associate before interpolation; transparent hidden RGB never contributes.
 Image associated=source;for(std::size_t i=0;i<associated.rgba.size();i+=4){
  if((i&32767)==0&&stopped(cancel))return false;const float a=clamp01(source.rgba[i+3]);associated.rgba[i+3]=a;
  for(int k=0;k<3;++k){float value=finite(source.rgba[i+k]);if(p.linear_light)value=signed_pow(value,2.2f);associated.rgba[i+k]=value*a;}
 }
 Field field;if(!make_field(source,p,field,map,cancel))return false;
 if(p.amount==0||p.mix==0){output=source;if(diagnostic)*diagnostic=std::move(field);return true;}
 Image out{source.width,source.height,source.left,source.top,std::vector<float>(source.rgba.size())};
 const int caps[]={24,64,128,256};const bool natural=p.type==Type::natural;
 const bool one_sided=natural||p.type==Type::direction_fading;
 const int steps=std::clamp(int(std::ceil(std::abs(p.amount))),2,one_sided?caps[p.quality]:caps[p.quality]/2);
 const float distance=float(p.amount)*(natural?-1.f:1.f)*(one_sided?1.f:.5f),step=distance/steps,mix=float(p.mix*.01);
 const bool uniform=p.type!=Type::direction_fading;
 const bool ok=rows(source.width,source.height,cancel,[&](int y,const auto& cancel_row){
  for(int x=0;x<source.width;++x){if(!(x&31)&&cancel_row())return false;
   const auto i=(std::size_t(y)*source.width+x)*4;RGBA total{associated.rgba[i],associated.rgba[i+1],associated.rgba[i+2],associated.rgba[i+3]};float weight_sum=1;
   const auto origin=field.at(float(x),float(y));
   for(int side=0;side<(one_sided?1:2);++side){float px=float(x),py=float(y),direction=side?-1.f:1.f;
    for(int n=1;n<=steps;++n){
     auto v=p.follow?field.at(px,py):origin;
     if(p.follow)v=field.at(px+v.x*step*direction*.5f,py+v.y*step*direction*.5f);
     px+=v.x*step*direction;py+=v.y*step*direction;
     const auto color=sample(associated,px,py,p.edge);const float weight=uniform?1.f:1.f-float(n)/(steps+1);
     total.r+=color.r*weight;total.g+=color.g*weight;total.b+=color.b*weight;total.a+=color.a*weight;weight_sum+=weight;
    }
   }
   const float a=std::clamp(total.a/weight_sum*mix+associated.rgba[i+3]*(1-mix),0.f,1.f);
   const float values[]={total.r,total.g,total.b};
   for(int k=0;k<3;++k){float value=a>1.e-8f?(values[k]/weight_sum*mix+associated.rgba[i+k]*(1-mix))/a:0;if(p.linear_light)value=signed_pow(value,1/2.2f);out.rgba[i+k]=finite(value);}
   out.rgba[i+3]=p.preserve_alpha?clamp01(source.rgba[i+3]):a;
  }return true;
 });
 if(!ok)return false;output=std::move(out);if(diagnostic)*diagnostic=std::move(field);return true;
}
}
