#include "cell_core.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <filesystem>
#include <fstream>
#include <algorithm>
using namespace cell;
int checks=0;
void check(bool v,const char* msg){++checks;if(!v){std::cerr<<"FAIL: "<<msg<<'\n';std::exit(1);}}
bool same(const Image& a,const Image& b,double eps=0){if(a.rgba.size()!=b.rgba.size())return false;for(std::size_t i=0;i<a.rgba.size();++i)if(std::abs(a.rgba[i]-b.rgba[i])>eps)return false;return true;}
Image run(Parameters p,Region r={-44,-39,1},int w=99,int h=83,Source s={}){Image im;check(render(p,r,w,h,im,s),"render succeeds");return im;}
int main(int argc,char** argv){
 auto p=defaults(1920,1080);check(p.contrast==100&&p.size==60&&p.offset_x==960&&p.offset_y==540,"AE verified defaults");p.offset_x=p.offset_y=0;
 Image colour{96,80,0,0,std::vector<float>(96*80*4)};
 for(int y=0;y<80;++y)for(int x=0;x<96;++x){const auto i=(std::size_t(y)*96+x)*4;colour.rgba[i]=x/95.f;colour.rgba[i+1]=y/79.f;colour.rgba[i+2]=.37f;colour.rgba[i+3]=(x<48?.25f:1.f);}
 const Source source{&colour,0,0,1,1};
 for(int m=0;m<int(Mode::count);++m){p.mode=Mode(m);const auto a=run(p,{0,0,1},96,80,source);check(same(a,run(p,{0,0,1},96,80,source)),"deterministic independent render");
  for(float f:a.rgba)check(std::isfinite(f)&&f>=0&&f<=1,"finite normalized output");
  // Full output vs arbitrary tile boundaries, including source sampling across them.
  const auto tile=run(p,{31,19,1},43,47,source);bool equal=true;
  for(int y=0;y<47;++y)for(int x=0;x<43;++x)for(int k=0;k<4;++k)equal&=tile.rgba[(std::size_t(y)*43+x)*4+k]==a.rgba[(std::size_t(y+19)*96+x+31)*4+k];check(equal,"output is independent of host tile boundaries");
  p.invert=true;const auto inverse=run(p,{0,0,1},96,80,source);bool inverted=true;for(std::size_t i=0;i<a.rgba.size();i+=4)for(int k=0;k<3;++k)inverted&=std::abs(a.rgba[i+k]+inverse.rgba[i+k]-1)<1.e-5;check(inverted,"invert flips RGB and leaves geometry");p.invert=false;
  p.tiling=true;p.cells_x=3;p.cells_y=2;p.evolution=43;
  if(!samples_source(p.mode)){const auto b=run(p),c=run(p,{-44+p.size*3,-39+p.size*2,1});check(same(b,c,2.e-6),"toroidal seamless spatial period");}
  p.cycle_evolution=true;p.cycle=2;const auto b=run(p,{0,0,1},96,80,source);p.evolution+=720;check(same(b,run(p,{0,0,1},96,80,source),1.e-6),"configured evolution cycle repeats");
  p.cycle_evolution=false;p.tiling=false;p.evolution=0;
 }
 p.mode=Mode::bubbles;const auto zero=run(p);p.evolution=360;check(!same(zero,run(p)),"ordinary full revolution must not repeat");p.evolution=0;p.seed=17;check(!same(zero,run(p)),"random seed changes layout");p.seed=0;
 // A distance-to-fixed-segments tube is Lipschitz continuous. The old
 // per-pixel nearest-pair graph broke this bound at Voronoi transitions.
 for(int seed:{0,7})for(double disperse:{1.,1.5}){
  auto tube=defaults(512,320);tube.mode=Mode::tubular;tube.seed=seed;tube.disperse=disperse;tube.size=30;tube.contrast=200;
  const auto image=run(tube,{-211,-17,.25},1600,32);
  const double bound=2*std::sqrt(1.06*tube.contrast/100)*.25/tube.size+1.e-5;
  for(int y=0;y<image.height;++y)for(int x=1;x<image.width;++x){
   const auto i=(std::size_t(y)*image.width+x)*4;
   check(std::abs(image.rgba[i]-image.rgba[i-4])<=bound,"tubular has no per-pixel graph tears");
  }
 }
 p.mode=Mode::static_plates;p.disperse=0;const auto st=run(p);p.evolution=120;check(same(st,run(p)),"static plates retain lightness with fixed positions");
 p.mode=Mode::plates;p.evolution=0;const auto pl=run(p);p.evolution=120;check(!same(pl,run(p)),"plates lightness evolves with fixed positions");
 p=defaults(96,80);p.mode=Mode::crystallize;Image out;check(!render(p,{},8,8,out),"crystallize cannot invent source pixels");
 Image grey=colour;for(std::size_t i=0;i<grey.rgba.size();i+=4){grey.rgba[i]=grey.rgba[i+1]=grey.rgba[i+2]=.25f;grey.rgba[i+3]=1;}
 const auto flat=run(p,{0,0,1},96,80,{&grey});bool uniform=true;for(std::size_t i=0;i<flat.rgba.size();i+=4)uniform&=std::abs(flat.rgba[i]-.25)<1.e-6&&flat.rgba[i+3]==1;check(uniform,"uniform source remains uniform in crystallize");
 for(std::size_t i=3;i<grey.rgba.size();i+=4)grey.rgba[i]=0;const auto transparent=run(p,{0,0,1},96,80,{&grey});bool empty=true;for(float v:transparent.rgba)empty&=v==0;check(empty,"transparent source remains empty and has no hidden RGB fringes");
 p.mode=Mode::bubbles;check(render(p,{0,0,1},96,80,out),"blank-layer procedural generation");for(std::size_t i=3;i<out.rgba.size();i+=4)check(out.rgba[i]==1,"procedural mode creates alpha");
 check(render(p,{0,0,1},96,80,out,source,{},true),"protected alpha render");bool alpha=true;for(std::size_t i=3;i<out.rgba.size();i+=4)alpha&=std::abs(out.rgba[i]-colour.rgba[i])<1.e-6;check(alpha,"protected alpha retained");
 check(!render(p,{},96,80,out,{},[]{return true;})&&!out.valid(),"cancel has no partial published output");
 p.size=std::numeric_limits<double>::quiet_NaN();check(!render(p,{},8,8,out),"reject nonfinite parameters");p.size=60;check(!render(p,{},100000,100000,out),"reject unreasonable allocation");
 if(argc>1){std::filesystem::create_directories(argv[1]);for(int m=0;m<int(Mode::count);++m)for(int c:{100,200}){p=defaults(512,320);p.mode=Mode(m);p.contrast=c;const auto im=run(p,{0,0,1},512,320,{&colour,0,0,512./96,320./80});const auto file=std::filesystem::path(argv[1])/(std::to_string(m)+"-c"+std::to_string(c)+".ppm");std::ofstream f(file,std::ios::binary);f<<"P6\n512 320\n255\n";for(std::size_t i=0;i<im.rgba.size();i+=4)for(int k=0;k<3;++k)f.put(char(std::lround(std::clamp(im.rgba[i+k],0.f,1.f)*255)));}}
 std::cout<<"PASS: "<<checks<<" assertions; 12 modes, default/seed/evolution/cycles/tiling; source sampling; irregular tiles; alpha; cancellation and input guards.\n";
}
