#include "dither_core.hpp"
#include "dither_area.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace dither;
int checks=0;void check(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
Image solid(int w,int h,float v,float a=1){Image s{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<s.rgba.size();i+=4){s.rgba[i]=s.rgba[i+1]=s.rgba[i+2]=v;s.rgba[i+3]=a;}return s;}
bool same(const Image& a,const Image& b){return a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*sizeof(float));}
double mean(const Image& a){double v=0;for(std::size_t i=0;i<a.rgba.size();i+=4)v+=a.rgba[i];return v/(a.width*a.height);}
bool in_palette(const Color& c,const std::vector<Color>& palette){for(auto a:palette){bool ok=true;for(int k=0;k<3;k++)ok&=float(a[k])==float(c[k]);if(ok)return true;}return false;}
int main(int argc,char** argv){try{
 Parameters p;Image out,first;auto input=solid(128,128,.37f);p.pixel_size=1;
 for(int pattern=0;pattern<9;pattern++){p.pattern=pattern;check(render(input,p,out),"ordered pattern");const int n=pattern<3?2<<pattern:8;check(std::abs(mean(out)-.37)<.5/(n*n)+1e-6,"pattern threshold distribution within matrix quantization step");for(std::size_t i=0;i<out.rgba.size();i+=4)check((out.rgba[i]==0||out.rgba[i]==1)&&out.rgba[i+3]==1,"true 1-bit output");}
 p={};p.pixel_size=1;p.pattern=2;check(render(input,p,first),"native Bayer");auto small=solid(32,32,.37f);auto q=p;q.pixel_size=.25;check(render(small,q,out),"analytic minification");for(int y=0;y<32;y++)for(int x=0;x<32;x++){auto a=preview::sample_area(first,x*4,y*4,x*4+4,y*4+4);check(std::abs(a[0]-out.rgba[(y*32+x)*4])<1e-6,"coarse pattern matches native area average");}
 for(int a=1;a<4;a++){p.algorithm=a;check(render(input,p,out),"algorithm");if(a==1)check(std::abs(mean(out)-.37)<.025,"Floyd mean tone");}
 p.algorithm=3;check(render(input,p,first),"nearest reference");for(int a=0;a<3;a++){p.algorithm=a;p.amount=0;check(render(input,p,out)&&same(out,first),"zero dither is nearest palette");}
 for(int pal=0;pal<6;pal++){p={};p.palette=pal;p.pixel_size=3;auto a=solid(67,49,.4f);for(std::size_t i=0;i<a.rgba.size();i+=4){a.rgba[i]=float((i*7)%101)/100;a.rgba[i+1]=float((i*11)%97)/96;a.rgba[i+2]=float((i*3)%89)/88;}auto colors=palette_colors(p);for(int alg=0;alg<4;alg++){p.algorithm=alg;check(render(a,p,out),"palette/algorithm pair");for(std::size_t i=0;i<out.rgba.size();i+=4)check(in_palette({out.rgba[i],out.rgba[i+1],out.rgba[i+2]},colors),"only requested palette colours");}}
 input=solid(65,47,.4f,.6f);p={};check(render(input,p,out),"alpha source");for(std::size_t i=3;i<out.rgba.size();i+=4)check(out.rgba[i]==input.rgba[i],"source alpha preserved");p.mix=0;input.rgba[0]=4.7f;check(render(input,p,out)&&same(input,out),"zero mix HDR identity");
 p={};check(render(input,p,first),"grid base");input.left=-19;input.top=37;p.shift_x=-19;p.shift_y=37;check(render(input,p,out)&&same(first,out),"coordinate anchoring");
 auto hidden=solid(64,32,1);for(int y=0;y<32;y++)for(int x=0;x<32;x++){auto i=(y*64+x)*4;hidden.rgba[i]=hidden.rgba[i+1]=hidden.rgba[i+2]=0;hidden.rgba[i+3]=0;}p={};p.pixel_size=64;check(render(hidden,p,out),"alpha-weighted cell mean");for(int y=0;y<32;y++)for(int x=32;x<64;x++)check(out.rgba[(y*64+x)*4]==1,"transparent black does not contaminate white");
 auto checker=solid(8,8,0);for(int y=0;y<8;y++)for(int x=0;x<8;x++)for(int k=0;k<3;k++)checker.rgba[(y*8+x)*4+k]=float((x+y)&1);auto averaged=preview::sample_area(checker,0,0,8,8);check(averaged[0]==.5f&&averaged[3]==1,"Fit view integrates checkerboard");
 for(auto size:{std::pair{1,1},std::pair{1,17},std::pair{31,1},std::pair{33,19}}){auto a=solid(size.first,size.second,.4f);p={};p.pixel_size=5.7;p.shift_x=-1.3;check(render(a,p,out)&&out.valid(),"tiny fractional grids");}
 p={};p.colors.fill({.3,.3,.3});p.palette=5;check(render(input,p,out),"duplicate palette colours");p={};p.pixel_size=0;check(!valid(p),"invalid block");p={};p.palette=2;p.levels=14;check(!valid(p),"RGB colour limit");
 p={};Image sentinel=solid(2,2,.7);out=sentinel;int polls=0;auto medium=solid(513,257,.4f);check(!render(medium,p,out,[&]{return ++polls>4;})&&same(out,sentinel),"cancel does not publish partial image");
 std::ofstream raw(argc>1?argv[1]:"dither-deterministic.f32",std::ios::binary);for(int i=0;i<6;i++){p=preset(i);if(i==2)p.algorithm=2;if(i==4)p.linear_light=true;for(std::size_t n=0;n<medium.rgba.size();n+=4){medium.rgba[n]=float((n*7)%129)/128;medium.rgba[n+1]=float((n*13)%113)/112;medium.rgba[n+2]=float((n*3)%97)/96;medium.rgba[n+3]=float((n*11)%31)/30;}check(render(medium,p,out),"deterministic preset");raw.write(reinterpret_cast<const char*>(out.rgba.data()),std::streamsize(out.rgba.size()*sizeof(float)));}check(raw.good(),"output fixtures");
 std::cout<<"PASS: "<<checks<<" assertions; nine threshold maps, palette membership, ordered/FS/Atkinson, tone, area-correct previews, alpha, coordinates, fractional/tiny grids and cancellation.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
