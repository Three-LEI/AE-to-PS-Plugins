#include "filmdamage_core.hpp"
#include "../../native/common/grain_noise.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace filmdamage {namespace {
constexpr double pi=std::numbers::pi;
double unit(double x){return std::clamp(x,0.,1.);}
double clean(float x){return std::isfinite(x)?x:0.;}
double smooth(double a,double b,double x){const auto t=unit((x-a)/std::max(1e-8,b-a));return t*t*(3-2*t);}
using psgrain::grain_noise;
using psgrain::hash;
using psgrain::noise;
using psgrain::random;
bool stopped(const Cancel& c){return c&&c();}
template<class Read,class Write>void box_line(int length,int r,double fraction,Read at,Write write){
 double sum=0;
 for(int j=-r;j<=r;j++)sum+=at(j);
 const double divisor=2*r+1+2*fraction;
 for(int i=0;i<length;i++){
  const double outer=fraction>0?fraction*(double(at(i-r-1))+at(i+r+1)):0;
  write(i,float((sum+outer)/divisor));
  sum+=at(i+r+1)-at(i-r);
 }
}
bool box(std::vector<float>& v,int w,int h,int r,const Cancel& cancel,double fraction=0){
 if(!r&&fraction==0)return true;
 std::vector<float> tmp(v.size());
 if(!pscpu::parallel_ranges(h,v.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){for(auto y=lo;y<hi;y++){if(stop())return false;auto at=[&](int x){return v[y*w+std::clamp(x,0,w-1)];};box_line(w,r,fraction,at,[&](int x,float value){tmp[y*w+x]=value;});}return true;}))return false;
 return pscpu::parallel_ranges(w,v.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){for(auto x=lo;x<hi;x++){if(stop())return false;auto at=[&](int y){return tmp[std::size_t(std::clamp(y,0,h-1))*w+x];};box_line(h,r,fraction,at,[&](int y,float value){v[std::size_t(y)*w+x]=value;});}return true;});
}
bool blur(Image& image,double radius,const Cancel& cancel){
 if(radius<=0)return true;
 int r=std::max(1,int(std::ceil(radius*.5)));const double strength=unit(radius*radius/(r*(r+1.)));const auto n=image.rgba.size()/4;std::vector<float> alpha(n),weight;
 for(std::size_t i=0;i<n;i++)alpha[i]=float(unit(clean(image.rgba[i*4+3])));weight=alpha;
 for(int pass=0;pass<3;pass++)if(!box(weight,image.width,image.height,r,cancel))return false;
 for(int k=0;k<3;k++){std::vector<float> plane(n);for(std::size_t i=0;i<n;i++)plane[i]=float(clean(image.rgba[i*4+k])*alpha[i]);for(int pass=0;pass<3;pass++)if(!box(plane,image.width,image.height,r,cancel))return false;for(std::size_t i=0;i<n;i++)image.rgba[i*4+k]=weight[i]>1e-12f?float(std::lerp(alpha[i]>0?clean(image.rgba[i*4+k]):0.,double(plane[i]/weight[i]),strength)):0;}
 return true;
}
bool make_halation(const Image& image,double radius,double threshold,std::vector<float>& halo,const Cancel& cancel){
 const int w=image.width,h=image.height;
 const auto count=std::size_t(w)*h;
 std::vector<float> highlights(count);
 if(!pscpu::parallel_ranges(h,count,cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){
  for(auto y=lo;y<hi;y++){
   if(stop())return false;
   for(int x=0;x<w;x++){
    const auto i=y*w+x;
    const auto* pixel=image.rgba.data()+i*4;
    const double light=.2126*clean(pixel[0])+.7152*clean(pixel[1])+.0722*clean(pixel[2]);
    highlights[i]=float(smooth(threshold,threshold+.18,light)*unit(clean(pixel[3])));
   }
  }
  return true;
 }))return false;
 halo=highlights;
 const double box_radius=(std::sqrt(1+4*radius*radius)-1)*.5;
 const int r=int(box_radius);
 const double fraction=box_radius-r;
 for(int pass=0;pass<3;pass++)if(!box(halo,w,h,r,cancel,fraction))return false;
 return pscpu::parallel_ranges(h,count,cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){
  for(auto y=lo;y<hi;y++){
   if(stop())return false;
   for(int x=0;x<w;x++){
    const auto i=y*w+x;
    halo[i]=std::max(0.f,halo[i]-highlights[i]);
   }
  }
  return true;
 });
}
bool resolve_edge(int& x,int& y,int w,int h,int edge){
 if(edge==2&&(x<0||x>=w||y<0||y>=h))return false;
 if(edge==1){x=(x%w+w)%w;y=(y%h+h)%h;}
 else{x=std::clamp(x,0,w-1);y=std::clamp(y,0,h-1);}
 return true;
}
std::array<double,4> sample(const Image& im,double x,double y,int edge){
 std::array<double,4> out{};const auto ix=int(std::floor(x)),iy=int(std::floor(y));const double fx=x-ix,fy=y-iy;
 for(int j=0;j<2;j++)for(int i=0;i<2;i++){int xx=ix+i,yy=iy+j;
  if(!resolve_edge(xx,yy,im.width,im.height,edge))continue;
  const auto n=(std::size_t(yy)*im.width+xx)*4;const double w=(i?fx:1-fx)*(j?fy:1-fy),a=unit(clean(im.rgba[n+3]));out[3]+=a*w;for(int k=0;k<3;k++)out[k]+=clean(im.rgba[n+k])*a*w;
 }
 if(out[3]>1e-12){for(int k=0;k<3;k++)out[k]/=out[3];}
 return out;
}
double sample_halo(const std::vector<float>& halo,int w,int h,double x,double y,int edge){
 const int ix=int(std::floor(x)),iy=int(std::floor(y));
 const double fx=x-ix,fy=y-iy;
 double value=0;
 for(int j=0;j<2;j++)for(int i=0;i<2;i++){
  int xx=ix+i,yy=iy+j;
  if(resolve_edge(xx,yy,w,h,edge))value+=halo[std::size_t(yy)*w+xx]*(i?fx:1-fx)*(j?fy:1-fy);
 }
 return value;
}
// Three coverage planes: print damage, negative damage and colored stains.
struct Marks {
 int w,h;RenderView v;std::vector<std::array<float,3>> pixels;
 Marks(int width,int height,RenderView view):w(width),h(height),v(view),pixels(std::size_t(w)*h){}
 void put(int x,int y,int k,double a){auto& old=pixels[std::size_t(y)*w+x][k];old=float(1-(1-old)*(1-unit(a)));}
 template<class F>bool area(double l,double t,double r,double b,int k,F coverage,const Cancel& cancel={}){
  const int x0=std::max(0,int(std::floor((l-v.left)/v.step))),y0=std::max(0,int(std::floor((t-v.top)/v.step)));
  const int x1=std::min(w,int(std::ceil((r-v.left)/v.step))),y1=std::min(h,int(std::ceil((b-v.top)/v.step)));
  for(int y=y0;y<y1;y++){
   if(stopped(cancel))return false;
   for(int x=x0;x<x1;x++)put(x,y,k,coverage(v.left+(x+.5)*v.step,v.top+(y+.5)*v.step));
  }
  return true;
 }
 void line(double ax,double ay,double bx,double by,double width,double opacity,int k){
  const double margin=width*.5+v.step,dx=bx-ax,dy=by-ay,length=dx*dx+dy*dy;
  area(std::min(ax,bx)-margin,std::min(ay,by)-margin,std::max(ax,bx)+margin,std::max(ay,by)+margin,k,[&](double x,double y){const auto t=length>0?unit(((x-ax)*dx+(y-ay)*dy)/length):0;const auto d=std::hypot(x-ax-t*dx,y-ay-t*dy);return opacity*unit((width*.5+v.step*.5-d)/v.step)*std::min(1.,width/v.step);});
 }
};
bool make_marks(Marks& marks,const Parameters& p,std::uint32_t seed,const Cancel& cancel){
 const auto v=marks.v;auto yy=[&](double y){return p.flip?v.top+v.height-(y-v.top):y;};
 const auto location_seed=seed^hash(std::uint32_t(p.damage_variation));
 for(int type=0;type<2;type++)for(int i=0;i<(type?p.stains:p.dust);i++){
  if(stopped(cancel))return false;const int id=(type?30000:10000)+i*19;const double x=v.left+random(location_seed,id)*v.width,y=yy(v.top+random(location_seed,id+1)*v.height);
  const double size=(type?p.stain_size:p.dust_size)*(.35+1.3*random(seed,id+2)),rx=std::max(.1,size*.5),ry=rx*(.4+random(seed,id+3)*1.5),aa=std::max(.01,v.step/std::min(rx,ry));
  const int k=type?2:(random(seed,id+4)*100<p.dust_white?1:0);const double opacity=(type?p.stain_opacity:p.dust_opacity)*.01*(.45+.55*random(seed,id+5));
  const double texture=type?p.stain_texture*.01:0;
  const double roughness=type?p.stain_edge_roughness*.01:0;
  if(texture==0&&roughness==0){
   marks.area(x-rx*1.5-v.step,y-ry*1.5-v.step,x+rx*1.5+v.step,y+ry*1.5+v.step,k,[&](double px,double py){const auto u=(px-x)/rx,z=(py-y)/ry;const double edge=1+(type?.23:.12)*noise(u*3,z*3,seed+id);return opacity*(1-smooth(edge-aa,edge+aa,std::hypot(u,z)));});
   continue;
  }
  if(opacity==0)continue;
  const double coarse_weight=1-smooth(.35,1.25,aa*3.6),fine_weight=1-smooth(.35,1.25,aa*9);
  const double rim_width=std::max(.075,aa),rim_weight=std::min(1.,.075/rim_width);
  const auto texture_seed=hash(seed+id+113u),edge_seed=hash(seed+id+231u);
  // Shrink both spacing and depth for fine chips, without changing the base
  // stain shape. Drop subpixel octaves using the native/draft pixel footprint.
  const double scale=p.stain_edge_size*.01,frequency=1/scale;
  const double depth=roughness*std::min(1.,scale);
  const double low_frequency=5.5*frequency,mid_frequency=12*frequency,high_frequency=24*frequency;
  const double low=depth*.25*(1-smooth(.35,1.25,aa*low_frequency));
  const double mid=depth*.15*(1-smooth(.35,1.25,aa*mid_frequency));
  const double high=depth*.075*(1-smooth(.35,1.25,aa*high_frequency));
  const double extent=roughness>0?std::max(1.5,1.23+low+mid+high+aa):1.5;
  if(!marks.area(x-rx*extent-v.step,y-ry*extent-v.step,x+rx*extent+v.step,y+ry*extent+v.step,k,[&](double px,double py){
   const double u=(px-x)/rx,z=(py-y)/ry;
   double edge=1+.23*noise(u*3,z*3,seed+id);
   if(low)edge+=low*noise(u*low_frequency,z*low_frequency,edge_seed);
   if(mid)edge+=mid*noise(u*mid_frequency,z*mid_frequency,edge_seed+31);
   if(high)edge+=high*noise(u*high_frequency,z*high_frequency,edge_seed+77);
   const double radial=std::hypot(u,z),distance=edge-radial;
   const double coverage=1-smooth(edge-aa,edge+aa,radial);
   if(coverage==0||texture==0)return opacity*coverage;
   const double coarse=coarse_weight?noise(u*3.6,z*3.6,texture_seed)*coarse_weight:0.;
   const double fine=fine_weight?noise(u*9,z*9,texture_seed+71)*fine_weight:0.;
   const double rim=(1-smooth(.025,.025+rim_width,std::abs(distance)))*rim_weight*smooth(-.25,.55,coarse);
   const double pockets=smooth(.15,.6,-.4*coarse-.6*fine);
   const double density=.93+.72*coarse+.28*fine;
   return opacity*coverage*std::lerp(1.,std::clamp(density+texture*(1.05*rim-.85*pockets),.04,1.9),texture);
  },cancel))return false;
 }
 for(int i=0;i<p.hairs;i++){
  if(stopped(cancel))return false;const int id=50000+i*23;const double x=v.left+random(location_seed,id)*v.width,y=v.top+random(location_seed,id+1)*v.height,angle=random(seed,id+2)*2*pi,length=p.hair_length*(.4+random(seed,id+3));
  const double curl=p.hair_curl*.01*length*(random(seed,id+4)>.5?1:-1),co=std::cos(angle),si=std::sin(angle);double ax=x,ay=yy(y);
  for(int j=1;j<=24;j++){const double t=j/24.,side=std::sin(t*pi)*curl,bx=x+co*t*length-si*side,by=yy(y+si*t*length+co*side);marks.line(ax,ay,bx,by,p.hair_width*(.55+.45*std::sin(t*pi)),p.hair_opacity*.01,0);ax=bx;ay=by;}
 }
 const double a=p.scratch_angle*pi/180,co=std::cos(a),si=std::sin(a);
 for(int i=0;i<p.scratches;i++){
  if(stopped(cancel))return false;const int id=70000+i*29;const double x=v.left+v.width*(.5+p.scratch_center*.005+(random(location_seed,id)-.5)*p.scratch_spread*.01),y=v.top+random(location_seed,id+1)*v.height;
  const double length=v.height*p.scratch_length*.01*(.4+.8*random(seed,id+2)),width=p.scratch_width*(.45+1.1*random(seed,id+3));const int k=random(seed,id+4)*100<p.scratch_white?1:0;
  auto point=[&](double t){double q=(t-.5)*length;double rough=p.scratch_roughness*noise(0,t*37,seed+id);return std::array<double,2>{x-si*q+co*rough,yy(y+co*q+si*rough)};};auto prev=point(0);
  const int segments=std::clamp(int(length/6)+1,16,512);
  for(int j=1;j<=segments;j++){const auto t=double(j)/segments;auto next=point(t);const double gaps=smooth(p.scratch_gaps*.01-.08,p.scratch_gaps*.01+.08,(noise(t*41,0,seed+id+7)+1)*.5),taper=smooth(0,.05,t)*(1-smooth(.95,1,t));marks.line(prev[0],prev[1],next[0],next[1],width,p.scratch_opacity*.01*gaps*taper,k);prev=next;}
 }return true;
}
}
bool valid(const Parameters& p){
 auto range=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};
 for(double x:{p.grain,p.grain_color,p.grain_darks,p.grain_lights,p.fade,p.dust_opacity,p.dust_white,p.hair_opacity,p.scratch_opacity,p.scratch_white,p.scratch_gaps,p.stain_opacity,p.vignette,p.vignette_softness,p.leak,p.mix,p.grain_softness,p.halation,p.halation_threshold,p.stain_texture,p.stain_edge_roughness})if(!range(x,0,100))return false;
 if(!range(p.halation_radius,.5,160)||!range(p.stain_edge_size,5,250)||p.damage_variation<0||p.damage_variation>1000000)return false;
 for(double x:{p.hair_length,p.stain_size})if(!range(x,.1,2000))return false;
 for(double x:{p.hair_width,p.scratch_width})if(!range(x,.1,64))return false;
 if(!range(p.dust_size,.1,200))return false;
 for(double x:{p.hair_curl,p.scratch_roughness,p.scratch_length,p.scratch_spread,p.shake,p.defocus,p.flicker})if(!range(x,0,200))return false;
 for(double x:{p.scratch_angle,p.leak_angle})if(!range(x,-1e9,1e9))return false;
 for(double x:p.tint)if(!range(x,0,2))return false;
 for(double x:p.stain_color)if(!range(x,0,1))return false;
 return range(p.grain_size,.25,40)&&range(p.saturation,0,300)&&range(p.contrast,0,300)&&range(p.exposure,-10,10)&&range(p.scratch_center,-200,200)&&range(p.tilt,0,30)&&range(p.vignette_radius,1,200)&&p.dust>=0&&p.dust<=2000&&p.hairs>=0&&p.hairs<=100&&p.scratches>=0&&p.scratches<=200&&p.stains>=0&&p.stains<=100&&p.variation>=0&&p.variation<=1000000&&p.edge>=0&&p.edge<=2;
}
Parameters preset(int i){Parameters p;switch(i){
 case 1:p.grain=4;p.dust=12;p.scratches=2;p.hairs=0;p.stains=0;p.saturation=100;p.fade=1;p.vignette=6;p.tint={1,1,1};break;
 case 2:p.grain=18;p.grain_mono=true;p.saturation=0;p.dust=110;p.scratches=12;p.hairs=4;p.stains=5;p.fade=8;p.contrast=115;p.vignette=32;p.tint={1,1,1};break;
 case 3:p.saturation=25;p.tint={1,.78,.48};p.fade=12;p.grain=13;p.stains=8;p.stain_size=150;p.stain_opacity=38;p.scratches=10;p.vignette=24;break;
 case 4:p.dust=220;p.dust_size=2.8;p.dust_white=35;p.scratches=16;p.scratch_white=45;p.hairs=5;p.stains=6;p.grain=18;p.fade=6;p.defocus=.8;p.vignette=22;break;
 case 5:p.grain=9;p.leak=70;p.leak_angle=160;p.tint={1,.96,.88};p.exposure=.15;p.dust=20;p.hairs=0;p.stains=1;p.scratches=3;p.fade=5;break;
 case 6:p.grain=0;p.dust=0;p.hairs=0;p.stains=0;p.scratches=24;p.scratch_width=.5;p.scratch_opacity=85;p.scratch_white=35;p.scratch_gaps=45;p.saturation=100;p.fade=p.vignette=0;p.tint={1,1,1};break;
 case 7:p.dust=180;p.stains=12;p.stain_size=120;p.stain_opacity=45;p.hairs=7;p.scratches=20;p.grain=24;p.grain_size=1.5;p.saturation=65;p.fade=10;p.contrast=110;p.defocus=1.5;p.shake=2;p.tilt=.2;p.flicker=20;p.vignette=35;break;
 default:break;}return p;}
