#include "surface_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace surface {
Parameters defaults(Kind kind,int w,int h,int left,int top){Parameters p;p.kind=kind;p.light_x=left+w*(kind==Kind::blob?.25:.5);p.light_y=top+h*(kind==Kind::blob?.25:.5);if(kind==Kind::blob){p.ambient=75;p.diffuse=25;p.specular=100;}return p;}
bool valid(const Parameters& p){auto range=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 return (p.kind==Kind::blob||p.kind==Kind::glass)&&p.property>=0&&p.property<6&&range(p.softness,0,500)&&range(p.cut,0,100)&&range(p.height,-100,100)&&range(p.displacement,-500,500)&&p.light_type>=0&&p.light_type<2&&range(p.intensity,0,1000)&&range(p.light_height,-100,100)&&range(p.light_x,-1e7,1e7)&&range(p.light_y,-1e7,1e7)&&range(p.direction,-360000,360000)&&range(p.ambient,0,200)&&range(p.diffuse,0,100)&&range(p.specular,0,100)&&range(p.roughness,.00099,.5)&&range(p.metal,0,100)&&std::all_of(p.light_color.begin(),p.light_color.end(),[&](double c){return range(c,0,16);});
}
namespace {
double finite(float x){return std::isfinite(x)?double(x):0.;}
struct Vec {double x,y,z;};
Vec normalized(Vec v){const double n=std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);return n>1e-12?Vec{v.x/n,v.y/n,v.z/n}:Vec{0,0,1};}
double dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
struct Field {int w,h;std::vector<float> data;Field(int width,int height):w(width),h(height),data(std::size_t(w)*h){};float at(int x,int y)const{return data[std::size_t(std::clamp(y,0,h-1))*w+std::clamp(x,0,w-1)];}};
bool box(Field& field,int radius,const Cancel& cancel){
 if(radius<1)return true;Field tmp(field.w,field.h);
 if(!pscpu::parallel_ranges(field.h,field.data.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(int y=int(begin);y<int(end);++y){if(stop())return false;double sum=0;for(int i=-radius;i<=radius;++i)sum+=field.at(i,y);for(int x=0;x<field.w;++x){tmp.data[std::size_t(y)*field.w+x]=float(sum/(2*radius+1));sum+=field.at(x+radius+1,y)-field.at(x-radius,y);}}return true;}))return false;
 return pscpu::parallel_ranges(field.w,field.data.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(int x=int(begin);x<int(end);++x){if(stop())return false;double sum=0;for(int i=-radius;i<=radius;++i)sum+=tmp.at(x,i);for(int y=0;y<field.h;++y){field.data[std::size_t(y)*field.w+x]=float(sum/(2*radius+1));sum+=tmp.at(x,y+radius+1)-tmp.at(x,y-radius);}}return true;});
}
bool soften(Field& field,double softness,const Cancel& cancel){
 const double sigma=softness*.45;if(sigma<.1)return true;
 int lower=int(std::floor(std::sqrt(4*sigma*sigma+1)));if(!(lower&1))--lower;lower=std::max(1,lower);const int upper=lower+2;
 const int count=std::clamp(int(std::lround((12*sigma*sigma-3*lower*lower-12*lower-9)/(-4*lower-4))),0,3);
 for(int i=0;i<3;++i)if(!box(field,((i<count?lower:upper)-1)/2,cancel))return false;return true;
}
std::array<double,4> sample(const Image& image,double x,double y){
 if(!std::isfinite(x)||!std::isfinite(y))return {};
 auto mirror=[](double value,int size){if(size<=1)return 0.;const double end=size-1.,period=end*2;value=std::fmod(value,period);if(value<0)value+=period;return value>end?period-value:value;};
 x=mirror(x,image.width);y=mirror(y,image.height);
 const int ix=int(std::floor(x)),iy=int(std::floor(y));const double fx=x-ix,fy=y-iy;std::array<double,4> value{};
 for(int j=0;j<2;++j)for(int i=0;i<2;++i){const int xx=ix+i,yy=iy+j;if(xx<0||yy<0||xx>=image.width||yy>=image.height)continue;const auto n=(std::size_t(yy)*image.width+xx)*4;const double a=std::clamp(finite(image.rgba[n+3]),0.,1.),weight=(i?fx:1-fx)*(j?fy:1-fy);for(int k=0;k<3;++k)value[k]+=finite(image.rgba[n+k])*a*weight;value[3]+=a*weight;}
 if(value[3]>1e-12)for(int k=0;k<3;++k)value[k]/=value[3];return value;
}
}
bool render(const Image& source,const Parameters& p,Image& out,const Image* map,const Cancel& cancel){
 if(!source.valid()||!valid(p)||(map&&!map->valid())||(cancel&&cancel()))return false;
 if(p.kind==Kind::glass&&(p.height==0||p.displacement==0)&&p.ambient==100&&(p.diffuse==0||p.intensity==0)&&(p.specular==0||p.intensity==0)){out=source;return true;}
 if(p.kind==Kind::blob&&p.cut==100){Image empty=source;std::fill(empty.rgba.begin(),empty.rgba.end(),0);out=std::move(empty);return true;}
 const Image& bump=map?*map:source;Field field(source.width,source.height);
 if(!pscpu::parallel_ranges(source.height,field.data.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(int y=int(begin);y<int(end);++y){if(stop())return false;for(int x=0;x<source.width;++x){const int mx=x+source.left-bump.left,my=y+source.top-bump.top;double value=0;if(mx>=0&&my>=0&&mx<bump.width&&my<bump.height){const auto i=(std::size_t(my)*bump.width+mx)*4;const double r=std::clamp(finite(bump.rgba[i]),0.,1.),g=std::clamp(finite(bump.rgba[i+1]),0.,1.),b=std::clamp(finite(bump.rgba[i+2]),0.,1.),a=std::clamp(finite(bump.rgba[i+3]),0.,1.);value=p.property==0?r:p.property==1?g:p.property==2?b:p.property==3?1:p.property==4?.299*r+.587*g+.114*b:(std::max({r,g,b})+std::min({r,g,b}))*.5;value*=a;}field.data[std::size_t(y)*source.width+x]=float(value);}}return true;}))return false;
 if(!soften(field,p.softness,cancel))return false;
 const double cut=std::max(1./512,p.cut*.01);
 auto profile=[&](int x,int y){return std::sqrt(std::max(0.,(field.at(x,y)-cut)/std::max(1e-6,1-cut)));};
 const double angle=p.direction*std::numbers::pi/180,horizontal=100-std::abs(p.light_height);
 const Vec distant=normalized({std::sin(angle)*horizontal,-std::cos(angle)*horizontal,p.light_height});
 Image result{source.width,source.height,source.left,source.top,std::vector<float>(source.rgba.size())};
 if(!pscpu::parallel_ranges(source.height,field.data.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(int y=int(begin);y<int(end);++y){if(stop())return false;for(int x=0;x<source.width;++x){const auto i=(std::size_t(y)*source.width+x)*4;
  double gx,gy,dx,dy,coverage=1,z=field.at(x,y)*p.height;
  if(p.kind==Kind::glass){gx=(field.at(x,y)-field.at(x-1,y))*p.height;gy=(field.at(x,y)-field.at(x,y-1))*p.height;dx=gx*p.height*p.displacement*.012;dy=gy*p.height*p.displacement*.012;}
  else {
   const double value=field.at(x,y);if(value<1./512)continue;
   const double ax=std::abs(field.at(x+1,y)-field.at(x-1,y)),ay=std::abs(field.at(x,y+1)-field.at(x,y-1));coverage=std::clamp(.5+(value-cut)/std::max(1e-6,(ax+ay)*.5),0.,1.);if(coverage==0)continue;
   const double center=profile(x,y);gx=(profile(x+2,y)-center)*6;gy=(profile(x,y+2)-center)*6;dx=-gx*36;dy=-gy*36;z=center*6;
  }
  const Vec normal=normalized({-gx,-gy,1});const Vec light=p.light_type?normalized({p.light_x-source.left-x,p.light_y-source.top-y,p.light_height*std::max(source.width,source.height)*.01-z}):distant;
  const double nl=std::max(0.,dot(normal,light)),reflected=std::max(0.,2*nl*normal.z-light.z),shine=nl>0?std::pow(reflected,1/p.roughness):0;
  auto color=sample(source,x+dx,y+dy);color[3]*=coverage;if(color[3]<=1e-12)continue;
  for(int k=0;k<3;++k){const double light_amount=p.intensity*.01*p.light_color[k];const double diffuse=nl*p.diffuse*.01*light_amount,specular=shine*p.specular*.01*light_amount;const double tinted=color[k]*p.metal*.01+(1-p.metal*.01);const double shaded=color[k]*(p.ambient*.01+diffuse)+tinted*specular;result.rgba[i+k]=float(std::clamp(shaded,-1e8,1e8));}result.rgba[i+3]=float(color[3]);
 }}return true;}))return false;
 out=std::move(result);return true;
}
}
