#include "creative_core.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
#include <set>
using namespace creative;
std::size_t checks=0;void check(bool v,const char* why){++checks;if(!v){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
Image fixture(int w,int h){Image a{w,h,-13,7,std::vector<float>(std::size_t(w)*h*4)};for(int y=0;y<h;y++)for(int x=0;x<w;x++){const auto i=(std::size_t(y)*w+x)*4;a.rgba[i]=float(x)/std::max(1,w-1)*2-.25f;a.rgba[i+1]=float(y)/std::max(1,h-1);a.rgba[i+2]=float(.5+.4*std::sin(x*.17)*std::cos(y*.11));a.rgba[i+3]=(x>3&&y>3&&x<w-4&&y<h-4)?float(.3+.7*std::abs(std::sin(x*.08))):0;}return a;}
bool near(double a,double b,double e=1e-8){return std::abs(a-b)<e;}
int main(int argc,char** argv){
 auto src=fixture(192,128);Image out,second;for(int kind=0;kind<3;kind++){
  auto p=defaults(Kind(kind),src.width,src.height,src.left,src.top);check(valid(p),"valid defaults");check(p.cx==83&&p.cy==(kind==1?39:71),"document-relative center");
  p.mix=0;check(render(src,p,out)&&out.rgba==src.rgba,"zero mix exact identity including hidden RGB/HDR");p.mix=100;
  if(kind==0){p.amount=0;check(render(src,p,out)&&out.rgba==src.rgba,"zero amount exact identity");p.amount=20;}
  if(kind==1){p.intensity=p.edge_intensity=0;check(render(src,p,out)&&out.rgba==src.rgba,"neutral sweep exact identity");p.intensity=25;}
  const auto saved=out;check(!render(src,p,out,[]{return true;})&&out.rgba==saved.rgba,"cancel leaves output untouched");p.size=0;check(!valid(p)&&!render(src,p,out),"invalid size rejected");p.size=20;p.cx=std::numeric_limits<double>::infinity();check(!valid(p),"infinite center rejected");
 }
 auto p=defaults(Kind::turbulent,src.width,src.height,src.left,src.top);p.pinning=0;p.amount=25;
 for(int type=0;type<9;type++){p.type=type;check(render(src,p,out),"all turbulence types render");for(float v:out.rgba)check(std::isfinite(v),"finite turbulent HDR");for(std::size_t i=3;i<out.rgba.size();i+=4)check(out.rgba[i]>=0&&out.rgba[i]<=1,"turbulent alpha range");}
 p.type=6;for(int x=0;x<30;x++){auto q=warp(src.left+x,src.top+50,p,src);check(q[0]==src.left+x,"vertical type does not move x");}
 p.type=7;for(int y=0;y<30;y++){auto q=warp(src.left+50,src.top+y,p,src);check(q[1]==src.top+y,"horizontal type does not move y");}
 p.type=0;p.pinning=8;for(int x=0;x<src.width;x++)for(int y:{0,src.height-1}){auto q=warp(src.left+x,src.top+y,p,src);check(q[0]==src.left+x&&q[1]==src.top+y,"locked top/bottom exact");}
 for(int y=0;y<src.height;y++)for(int x:{0,src.width-1}){auto q=warp(src.left+x,src.top+y,p,src);check(q[0]==src.left+x&&q[1]==src.top+y,"locked left/right exact");}
 p.pinning=1;check(warp(src.left,src.top+50,p,src)[0]==src.left,"pin keeps normal component");
 p.pinning=0;p.cycle=true;p.evolution=0;p.cycles=2;check(render(src,p,out),"cycle begin");p.evolution=720;check(render(src,p,second)&&out.rgba==second.rgba,"cycle full period byte exact");p.evolution=180;check(render(src,p,second)&&out.rgba!=second.rgba,"evolution changes form");p.cycle=false;p.evolution=0;p.seed=19;check(render(src,p,second)&&out.rgba!=second.rgba,"seed changes form");check(render(src,p,out)&&out.rgba==second.rgba,"seed deterministic");
 auto alias=src;check(render(alias,p,alias)&&alias.rgba==out.rgba,"exact in-place support");
 Image flat{64,48,0,0,std::vector<float>(64*48*4,1)};for(std::size_t i=0;i<flat.rgba.size();i+=4)flat.rgba[i]=flat.rgba[i+1]=flat.rgba[i+2]=.25f;
 p=defaults(Kind::sweep,64,48);p.direction=0;p.color={1,1,1};p.edge_intensity=0;p.width=8;p.intensity=100;p.shape=2;check(render(flat,p,out),"analytic sweep render");check(near(out.rgba[(12*64+32)*4],1.25,1e-6),"sweep center HDR energy");check(near(out.rgba[(12*64+40)*4],.5,1e-6),"sharp quadratic width response");check(near(out.rgba[(12*64+48)*4],.25,1e-6),"sweep support radius twice width");p.reception=1;check(render(flat,p,out)&&near(out.rgba[(12*64+32)*4],1,1e-6),"composite alpha-over color");p.reception=2;check(render(flat,p,out)&&near(out.rgba[(12*64+40)*4+3],.25,1e-6),"cutout mask profile");
 p=defaults(Kind::sweep,src.width,src.height,src.left,src.top);p.cx=1e6;check(render(src,p,out)&&out.rgba==src.rgba,"beam outside image suppresses both body and edge light");
 p=defaults(Kind::kaleida,src.width,src.height,src.left,src.top);std::set<std::vector<float>> variants;for(int type=0;type<9;type++){p.mirroring=type;check(render(src,p,out),"all kaleida modes render");variants.insert(out.rgba);for(float v:out.rgba)check(std::isfinite(v),"finite kaleida output");}check(variants.size()==9,"nine distinct mirror modes");
 p.mirroring=0;const double period=src.width*p.size*.01;auto a=warp(p.cx+7,p.cy+11,p,src),b=warp(p.cx+7+period,p.cy+11,p,src);check(near(a[0],b[0])&&near(a[1],b[1]),"unfold analytic tile period");
 auto one=fixture(1,1);one.rgba={2,.4f,.1f,1};for(int edge=1;edge<4;edge++){p=defaults(Kind::kaleida,1,1);p.edge=edge;check(render(one,p,out)&&near(out.rgba[3],1),"one-pixel boundary modes");}
 const auto owner=std::this_thread::get_id();p=defaults(Kind::turbulent,192,128);int calls=0;check(!render(src,p,out,[&]{check(std::this_thread::get_id()==owner,"cancel stays on invoking thread");return ++calls>3;}),"mid-render cancellation");
 std::filesystem::path folder=argc>1?argv[1]:"evidence/creative-v01/core";std::filesystem::create_directories(folder);std::ofstream raw(folder/"deterministic.f32",std::ios::binary);
 for(int kind=0;kind<3;kind++)for(int n=0;n<8;n++){auto q=defaults(Kind(kind),192,128,-13,7);q.seed=n;q.type=n%9;q.mirroring=n%9;q.shape=n%3;q.reception=n%3;q.quality=n%3;q.pinning=n%15;q.rotation=n*17;q.complexity=1+(n%3)*.5;check(render(src,q,out),"serial/parallel cases");raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*4);}raw.close();
 auto large=fixture(1024,768);p=defaults(Kind::turbulent,1024,768,-13,7);auto before=std::chrono::steady_clock::now();check(render(large,p,out),"large render");const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count();
 std::cout<<"PASS: "<<checks<<" assertions; analytic sweep/tile/axis/pinning; neutral/HDR/alpha; nine mirror modes; deterministic seed/cycle; alias/guards/cancel; 24 serial/parallel cases.\nBENCH: 1024x768 Turbulent ms="<<ms<<"\n";
}
