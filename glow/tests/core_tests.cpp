#include "glow_core.hpp"
#include "psui/async_preview.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
using namespace glow;
void strength_tests();
void check(bool v,const char* m){if(!v){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
Image canvas(int w,int h,float a=0){Image im{w,h,0,0,{}};im.rgba.resize(std::size_t(w)*h*4);for(std::size_t i=3;i<im.rgba.size();i+=4)im.rgba[i]=a;return im;}
double energy(const Image& im,int k){double s=0;for(std::size_t i=0;i<im.rgba.size();i+=4)s+=im.rgba[i+k]*im.rgba[i+3];return s;}
int main(){
 for(auto kind:{Kind::classic,Kind::deep}){
  auto p=defaults(kind);Image out;auto flat=canvas(35,27,1);
  for(std::size_t i=0;i<flat.rgba.size();i+=4){flat.rgba[i]=.7f;flat.rgba[i+1]=.8f;flat.rgba[i+2]=.9f;}
  p.intensity=0;flat.rgba[3]=.2f;check(render(flat,p,out),"identity render");
  for(std::size_t i=0;i<out.rgba.size();++i)check(std::abs(out.rgba[i]-flat.rgba[i])<1e-6,"zero intensity exact identity including partial alpha");
  auto im=canvas(161,161);const auto center=(std::size_t(80)*161+80)*4;
  for(int k=0;k<4;++k)im.rgba[center+k]=1;
  p=defaults(kind);p.threshold=0;p.gamma_amount=0;p.radius=24;p.blend=Blend::add;
  check(render(im,p,out),"transparent impulse");
  check(out.rgba[center+7]>0&&out.rgba[3]<out.rgba[center+7]*.01,"halo grows into transparency and decays with distance");
  for(int d=1;d<55;++d){auto l=center-4*d,r=center+4*d,t=center-4*161*d,b=center+4*161*d;
   check(std::abs(out.rgba[l+3]-out.rgba[r+3])<1e-6&&std::abs(out.rgba[t+3]-out.rgba[b+3])<1e-6,"centered symmetric halo");}
  p.intensity=0;check(render(im,p,out),"no intensity");check(out.rgba==im.rgba,"transparent identity");
  p.intensity=1;p.threshold=200;check(render(im,p,out),"HDR threshold");check(out.rgba==im.rgba,"below threshold identity");
  p.threshold=0;Image mask=canvas(3,2);check(render(im,p,out,&mask),"black alpha mask");check(out.rgba==im.rgba,"mask excludes all glow input");
  p.invert_mask=true;check(render(im,p,out,&mask)&&out.rgba[center+7]>0,"inverted mask allows halo");
  p.invert_mask=false;p.based_on=1;im.rgba[center]=im.rgba[center+1]=im.rgba[center+2]=0;
  check(render(im,p,out)&&out.rgba[center+4]>0,"opaque black artwork can glow from alpha");
  p.based_on=0;for(int k=0;k<3;++k)im.rgba[center+k]=4;
  p.blend=Blend::screen;check(render(im,p,out)&&out.rgba[center]>=4,"HDR screen never dims highlights");
  Image transparent=canvas(19,13);transparent.rgba[0]=99;
  check(render(transparent,p,out)&&energy(out,0)==0,"hidden RGB in transparent pixels emits no light");
  Image previous=out;int polls=0;check(!render(im,p,out,nullptr,nullptr,[&]{return ++polls>2;}),"cancellation interrupts CPU render");check(out.rgba==previous.rgba,"cancelled job cannot publish partial frame");
  p.aspect=std::numeric_limits<double>::quiet_NaN();check(!render(im,p,out),"NaN rejected");
 }
 auto p=defaults(Kind::classic);p.based_on=1;p.dimensions=1;p.radius=24;auto im=canvas(101,101);const auto c=(std::size_t(50)*101+50)*4;for(int k=0;k<4;++k)im.rgba[c+k]=1;Image out;
 check(render(im,p,out)&&out.rgba[c+7]>0&&out.rgba[c+101*4+3]==0,"horizontal blur is one dimensional");
 p=defaults(Kind::deep);p.radius=25;p.aspect=4;p.angle=90;p.gamma_amount=0;
 check(render(im,p,out)&&out.rgba[c+101*4*8+3]>out.rgba[c+4*8+3],"rotated aspect ratio points along vertical axis");
 p.tint=1;p.color_a={1,0,0};p.view=1;check(render(im,p,out)&&energy(out,1)==0&&energy(out,0)>0,"single color glow only");
 p.tint=0;p.chromatic_shift=4;check(render(im,p,out),"chromatic render");
 double rx=0,bx=0,re=0,be=0;for(int y=0;y<101;++y)for(int x=0;x<101;++x){const auto i=(std::size_t(y)*101+x)*4;const double r=out.rgba[i]*out.rgba[i+3],b=out.rgba[i+2]*out.rgba[i+3];rx+=r*x;bx+=b*x;re+=r;be+=b;}
 check(rx/re>bx/be+6,"red and blue pixel offsets separate channels");
 // Async result, replacement and exception handling are independent of the host.
 psui::AsyncPreview<int> job;job.start([](std::stop_token t){while(!t.stop_requested())std::this_thread::yield();return 1;});
 job.start([](std::stop_token){return 42;});int v=0;while(!job.take(v))std::this_thread::yield();check(v==42,"latest result only");
 job.start([](std::stop_token)->int{throw 7;});bool caught=false;while(!caught){try{job.take(v);}catch(int){caught=true;}}check(caught,"worker exceptions propagate on UI thread");
 strength_tests();
 std::cout<<"PASS: both kernels; identity/partial alpha; symmetric transparent halos; HDR; threshold/mask; hidden RGB; cancellation; direction/rotation; tint; chromatic offset; async replacement/errors.\n";
}
