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
 require(b.rgba[c+1]>.60f,"exposure one brightens colored highlights, not only distant haze");
 p.intensity=0;require(glow::render(orange,p,b)&&b.rgba==orange.rgba,"zero exposure preserves source after gamma fix");
 std::cout<<"PASS: Deep gamma/alpha black-background equivalence; Unmult association; thin-stroke near halo at radius 1000; wide falloff; colored highlight; zero exposure. near100="<<small<<" near1000="<<large<<" far1000="<<far<<'\n';
}
