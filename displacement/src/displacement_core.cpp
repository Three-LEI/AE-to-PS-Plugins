#include "displacement_core.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
namespace displacement {
bool Image::valid() const {
  return width>0&&height>0&&std::isfinite(left)&&std::isfinite(top)&&
    std::isfinite(step)&&step>0&&std::isfinite(midpoint)&&midpoint>0&&midpoint<1&&
    std::size_t(width)<=std::numeric_limits<std::size_t>::max()/4/std::size_t(height)&&
    rgba.size()==std::size_t(width)*height*4;
}
bool valid(const Parameters& p){
  return int(p.horizontal)>=0&&int(p.horizontal)<=8&&int(p.vertical)>=0&&int(p.vertical)<=8&&
    int(p.placement)>=0&&int(p.placement)<=2&&std::isfinite(p.amount_x)&&std::isfinite(p.amount_y)&&
    std::abs(p.amount_x)<=10000&&std::abs(p.amount_y)<=10000;
}
namespace {
double modulo(double x,double n){return x-std::floor(x/n)*n;}
std::array<double,4> sample(const Image& im,double x,double y,bool wrap,bool premultiply){
  double u=(x-im.left)/im.step-.5,v=(y-im.top)/im.step-.5;
  if(wrap){u=modulo(u,im.width);v=modulo(v,im.height);}
  else if(u<-.999999||u>im.width||v<-.999999||v>im.height)return {};
  const int x0=int(std::floor(u)),y0=int(std::floor(v));
  const double fx=u-x0,fy=v-y0;
  std::array<double,4> result{};
  for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){
    int xx=x0+dx,yy=y0+dy;
    if(wrap){xx=(xx%im.width+im.width)%im.width;yy=(yy%im.height+im.height)%im.height;}
    else if(xx<0||xx>=im.width||yy<0||yy>=im.height)continue;
    const auto i=(std::size_t(yy)*im.width+xx)*4;
    const double weight=(dx?fx:1-fx)*(dy?fy:1-fy),a=std::clamp(double(im.rgba[i+3]),0.,1.);
    for(int c=0;c<3;++c){const double q=im.rgba[i+c];result[c]+=weight*(std::isfinite(q)?q:0)*(premultiply?a:1);}
    result[3]+=weight*a;
  }
  if(premultiply&&result[3]>0)for(int c=0;c<3;++c)result[c]/=result[3];
  return result;
}
double channel(std::array<double,4> q,Channel c,double mid){
  if(c==Channel::none)return mid;
  const double r=std::clamp(q[0],0.,1.),g=std::clamp(q[1],0.,1.),b=std::clamp(q[2],0.,1.);
  double value=0;
  const double hi=std::max({r,g,b}),lo=std::min({r,g,b}),d=hi-lo;
  switch(c){
    case Channel::red:value=r;break;case Channel::green:value=g;break;case Channel::blue:value=b;break;
    case Channel::alpha:return std::clamp(q[3],0.,1.);
    case Channel::luminance:value=.2126*r+.7152*g+.0722*b;break;
    case Channel::lightness:value=(hi+lo)*.5;break;
    case Channel::saturation:value=d==0?0:d/(1-std::abs(hi+lo-1));break;
    case Channel::hue:if(d==0)value=0;else{value=(hi==r?(g-b)/d:hi==g?(b-r)/d+2:(r-g)/d+4)/6;value=modulo(value,1);}break;
    default:return mid;
  }
  // Invisible map RGB is undefined: fade its displacement toward neutral.
  return mid+(value-mid)*std::clamp(q[3],0.,1.);
}
double displacement(double value,double mid){if(std::abs(value-mid)<1e-7)return 0;return value>=mid?(value-mid)/(1-mid):(value-mid)/mid;}
}
bool render(const Image& source,const Image& map,const Parameters& p,Region area,float* output){
  if(!source.valid()||!map.valid()||!valid(p)||!output||area.width<=0||area.height<=0||
     !std::isfinite(area.left)||!std::isfinite(area.top)||!std::isfinite(area.step)||area.step<=0)return false;
  const double sw=source.width*source.step,sh=source.height*source.step,
    mw=map.width*map.step,mh=map.height*map.step;
  for(int y=0;y<area.height;++y)for(int x=0;x<area.width;++x){
    const double wx=area.left+(x+.5)*area.step,wy=area.top+(y+.5)*area.step;
    double mx=wx-source.left,my=wy-source.top;
    if(p.placement==Placement::stretch){mx=map.left+mx*mw/sw;my=map.top+my*mh/sh;}
    else {mx=map.left+mx-(sw-mw)*.5;my=map.top+my-(sh-mh)*.5;}
    const bool tiling=p.placement==Placement::tile;
    const bool inside=tiling||(mx>=map.left&&mx<map.left+mw&&my>=map.top&&my<map.top+mh);
    double dx=0,dy=0;
    if(inside){
      if(!tiling){mx=std::clamp(mx,map.left+map.step*.5,map.left+mw-map.step*.5);my=std::clamp(my,map.top+map.step*.5,map.top+mh-map.step*.5);}
      const auto m=sample(map,mx,my,tiling,false);
      dx=p.amount_x*displacement(channel(m,p.horizontal,map.midpoint),map.midpoint);
      dy=p.amount_y*displacement(channel(m,p.vertical,map.midpoint),map.midpoint);
    }
    // Inverse image lookup: positive map displacement moves source content right/down.
    const auto color=sample(source,wx-dx,wy-dy,p.wrap,true);
    for(int c=0;c<4;++c)output[(std::size_t(y)*area.width+x)*4+c]=float(color[c]);
  }
  return true;
}
}