bool render(const Image& src,const Parameters& p,Image& out,const Cancel& cancel,RenderView view){
 pscpu::ParallelScope parallel;if(!src.valid()||!valid(p)||stopped(cancel))return false;if(p.mix==0){out=src;return true;}
 if(view.width==0&&view.height==0)view={double(src.left),double(src.top),double(src.width),double(src.height),1};
 for(double v:{view.left,view.top,view.width,view.height,view.step})if(!std::isfinite(v))return false;
 if(view.width<=0||view.height<=0||view.step<1||view.step>2e6)return false;
 const auto seed=p.seed^hash(std::uint32_t(p.variation));Marks marks(src.width,src.height,view);if(!make_marks(marks,p,seed,cancel))return false;
 Image blurred;if(p.defocus>0){blurred=src;if(!blur(blurred,p.defocus/view.step,cancel))return false;}const auto& input=blurred.valid()?blurred:src;
 std::vector<float> halo;
 const double halo_gain=p.halation*.01,grain_softness=p.grain_softness*.01;
 if(halo_gain>0&&!make_halation(input,p.halation_radius/view.step,p.halation_threshold*.01,halo,cancel))return false;
 constexpr std::array<double,3> halo_tint{.85,.18,.045};
 Image result{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};
 const double shift_x=(random(seed,90001)*2-1)*p.shake/view.step,shift_y=(random(seed,90002)*2-1)*p.shake/view.step;
 const double angle=(random(seed,90003)*2-1)*p.tilt*pi/180,co=std::cos(angle),si=std::sin(angle),gain=std::exp2(p.exposure+(random(seed,90004)*2-1)*p.flicker*.01);
 const double la=p.leak_angle*pi/180,lx=std::cos(la),ly=std::sin(la),mix=p.mix*.01;
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){for(auto y=lo;y<hi;y++){if(stop())return false;for(int x=0;x<src.width;x++){
  const auto n=(std::size_t(y)*src.width+x)*4;std::array<double,4> color;
  double glow=0;
  if(p.shake||p.tilt){
   const double dx=x-(src.width-1)*.5,dy=double(y)-(src.height-1)*.5;
   const double sx=co*dx+si*dy+(src.width-1)*.5-shift_x,sy=-si*dx+co*dy+(src.height-1)*.5-shift_y;
   color=sample(input,sx,sy,p.edge);
   if(!halo.empty())glow=sample_halo(halo,src.width,src.height,sx,sy,p.edge)*halo_gain;
  }else{
   for(int k=0;k<4;k++)color[k]=clean(input.rgba[n+k]);
   if(!halo.empty())glow=halo[n/4]*halo_gain;
  }
  if(p.preserve_alpha)color[3]=unit(clean(src.rgba[n+3]));else color[3]=unit(color[3]);
  const double wx=view.left+(x+.5)*view.step,wy=view.top+(double(y)+.5)*view.step;
  const double u=(wx-view.left)/view.width-.5,v=(wy-view.top)/view.height-.5,luma=.2126*color[0]+.7152*color[1]+.0722*color[2];
  const double radius=std::hypot(u*2,v*2),edge=p.vignette_radius*.01,soft=std::max(.01,p.vignette_softness*.01);
  const double vig=1-p.vignette*.01*smooth(std::max(0.,edge-soft),edge+soft,radius);
  const double leak=p.leak*.01*std::pow(unit(.5+(u*lx+v*ly)*.9),4)*(1+.25*noise(wx/90,wy/90,seed+81));
  const double gx=wx/p.grain_size,gy=wy/p.grain_size,attenuation=std::min(1.,p.grain_size/view.step);
  const double mono=p.grain?grain_noise(gx,gy,seed+101,grain_softness):0;
  const auto mark=marks.pixels[std::size_t(y)*src.width+x];
  for(int k=0;k<3;k++){
   double value=std::max(0.,((luma+(color[k]-luma)*p.saturation*.01)*gain-.18)*p.contrast*.01+.18);
   value=std::lerp(value,.14,p.fade*.01*(1-unit(value)))*p.tint[k]*vig;
   const double lum=unit(value),amplitude=std::lerp(p.grain_darks*.01,1.,smooth(0,.5,lum))*(1-smooth(.5,1,lum))+p.grain_lights*.01*smooth(.5,1,lum);
   const double chroma=p.grain_mono||!p.grain?mono:std::lerp(mono,grain_noise(gx,gy,seed+131+k*13,grain_softness),p.grain_color*.01);
   value=std::max(0.,value+chroma*p.grain*.02*amplitude*attenuation);
   if(glow>0)value+=glow*halo_tint[k];
   value=std::lerp(value,p.stain_color[k],mark[2]);value*=1-mark[0];value=std::lerp(value,1.,mark[1]);value+=leak*(k==0?1.2:k==1?.24:.025);
   color[k]=value;
  }
  const double original_alpha=unit(clean(src.rgba[n+3])),alpha=std::lerp(original_alpha,color[3],mix);result.rgba[n+3]=float(alpha);
  for(int k=0;k<3;k++)result.rgba[n+k]=alpha>1e-12?float((clean(src.rgba[n+k])*original_alpha*(1-mix)+color[k]*color[3]*mix)/alpha):0;
 }}return true;}))return false;
 if(stopped(cancel))return false;out=std::move(result);return true;
}
}
