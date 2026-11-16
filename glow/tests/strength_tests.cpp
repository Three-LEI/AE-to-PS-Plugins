#include "glow_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
namespace {
void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
glow::Image stroke(bool opaque){
 glow::Image im{513,257,0,0,{}};im.rgba.resize(513*257*4);
 for(int y=0;y<257;++y)for(int x=0;x<513;++x){const auto i=(std::size_t(y)*513+x)*4;const bool lit=x>=128&&x<=384&&y>=126&&y<=130;
  im.rgba[i]=im.rgba[i+1]=im.rgba[i+2]=lit?1:0;im.rgba[i+3]=lit||opaque?1:0;
 }
 return im;
}
double premul(const glow::Image& im,int x,int y,int k=0){const auto i=(std::size_t(y)*im.width+x)*4;return im.rgba[i+k]*im.rgba[i+3];}
}
void strength_tests(){
 const auto transparent=stroke(false),black=stroke(true);glow::Image a,b;
 auto p=glow::defaults(glow::Kind::deep);p.radius=1000;
 for(auto mode:{glow::Blend::add,glow::Blend::screen}){
  p.blend=mode;require(glow::render(transparent,p,a)&&glow::render(black,p,b),"transparent and opaque glow render");
  for(std::size_t i=0;i<a.rgba.size();i+=4)for(int k=0;k<3;++k)
   require(std::abs(a.rgba[i+k]*a.rgba[i+3]-b.rgba[i+k])<2e-5,"gamma-correct light is consistent on transparency and black");
 }
 p.view=1;p.unmult=true;require(glow::render(transparent,p,a),"straight glow-only render");
 p.unmult=false;require(glow::render(transparent,p,b),"associated glow-only render");
 for(std::size_t i=0;i<a.rgba.size();i+=4){require(a.rgba[i+3]==b.rgba[i+3],"Unmult preserves alpha");for(int k=0;k<3;++k)
  require(std::abs(a.rgba[i+k]*a.rgba[i+3]-b.rgba[i+k])<2e-5,"Unmult converts association exactly once in encoded space");}
 p.unmult=true;p.radius=100;require(glow::render(transparent,p,a),"radius 100");
 p.radius=1000;require(glow::render(transparent,p,b),"radius 1000");
 const double small=premul(a,256,135),large=premul(b,256,135),far=premul(b,256,200);
 require(large>.2&&large>small*.3,"large radius retains visible near-field around a thin stroke");
 require(far>.015&&far<large,"wide halo remains visible and decays with distance");
 auto orange=stroke(false);for(std::size_t i=0;i<orange.rgba.size();i+=4){orange.rgba[i+1]*=.35f;orange.rgba[i+2]*=.02f;}
 p.view=0;p.blend=glow::Blend::add;require(glow::render(orange,p,b),"colored highlight render");
 const auto c=(std::size_t(128)*513+256)*4;
 require(b.rgba[c+1]>.35f&&b.rgba[c+1]<1.f,"exposure one brightens colored highlights while preserving their chroma");
 p.intensity=0;require(glow::render(orange,p,b)&&b.rgba==orange.rgba,"zero exposure preserves source after gamma fix");
 // Public AE Deep Glow 2 samples, radius 100/exposure 1/default Screen.
 // The independent multi-scale kernel is approximate; keep a small display
 // tolerance while catching the previous clipped midtone (.5 became >1).
 const double grays[]={.1,.25,.5,.75,.9};
 const double ae[]={.29423832893372,.640380859375,.9404296875,.9986572265625,1.};
 for(int falloff:{0,1})for(double radius:{0.,25.,100.,1000.}){
  double previous=-1;
  for(int sample=0;sample<5;sample++){
   const double gray=grays[sample];auto q=glow::defaults(glow::Kind::deep);q.radius=radius;q.falloff=falloff;
   glow::Image flat{31,23,0,0,std::vector<float>(31*23*4)};
   for(std::size_t i=0;i<flat.rgba.size();i+=4){for(int k=0;k<3;k++)flat.rgba[i+k]=float(gray);flat.rgba[i+3]=1;}
   require(glow::render(flat,q,b),"uniform diffuse surface render");
   require(b.rgba[0]>previous&&b.rgba[0]<=1.000001,"screen pyramid preserves ordered diffuse tones without HDR clipping");previous=b.rgba[0];
   if(falloff==0)require(std::abs(b.rgba[0]-ae[sample])<.025,"default diffuse tones remain close to measured AE Deep Glow 2");
  }
 }
 auto hdr=stroke(true);for(std::size_t i=0;i<hdr.rgba.size();i+=4)for(int k=0;k<3;k++)hdr.rgba[i+k]=2;
 p=glow::defaults(glow::Kind::deep);require(glow::render(hdr,p,b)&&b.rgba[0]>2.9f,"HDR emission remains above one; no clipping-based correction");
 std::cout<<"PASS: Deep gamma/alpha black-background equivalence; Unmult association; thin-stroke near halo at radius 1000; wide falloff; colored highlight; zero exposure; AE2 diffuse contrast at 4 radii and 2 falloffs; HDR. near100="<<small<<" near1000="<<large<<" far1000="<<far<<'\n';
}
