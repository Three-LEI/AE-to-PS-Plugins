#include "pattern_core.hpp"
#include <cmath>
namespace patterns {
namespace {
double smooth(double v){v=std::clamp(v,0.,1.);return v*v*(3-2*v);}
double modulo(double x,double period){return x-period*std::floor(x/period);}
double period(double value){return std::max(.01,std::abs(value));}
struct Shader {
 Kind kind;const Parameters& p;double width,height,cs,sn;mutable double tile_mean=-1;
 Shader(Kind k,const Parameters& params):kind(k),p(params){
  width=p.size_from==SizeFrom::corner?p.corner_x-p.anchor_x:p.width;
  height=p.size_from==SizeFrom::corner?p.corner_y-p.anchor_y:p.size_from==SizeFrom::width?p.width:p.height;
  width=std::copysign(period(width),width);height=std::copysign(period(height),height);
  const double angle=std::remainder(p.rotation,360.)*3.14159265358979323846/180;cs=std::cos(angle);sn=std::sin(angle);
 }
 double at(double x,double y) const {
  x-=p.anchor_x;y-=p.anchor_y;const double xx=x*cs+y*sn,yy=-x*sn+y*cs;
  double alpha;
  if(kind==Kind::grid){
   const double dx=std::abs(std::remainder(xx,std::abs(width))),dy=std::abs(std::remainder(yy,std::abs(height)));
   const double ax=std::max(0.,(p.border*.5-dx)/(p.feather_x+1)),ay=std::max(0.,(p.border*.5-dy)/(p.feather_y+1));
   alpha=p.feather_x>0||p.feather_y>0?smooth(std::hypot(ax,ay)):std::clamp(ax+ay,0.,1.);
  }else{
   const double tx=modulo(xx/width,2),ty=modulo(yy/height,2);
   const bool even=(tx<1)==(ty<1);
   const double ux=modulo(xx,std::abs(width)),uy=modulo(yy,std::abs(height));
   const double dx=std::min(ux,std::abs(width)-ux)/(p.feather_x+1),dy=std::min(uy,std::abs(height)-uy)/(p.feather_y+1);
   const double value=.5+(even?1:-1)*std::min(dx,dy);
   alpha=p.feather_x>0||p.feather_y>0?smooth(value):std::clamp(value,0.,1.);
  }
  return p.invert?1-alpha:alpha;
 }
 double average(double x,double y,double footprint) const {
  if(footprint<=1)return at(x,y);
  // Bounded view-only area sampling. Tiny repeated cells converge to their tile mean.
  double a=0;const int n=std::min(8,std::max(2,int(std::ceil(footprint))));
  if(footprint>std::max(std::abs(width),std::abs(height))*4){
   if(tile_mean<0){
    if(kind==Kind::checker)tile_mean=.5;
    else{constexpr int side=48;double sum=0;for(int j=0;j<side;++j)for(int i=0;i<side;++i){const double lx=(i+.5)*std::abs(width)/side,ly=(j+.5)*std::abs(height)/side;sum+=at(p.anchor_x+lx*cs-ly*sn,p.anchor_y+lx*sn+ly*cs);}tile_mean=sum/(side*side);}
   }return tile_mean;
  }else for(int j=0;j<n;++j)for(int i=0;i<n;++i)a+=at(std::round(x+((i+.5)/n-.5)*footprint),std::round(y+((j+.5)/n-.5)*footprint));
  return a/(n*n);
 }
};
std::uint32_t random_at(double x,double y){std::uint32_t h=std::uint32_t(std::int64_t(std::floor(x)))*0x9e3779b9u^std::uint32_t(std::int64_t(std::floor(y)))*0x85ebca6bu;h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return h^(h>>16);}
Pixel shade(const Shader& shader,double x,double y,Pixel source,double footprint,bool locked){
 const auto& p=shader.p;const float alpha=float(shader.average(x,y,footprint)*p.opacity*.01);
 const Pixel foreground{float(p.color[0]),float(p.color[1]),float(p.color[2]),alpha};return locked?pscore::composite_locked(source,foreground,p.blend,random_at(x,y)):pscore::composite(source,foreground,p.blend,random_at(x,y));
}
}
Parameters defaults(Kind kind,double width,double height,double left,double top){
 Parameters p;p.anchor_x=left+width*.5;p.anchor_y=top+height*.5;p.corner_x=left+width*(kind==Kind::grid?.6:.58);p.corner_y=top+height*(kind==Kind::grid?.6:.58);
 if(kind==Kind::checker){p.width=16;p.height=8;p.size_from=SizeFrom::width;}return p;
}
Parameters preset(Kind kind,int index,double width,double height){auto p=defaults(kind,width,height);
 if(index==1){p.size_from=SizeFrom::width;p.width=kind==Kind::grid?32:64;p.border=2;}
 if(index==2){p.size_from=SizeFrom::width_height;p.width=kind==Kind::grid?width/3:32;p.height=kind==Kind::grid?height/3:16;if(kind==Kind::grid){p.anchor_x=p.anchor_y=0;p.border=3;}}
 if(index==3){p.size_from=SizeFrom::width;p.width=kind==Kind::grid?64:32;p.feather_x=p.feather_y=kind==Kind::grid?2:8;p.border=8;}
 return p;
}
bool valid(const Parameters& p){
 auto value=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 return int(p.size_from)>=0&&int(p.size_from)<3&&int(p.blend)>=0&&p.blend<Blend::count&&value(p.anchor_x,-1.e7,1.e7)&&value(p.anchor_y,-1.e7,1.e7)&&value(p.corner_x,-1.e7,1.e7)&&value(p.corner_y,-1.e7,1.e7)&&value(p.width,.01,1.e7)&&value(p.height,.01,1.e7)&&value(p.border,0,1.e7)&&value(p.feather_x,0,1.e7)&&value(p.feather_y,0,1.e7)&&value(p.opacity,0,100)&&value(p.rotation,-360000,360000)&&value(p.color[0],0,1)&&value(p.color[1],0,1)&&value(p.color[2],0,1);
}
double coverage(Kind kind,const Parameters& p,double x,double y){return Shader(kind,p).at(x,y);}
Pixel pixel(Kind kind,const Parameters& p,double x,double y,Pixel source,double footprint,bool locked){return shade(Shader(kind,p),x,y,source,footprint,locked);}
bool render(Kind kind,const Parameters& p,Region region,int width,int height,Image& out,const std::vector<float>* source,const Cancel& cancel,bool locked){
 out={};if(!valid(p)||width<=0||height<=0||std::uint64_t(width)*height>16777216||!std::isfinite(region.x)||!std::isfinite(region.y)||!std::isfinite(region.step)||region.step<=0||(source&&source->size()!=std::size_t(width)*height*4))return false;
 const Shader shader(kind,p);Image image{width,height,int(region.x),int(region.y),std::vector<float>(std::size_t(width)*height*4)};
 for(int y=0;y<height;++y){if(cancel&&cancel())return false;for(int x=0;x<width;++x){const auto i=(std::size_t(y)*width+x)*4;Pixel before{};if(source)for(int k=0;k<4;++k)before[k]=(*source)[i+k];
  const double xx=region.x+x*region.step+(region.step-1)*.5,yy=region.y+y*region.step+(region.step-1)*.5;
  const auto color=shade(shader,xx,yy,before,region.step,locked);for(int k=0;k<4;++k)image.rgba[i+k]=color[k];
 }}out=std::move(image);return true;
}
}
