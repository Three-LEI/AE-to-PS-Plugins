#include "flare_core.hpp"
#include "flare_design.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace flare {namespace {
constexpr double pi=std::numbers::pi;
double unit(double v){return std::clamp(v,0.,1.);}
double clean(float v){return std::isfinite(v)?double(v):0.;}
double smooth(double a,double b,double v){const auto t=unit((v-a)/(b-a));return t*t*(3-2*t);}
double gaussian(double v){return v>24?0:std::exp(-v);}
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
double random(std::uint32_t seed,int i){return hash(seed+std::uint32_t(i)*0x9e3779b9u)/4294967296.;}
double noise(double x,double y,std::uint32_t seed){
 const int ix=int(std::floor(x)),iy=int(std::floor(y));double tx=x-ix,ty=y-iy;tx=tx*tx*(3-2*tx);ty=ty*ty*(3-2*ty);
 auto at=[&](int a,int b){return hash(std::uint32_t(a)*0x8da6b343u^std::uint32_t(b)*0xd8163841u^seed)/4294967296.;};
 return std::lerp(std::lerp(at(ix,iy),at(ix+1,iy),tx),std::lerp(at(ix,iy+1),at(ix+1,iy+1),tx),ty);
}
Color spectrum(double t){return {.5+.5*std::cos(2*pi*(t-.02)),.5+.5*std::cos(2*pi*(t-.35)),.5+.5*std::cos(2*pi*(t-.68))};}
void add(Color& to,const Color& color,double value){for(int k=0;k<3;k++)to[k]+=color[k]*value;}
Color tint(Color c,const Parameters& p){
 const double y=.2126*c[0]+.7152*c[1]+.0722*c[2];for(auto& v:c)v=std::lerp(y,v,p.saturation*.01);
 // Rotate around the neutral axis, retaining neutral colours and linear HDR.
 const double angle=std::remainder(p.hue,360.)*pi/180,co=std::cos(angle),si=std::sin(angle)/std::sqrt(3.);
 const Color a=c;for(int k=0;k<3;k++){c[k]=std::max(0.,a[k]*co+(a[(k+2)%3]-a[(k+1)%3])*si+(1-co)*(a[0]+a[1]+a[2])/3);c[k]=std::pow(c[k]*p.color[k],1/p.gamma);}
 return c;
}
struct Ghost {double x,y,r,angle,bright;Color color;};
struct PreparedElement {Element element;double x,y,co,si;};
Color design_pixel(const std::vector<PreparedElement>& elements,double x,double y,double base,double aa,double aspect,double atmosphere){
 Color sum{};
 for(const auto& object:elements){const auto& e=object.element;if(!e.enabled||e.brightness==0)continue;
  const double nx=(x-object.x)/(base*e.size),ny=(y-object.y)/(base*e.size*aspect*e.aspect),a=aa/e.size;
  if(e.type==5&&(std::abs(nx)>1+a*2||std::abs(ny)>1+a*2))continue;
  const double r=std::hypot(nx,ny),r2=r*r;
  double value=0;Color color=e.color;
  switch(e.type){
   case 0:{const double width=std::sqrt(.085*.085+a*a*.22);value=2.8*(.085*.085)/(width*width)*gaussian(r2/(width*width));break;}
   case 1:value=.55*gaussian(r2/.32)+.14*gaussian(r2/3.5);break;
   case 2:if(e.count>0&&r<6*e.spread){const double theta=std::atan2(ny,nx)-e.rotation*pi/180,delta=std::remainder(theta,2*pi/e.count),thickness=.025*std::max(.01,e.softness),width=std::sqrt(thickness*thickness+a*a*.22);value=.9*gaussian(r*r*delta*delta/(width*width))*thickness/width*gaussian(r/(.65*e.spread))/(1+r*.6);}break;
   case 3:{const double sx=nx*object.co+ny*object.si,sy=-nx*object.si+ny*object.co,thickness=.028*std::max(.01,e.softness),width=std::sqrt(thickness*thickness+a*a*.2);value=.9*gaussian(std::abs(sx)/(2.1*e.spread)+sy*sy/(width*width))*thickness/width;break;}
   case 4:{const double width=std::max(.016,a*.6),delta=r-1.7;if(std::abs(delta)<width*5){value=.55*gaussian(delta*delta/(width*width))*.016/width;const auto rainbow=spectrum(.2+delta/(width*7));for(int k=0;k<3;k++)color[k]=std::lerp(e.color[k],rainbow[k],e.chromatic*.01);}break;}
   case 5:if(r<=1+a*2){const double theta=std::remainder(std::atan2(ny,nx)-e.rotation*pi/180,2*pi/e.blades),boundary=std::cos(pi/e.blades)/std::cos(theta),soft=std::max(a*.7,.015+std::min(1.,e.softness)*.65),inside=1-smooth(boundary-soft,boundary+std::max(a*.5,.001),r),edge=gaussian(std::pow((r-boundary)/std::max(a*.6,.045),2));value=inside+edge*.7;}break;
   case 6:{const double thickness=.012+e.softness*.06,width=std::sqrt(thickness*thickness+a*a*.36),delta=r-1.7;if(std::abs(delta)<width*5){const double angle=std::abs(std::remainder(std::atan2(ny,nx)-e.rotation*pi/180,2*pi)),half=pi*std::min(1.,e.spread),feather=std::max(.08,a/std::max(.1,r));const double arc=e.spread>=1?1:1-smooth(std::max(0.,half-feather),half,angle);value=.8*arc*gaussian(delta*delta/(width*width))*thickness/width;const auto rainbow=spectrum(.2+delta/(width*7));for(int k=0;k<3;k++)color[k]=std::lerp(e.color[k],rainbow[k],e.chromatic*.01);}break;}
   case 8:if(e.count>0&&r<2.4*e.spread){const double angle=std::atan2(ny,nx)-e.rotation*pi/180,delta=std::remainder(angle,2*pi/e.count),thickness=.018*std::max(.05,e.softness),width=std::sqrt(thickness*thickness+a*a*.24);value=3*gaussian(std::pow(r*delta/width,2))*thickness/width*std::pow(unit(1-r/(2.4*e.spread)),3);}break;
   case 11:{const double sx=nx*object.co+ny*object.si,sy=-nx*object.si+ny*object.co,thickness=.1+e.softness*.25,width=std::sqrt(thickness*thickness+a*a*.3);value=.9*gaussian(sx*sx/(e.spread*e.spread)+sy*sy/(width*width))*thickness/width;const auto rainbow=spectrum(.55+sx/(e.spread*2.5));for(int k=0;k<3;k++)color[k]=std::lerp(e.color[k],rainbow[k],e.chromatic*.01);break;}
  }
  add(sum,color,value*e.brightness*(e.type==5?1:atmosphere));
 }return sum;
}
}
Parameters defaults(int w,int h,int l,int t){Parameters p;p.hx=l+w*.28;p.hy=t+h*.24;p.px=l+w*.5;p.py=t+h*.5;return p;}
Design design_from_legacy(const Parameters& p){
 Design d;d.enabled=true;
 auto add_element=[&](int type,double brightness,double size=1){auto e=make_element(type);e.brightness=brightness;e.size=size;e.color=p.hot_color;d.elements.push_back(e);return &d.elements.back();};
 if(p.hot_bright>0)add_element(0,p.hot_bright,p.hot_size);
 if(p.glow>0)add_element(1,p.glow);
 if(p.ray_bright>0&&p.rays){auto* e=add_element(2,p.ray_bright);e->spread=p.ray_length;e->softness=p.ray_thickness;e->rotation=p.rotation;e->count=p.rays;}
 if(p.streak_bright>0){auto* e=add_element(3,p.streak_bright);e->spread=p.streak_length;e->softness=p.streak_width;e->rotation=p.rotation;}
 if(p.ring_bright>0){auto* e=add_element(4,p.ring_bright,p.ring_size);e->chromatic=p.chromatic;}
 if(p.ghost_bright>0)for(int i=0;i<p.ghosts;i++){
  auto e=make_element(5);e.name_index=i+1;e.axis=(.3+i*.22)*p.ghost_spacing;e.size=(.14+.7*random(p.seed,i*3))*p.ghost_size;e.rotation=random(p.seed,i+500)*360;e.brightness=p.ghost_bright*(.09+.25*random(p.seed,i+99));e.softness=p.ghost_softness;e.blades=p.blades;e.color=spectrum(i*.217+random(p.seed,i)*.13);for(int k=0;k<3;k++)e.color[k]=std::lerp(.8,e.color[k],p.chromatic*.01)*p.ghost_color[k];d.elements.push_back(std::move(e));
 }
 return d;
}
Parameters preset(int i,int w,int h,int l,int t){
 auto p=defaults(w,h,l,t);
 switch(i){
 case 1:p.hot_color={1,.72,.34};p.ghost_color={1,.75,.5};p.rays=20;p.ray_bright=.75;p.rotation=12;p.streak_bright=.08;p.atmosphere=18;p.ghosts=10;break;
 case 2:p.hot_color={1,.86,.58};p.ghost_color={.8,1,.6};p.rays=8;p.ray_bright=.22;p.ghosts=12;p.ghost_size=1.4;p.ghost_softness=.1;p.ghost_bright=.75;p.ring_bright=.65;p.streak_bright=0;p.chromatic=70;break;
 case 3:p.hot_color={.9,.95,1};p.rays=6;p.ray_bright=.15;p.ghosts=5;p.ghost_softness=.6;p.ghost_bright=.32;p.ring_bright=1.2;p.ring_size=1.4;p.chromatic=100;p.streak_bright=.15;break;
 case 4:p.hot_color={.3,.62,1};p.ghost_color={.25,.55,1};p.rays=4;p.ray_bright=.1;p.ghosts=6;p.ghost_bright=.25;p.ring_bright=.12;p.streak_bright=1.5;p.streak_length=2.2;p.streak_width=.5;p.aspect=.65;break;
 case 5:p.hot_color={1,.98,.9};p.rays=0;p.ghosts=3;p.ghost_bright=.16;p.ring_bright=.18;p.streak_bright=0;p.glow=.65;p.hot_size=.7;break;
 case 6:p.hot_color={1,.3,.65};p.ghost_color={.7,.35,1};p.rays=8;p.rotation=22.5;p.ray_bright=.3;p.ghost_bright=.5;p.chromatic=60;p.streak_bright=.75;p.atmosphere=12;break;
 case 7:p.hot_color={.8,.94,1};p.rays=16;p.rotation=11.25;p.ghosts=14;p.blades=3;p.ghost_softness=.06;p.ghost_size=.7;p.ghost_bright=.8;p.chromatic=90;p.ring_bright=.65;p.streak_bright=.2;break;
 default:break;
 }return p;
}
bool valid(const Parameters& p){
 auto range=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};
 for(auto v:{p.hx,p.hy,p.px,p.py})if(!range(v,-1e7,1e7))return false;
 for(auto v:{p.brightness,p.hot_bright,p.glow,p.ray_bright,p.ghost_bright,p.ring_bright,p.streak_bright})if(!range(v,0,20))return false;
 for(auto v:{p.hot_size,p.ray_length,p.ray_thickness,p.ghost_size,p.ghost_spacing,p.ring_size,p.streak_length,p.streak_width})if(!range(v,.01,10))return false;
 for(const auto& c:{p.color,p.hot_color,p.ghost_color})for(auto v:c)if(!range(v,0,1))return false;
 return range(p.scale,1,1000)&&range(p.aspect,.1,10)&&range(p.gamma,.1,5)&&range(p.saturation,0,300)&&range(p.hue,-1e9,1e9)&&range(p.rotation,-1e9,1e9)&&p.rays>=0&&p.rays<=64&&p.ghosts>=0&&p.ghosts<=24&&p.blades>=3&&p.blades<=16&&range(p.ghost_softness,0,1)&&range(p.chromatic,0,100)&&range(p.atmosphere,0,100)&&range(p.noise_scale,1,10000)&&range(p.edge_width,0,100)&&range(p.center_radius,0,100)&&range(p.edge_bright,0,5)&&range(p.center_bright,0,5)&&p.occlusion>=0&&p.occlusion<=2&&range(p.occlusion_softness,0,2000)&&p.blend>=0&&p.blend<=2&&range(p.background,0,100)&&range(p.mix,0,100);
}
double visibility(const Image& mask,const Parameters& p){
 if(!p.occlusion||!mask.valid())return 1;
 auto sample=[&](double x,double y){x-=mask.left;y-=mask.top;if(x<0||y<0||x>=mask.width||y>=mask.height)return p.invert_occlusion?1.:0.;
  const auto i=(std::size_t(int(y))*mask.width+int(x))*4;const double a=unit(clean(mask.rgba[i+3]));double v=p.occlusion==1?a:unit((.2126*clean(mask.rgba[i])+.7152*clean(mask.rgba[i+1])+.0722*clean(mask.rgba[i+2]))*a);return p.invert_occlusion?1-v:v;};
 double blocked=0;const int samples=p.occlusion_softness>0?97:1;
 for(int i=0;i<samples;i++){const double radius=i?std::sqrt((i-.5)/(samples-1))*p.occlusion_softness:0,angle=i*2.399963229728653;blocked+=sample(p.hx+radius*std::cos(angle),p.hy+radius*std::sin(angle));}
 return unit(1-blocked/samples);
}
bool render(const Image& src,const Parameters& p,Image& out,const Image* occluder,const Cancel& cancel,RenderView v,const Design* design){
 if(!src.valid()||!valid(p)||(design&&!valid_design(*design))||(occluder&&!occluder->valid())||(cancel&&cancel()))return false;
 if(p.mix==0){out=src;return true;}
 if(v.width==0&&v.height==0)v={double(src.left),double(src.top),double(src.width),double(src.height),1};
 for(double a:{v.left,v.top,v.width,v.height,v.step})if(!std::isfinite(a))return false;
 if(v.width<=0||v.height<=0||v.step<1||v.step>2e6)return false;
 const double dimension=std::min(v.width,v.height),base=std::max(.01,dimension*.1*p.scale*.01),aa=v.step/base;
 double exposure=p.brightness*visibility(occluder?*occluder:src,p);
 if(exposure==0&&p.background==100&&p.blend!=2){out=src;return true;}
 if(p.edge_width>0){const double distance=std::min({std::abs(p.hx-v.left),std::abs(p.hy-v.top),std::abs(p.hx-v.left-v.width),std::abs(p.hy-v.top-v.height)});exposure*=std::lerp(1.,p.edge_bright,1-smooth(0,dimension*p.edge_width*.01,distance));}
 if(p.center_radius>0)exposure*=std::lerp(1.,p.center_bright,1-smooth(0,dimension*p.center_radius*.01,std::hypot(p.hx-p.px,p.hy-p.py)));
 std::vector<Ghost> ghosts;for(int i=0;i<p.ghosts;i++){
  const double t=(.3+i*.22)*p.ghost_spacing,r=(.14+.7*random(p.seed,i*3))*p.ghost_size;
  auto c=spectrum(i*.217+random(p.seed,i)*.13);for(int k=0;k<3;k++)c[k]=std::lerp(.8,c[k],p.chromatic*.01)*p.ghost_color[k];
  ghosts.push_back({p.hx+(p.px-p.hx)*t,p.hy+(p.py-p.hy)*t,r,random(p.seed,i+500)*2*pi,.09+.25*random(p.seed,i+99),c});
 }
 const double rotation=std::remainder(p.rotation,360.)*pi/180,co=std::cos(rotation),si=std::sin(rotation),apothem=std::cos(pi/p.blades);
 std::vector<PreparedElement> elements;const bool custom=design&&design->enabled;
 auto append=[&](const Element& e,double x,double y){const double angle=std::remainder(e.rotation,360.)*pi/180;elements.push_back({e,x,y,std::cos(angle),std::sin(angle)});};
 if(custom)for(const auto& e:design->elements){if(!e.enabled||e.brightness==0)continue;const double cx=p.hx+(p.px-p.hx)*e.axis+dimension*e.x*.01,cy=p.hy+(p.py-p.hy)*e.axis+dimension*e.y*.01;
  if(e.type==7||e.type==9||e.type==10){for(int i=0;i<e.count;i++){auto part=e;double x=cx,y=cy;
   if(e.type==7){part.type=5;const double offset=(i-(e.count-1)*.5)*.22*e.spread;part.size*=.3+.7*random(p.seed+37,i);part.rotation+=random(p.seed+91,i)*90;part.brightness*=.35+.65*random(p.seed+3,i);x+=(p.px-p.hx)*offset;y+=(p.py-p.hy)*offset;auto rainbow=spectrum(i*.217);for(int k=0;k<3;k++)part.color[k]*=std::lerp(1.,rainbow[k],e.chromatic*.01);}
   else if(e.type==9){part.type=8;part.count=4;const double distance=std::sqrt(random(p.seed+17,i))*e.spread*base*e.size,angle=random(p.seed+83,i)*2*pi+e.rotation*pi/180;x+=std::cos(angle)*distance;y+=std::sin(angle)*distance*p.aspect*e.aspect;part.size*=.1+.18*random(p.seed+71,i);part.spread=1;part.brightness*=.3+.7*random(p.seed+29,i);}
   else{part.type=6;part.size*=1+i*.14*e.spread;part.spread=1;part.brightness/=1+i*.65;}
   append(part,x,y);
  }}else append(e,cx,cy);
 }
 Image next{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};
 const bool ok=pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height*(custom?std::max(std::size_t(1),elements.size()):std::size_t(std::max(1,p.ghosts))),cancel,[&](std::size_t first,std::size_t end,const auto& stop){
  for(int y=int(first);y<int(end);y++){
   if(stop())return false;
   for(int x=0;x<src.width;x++){
    const double dx=v.left+(x+.5)*v.step-p.hx,dy=(v.top+(y+.5)*v.step-p.hy)/p.aspect;
    const double nx=dx/base,ny=dy/base,r=std::hypot(nx,ny),r2=r*r;
    Color emission{};
    if(exposure>0){
     if(custom){
      double atmosphere=1;if(p.atmosphere>0){const double n=.65*noise((dx+p.hx)/p.noise_scale,(dy+p.hy)/p.noise_scale,p.seed)+.35*noise((dx+p.hx)*2.7/p.noise_scale,(dy+p.hy)*2.7/p.noise_scale,p.seed+1);atmosphere=std::lerp(1.,.2+1.6*n,p.atmosphere*.01);}
      emission=design_pixel(elements,v.left+(x+.5)*v.step,v.top+(y+.5)*v.step,base,aa,p.aspect,atmosphere);
     }else{
     const double core=p.hot_size*.085,core_width=std::sqrt(core*core+aa*aa*.22),core_energy=(core*core)/(core_width*core_width);
     const double hot=2.8*p.hot_bright*core_energy*gaussian(r2/(core_width*core_width));
     const double glow=p.glow*(.55*gaussian(r2/.32)+.14*gaussian(r2/3.5));
     add(emission,p.hot_color,hot+glow);
     if(p.rays&&p.ray_bright>0&&r<6*p.ray_length){
      const double angle=std::atan2(ny,nx)-rotation,delta=std::remainder(angle,2*pi/p.rays);
      const double native_width=.025*p.ray_thickness,width=std::sqrt(native_width*native_width+aa*aa*.22);
      const double radial=gaussian(r/(.65*p.ray_length));
      const double profile=gaussian(r*r*delta*delta/(width*width))*native_width/width;
      add(emission,p.hot_color,p.ray_bright*.9*profile*radial/(1+r*.6));
     }
     if(p.streak_bright>0){const double sx=(nx*co+ny*si),sy=(-nx*si+ny*co),width=std::sqrt(std::pow(.028*p.streak_width,2)+aa*aa*.2);
      const double streak=gaussian(std::abs(sx)/(2.1*p.streak_length)+sy*sy/(width*width))*(.028*p.streak_width/width);
      add(emission,p.hot_color,streak*p.streak_bright*.9);
     }
     if(p.ring_bright>0){const double rr=1.7*p.ring_size,ww=std::max(.016*p.ring_size,aa*.6),delta=r-rr;
      if(std::abs(delta)<ww*5){const double spectrum_t=.2+delta/(ww*7),value=gaussian(delta*delta/(ww*ww));auto c=spectrum(spectrum_t);for(int k=0;k<3;k++)c[k]=std::lerp(p.hot_color[k],c[k],p.chromatic*.01);add(emission,c,p.ring_bright*.55*value*(.016*p.ring_size/ww));}
     }
     // Atmospheric variation affects emitter elements, leaving lens reflections clear.
     if(p.atmosphere>0){const double n=.65*noise((dx+p.hx)/p.noise_scale,(dy+p.hy)/p.noise_scale,p.seed)+.35*noise((dx+p.hx)*2.7/p.noise_scale,(dy+p.hy)*2.7/p.noise_scale,p.seed+1);for(auto& z:emission)z*=std::lerp(1.,.2+1.6*n,p.atmosphere*.01);}
     if(p.ghost_bright>0)for(const auto& g:ghosts){
      const double gx=(v.left+(x+.5)*v.step-g.x)/base,gy=(v.top+(y+.5)*v.step-g.y)/(base*p.aspect),distance=std::hypot(gx,gy);
      if(distance>g.r+aa*2)continue;
      const double theta=std::remainder(std::atan2(gy,gx)-g.angle,2*pi/p.blades),boundary=g.r*apothem/std::cos(theta);
      const double softness=std::max(aa*.7,g.r*(.015+p.ghost_softness*.65)),inside=1-smooth(boundary-softness,boundary+std::max(aa*.5,.001),distance);
      const double edge=gaussian(std::pow((distance-boundary)/std::max(aa*.6,g.r*.045),2));
      add(emission,g.color,p.ghost_bright*g.bright*(inside+edge*.7));
     }
     }
     emission=tint(emission,p);for(auto& z:emission)z*=exposure;
    }
    const auto i=(std::size_t(y)*src.width+x)*4;const double a=unit(clean(src.rgba[i+3])),m=p.mix*.01;
    const double power=std::max({emission[0],emission[1],emission[2]}),fa=unit(power);
    const double oa=p.blend==2?fa:p.preserve_alpha?a:a+fa*(1-a);
    const double final_alpha=std::lerp(a,oa,m);
    for(int k=0;k<3;k++){
     const double original=clean(src.rgba[i+k]),b=original*p.background*.01;
     double premul;
     if(p.blend==2)premul=emission[k];
     else if(p.preserve_alpha){const double e=p.blend==0?unit(emission[k]):emission[k];premul=(p.blend==0?b+e*(1-unit(b)):b+e)*a;}
     else{const double bg=b*a,e=p.blend==0?unit(emission[k]):emission[k];premul=p.blend==0?bg+e*(1-unit(bg)):bg+e;}
     const double mixed=std::lerp(original*a,premul,m);next.rgba[i+k]=final_alpha>0?float(mixed/final_alpha):0;
    }
    next.rgba[i+3]=float(final_alpha);
   }
  }return true;
 });
 if(!ok||(cancel&&cancel()))return false;out=std::move(next);return true;
}
}
#include "design_codec.inc"
