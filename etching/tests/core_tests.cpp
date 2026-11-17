#include "etching_core.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace etching;
int checks=0;void check(bool value,const char* why){++checks;if(!value)throw std::runtime_error(why);}
Image solid(int w,int h,float value,float alpha=1){Image s{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<s.rgba.size();i+=4){for(int k=0;k<3;k++)s.rgba[i+k]=value;s.rgba[i+3]=alpha;}return s;}
double mean(const Image& image){double sum=0;for(std::size_t i=0;i<image.rgba.size();i+=4)sum+=image.rgba[i];return sum/(image.width*image.height);}
bool equal(const Image& a,const Image& b){return a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*sizeof(float));}
int main(int argc,char** argv){try{
 Parameters p;check(valid(p),"valid defaults");Image out;
 for(int mode=0;mode<3;mode++){p={};p.mode=mode;p.frequency=20;p.wave_amp=p.warp_amp=p.edge_strength=0;double last=-1;
  for(float level:{0.f,.1f,.25f,.5f,.75f,.9f,1.f}){auto src=solid(160,160,level);check(render(src,p,out),"constant tone rendered");const auto m=mean(out);check(std::abs(m-level)<.04,"mean density preserves input tone");check(m>=last,"tone monotonic");last=m;for(float value:out.rgba)check(std::isfinite(value)&&value>=0&&value<=1,"bounded RGBA");}
 }
 auto source=solid(193,127,.5f,.7f);for(int y=0;y<source.height;y++)for(int x=0;x<source.width;x++){const auto i=(std::size_t(y)*source.width+x)*4;source.rgba[i]=float(x)/source.width;source.rgba[i+1]=float(y)/source.height;source.rgba[i+2]=float((x+y)%23)/23;}
 for(int i=0;i<6;i++){p=preset(i);check(valid(p)&&render(source,p,out),"preset renders");for(std::size_t n=0;n<out.rgba.size();n+=4){check(out.rgba[n+3]==.7f,"source alpha retained");for(int k=0;k<3;k++)check(std::isfinite(out.rgba[n+k])&&out.rgba[n+k]>=0&&out.rgba[n+k]<=1,"preset color bounded");}}
 p={};p.mix=0;check(render(source,p,out)&&equal(source,out),"zero mix exact identity");p=preset(1);p.background=1;check(render(source,p,out),"transparent ink");for(std::size_t n=3;n<out.rgba.size();n+=4)check(out.rgba[n]>=0&&out.rgba[n]<=.7f,"ink-only alpha bounded by input");
 auto hidden=solid(64,64,.4f);for(int y=0;y<64;y++)for(int x=0;x<32;x++){const auto n=(y*64+x)*4;hidden.rgba[n]=hidden.rgba[n+1]=hidden.rgba[n+2]=4;hidden.rgba[n+3]=0;}auto hidden2=hidden;for(std::size_t n=0;n<hidden2.rgba.size();n+=4)if(!hidden2.rgba[n+3])hidden2.rgba[n]=hidden2.rgba[n+1]=hidden2.rgba[n+2]=0;
 p={};p.smooth_source=4;Image a,b;check(render(hidden,p,a)&&render(hidden2,p,b)&&equal(a,b),"hidden RGB cannot bleed through weighted source/warp smoothing");
 p={};p.mode=1;p.warp_amp=p.edge_strength=0;p.paper_grain=15;auto native=solid(160,120,.43f);native.left=-17;native.top=31;check(render(native,p,a),"native coordinate image");auto crop=solid(32,24,.43f);crop.left=native.left+23;crop.top=native.top+29;check(render(crop,p,b,{}, {double(crop.left),double(crop.top),160,120,1}),"cropped native view");for(int y=0;y<24;y++)for(int x=0;x<32;x++)for(int k=0;k<4;k++)check(std::abs(b.rgba[(y*32+x)*4+k]-a.rgba[((y+29)*160+x+23)*4+k])<1e-6,"document anchored lines/waves/grain");
 p={};p.wave_amp=p.warp_amp=p.edge_strength=0;p.frequency=1000;auto full=solid(160,120,.4f);auto draft=solid(40,30,.4f);check(render(full,p,a)&&render(draft,p,b,{}, {0,0,160,120,4}),"fine native and draft");check(std::abs(mean(a)-.4)<.015&&std::abs(mean(b)-.4)<.015,"subpixel line coverage stable without moire");
 p={};p.frequency=p.edge_strength=0;check(render(full,p,out)&&mean(out)==1,"disabled lines and edges produce paper");
 p={};p.wave_amp=p.warp_amp=0;p.edge_strength=2;p.edge_threshold=.01;p.edge_width=0;check(render(source,p,a),"edge width baseline");p.edge_width=3;check(render(source,p,b)&&mean(b)<mean(a),"edge dilation increases ink");
 auto before=out;p={};p.frequency=-1;check(!valid(p)&&!render(source,p,out)&&equal(before,out),"invalid render atomic");p={};p.gamma=std::numeric_limits<double>::quiet_NaN();check(!render(source,p,out)&&equal(before,out),"nonfinite parameters rejected");p={};std::atomic<int> calls=0;check(!render(source,p,out,[&]{return ++calls>5;})&&equal(before,out),"cancel preserves output");
 std::ofstream raw(argc>1?argv[1]:"etching.f32",std::ios::binary);auto medium=solid(513,257,.5f);for(int y=0;y<257;y++)for(int x=0;x<513;x++){const auto n=(y*513+x)*4;medium.rgba[n]=float(x)/513;medium.rgba[n+1]=float(y)/257;medium.rgba[n+2]=float((x+y)%29)/29;medium.rgba[n+3]=float((x+y)%11)/10;}
 for(int i=0;i<6;i++){p=preset(i);check(render(medium,p,out),"deterministic fixture");raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));}check(raw.good(),"float output saved");
 const auto t=std::chrono::steady_clock::now();auto large=solid(768,768,.5f);check(render(large,Parameters{},out),"large render");const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();
 std::cout<<"PASS: "<<checks<<" assertions; six presets, tonal density, native/draft coordinates, anti-aliasing, opacity, hidden RGB, contours, cancellation and six float fixtures; 768px "<<ms<<"ms\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
