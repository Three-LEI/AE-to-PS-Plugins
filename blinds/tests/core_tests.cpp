#include "blinds_core.hpp"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <limits>
int checks=0;void require(bool v,const char* why){checks++;if(!v)throw std::runtime_error(why);}
int main(){try{
 using namespace blinds;Image src{256,128,-17,31,std::vector<float>(256*128*4)},out,second;
 for(std::size_t i=0;i<src.rgba.size();i+=4){src.rgba[i]=-1.25f;src.rgba[i+1]=2.7f;src.rgba[i+2]=.35f;src.rgba[i+3]=float(i%101)/101;}
 auto p=defaults(256,128,src.left,src.top);require(valid(p)&&p.completion==0&&p.width==20,"native defaults");
 require(render(src,p,out)&&out.rgba==src.rgba,"zero completion exact identity including HDR and hidden RGB");
 for(double feather:{0.,1.,50.,32000.}){p.completion=100;p.feather=feather;require(render(src,p,out),"complete wipe");for(std::size_t i=0;i<out.rgba.size();i++)require(out.rgba[i]==(i%4==3?0:src.rgba[i]),"complete wipe preserves color and clears alpha");}
 p.completion=50;p.feather=0;require(coverage(p.cx+5,p.cy,p)==0&&coverage(p.cx+15,p.cy,p)==1,"zero direction forms vertical strips");
 p.direction=90;require(coverage(p.cx,p.cy+5,p)==0&&coverage(p.cx,p.cy+15,p)==1,"ninety degree horizontal strips");
 for(double direction:{0.,30.,45.,90.,180.,-87.})for(double width:{.25,1.,7.5,20.,97.})for(double feather:{0.,.5,3.,18.,200.}){
  p.direction=direction;p.width=width;p.feather=feather;p.completion=37;
  require(render(src,p,out)&&render(src,p,second)&&out.rgba==second.rgba,"repeatable bounds/angles/subpixel/feather");
  for(std::size_t i=0;i<out.rgba.size();i++){if(i%4==3)require(std::isfinite(out.rgba[i])&&out.rgba[i]>=0&&out.rgba[i]<=src.rgba[i]+1e-6,"alpha only decreases without NaNs");else require(out.rgba[i]==src.rgba[i],"all RGB bits preserved");}
  for(double x:{-21.3,3.,22.125}){const double angle=direction*3.141592653589793/180;double a=coverage(x,27.1,p),b=coverage(x+width*std::cos(angle),27.1+width*std::sin(angle),p);require(std::abs(a-b)<2e-7,"periodicity along normal");}
 }
 p=defaults(256,128,-17,31);p.completion=50;p.direction=27;p.feather=4;require(render(src,p,out),"translation baseline");auto translated=src;translated.left+=73;translated.top-=41;p.cx+=73;p.cy-=41;require(render(translated,p,second)&&out.rgba==second.rgba,"nonzero origins translate with saved center");
 p=defaults(256,128,-17,31);p.completion=70;p.feather=32000;p.width=20;require(render(src,p,out)&&out.valid(),"wide feather has bounded input-edge handling");
 p.feather=3;auto keep=out;int polls=0;require(!render(src,p,out,[&]{return ++polls>3;})&&out.rgba==keep.rgba,"transactional cancellation");
 for(int i=0;i<6;i++){auto q=p;if(i==0)q.width=0;if(i==1)q.feather=-1;if(i==2)q.completion=101;if(i==3)q.direction=std::numeric_limits<double>::infinity();if(i==4)q.cx=std::numeric_limits<double>::quiet_NaN();if(i==5)q.cy=1e8;require(!valid(q)&&!render(src,q,out),"invalid input rejected");}
 std::cout<<"PASS "<<checks<<" blinds core checks: endpoints, RGB/HDR preservation, periodicity, orientation, subpixels, feather, origins and cancellation\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
