#include "caustics_core.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace caustics;
int checks=0;void check(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
Image solid(int w,int h,float v=.2f,float alpha=1){Image s{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<s.rgba.size();i+=4){s.rgba[i]=s.rgba[i+1]=s.rgba[i+2]=v;s.rgba[i+3]=alpha;}return s;}
bool equal(const Image& a,const Image& b){return a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*sizeof(float));}
double difference(const Image& a,const Image& b){double value=0;for(std::size_t i=0;i<a.rgba.size();i+=4)value+=std::abs(a.rgba[i]-b.rgba[i]);return value/(a.width*a.height);}
int main(int argc,char** argv){try{
 Parameters p;Image out;check(valid(p),"valid defaults");auto src=solid(193,127,.2f,.7f);
 for(int i=0;i<6;i++){p=preset(i);check(valid(p)&&render(src,p,out),"all presets render");for(std::size_t n=0;n<out.rgba.size();n+=4){check(std::isfinite(out.rgba[n])&&out.rgba[n]>=0,"finite light");check(out.rgba[n+3]>=0&&out.rgba[n+3]<=.7f,"alpha bounded");if(p.output!=2)check(out.rgba[n+3]==.7f,"source alpha retained");}}
 p={};p.mix=0;check(render(src,p,out)&&equal(src,out),"zero mix exact identity");
 p={};p.height=0;p.threshold=0;p.output=1;p.light={1,1,1};p.dark={0,0,0};check(render(src,p,out),"flat plane");const double expected=1-std::exp(-1.2);for(std::size_t i=0;i<out.rgba.size();i+=4)check(std::abs(out.rgba[i]-expected)<1e-6,"flat ray energy conserved including all borders");
 for(int flat=0;flat<3;flat++){p={};p.output=1;p.threshold=0;if(flat==0)p.focus=0;if(flat==1)p.depth=0;if(flat==2)p.refractive_index=1;check(render(src,p,out),"zero optics control");for(std::size_t i=4;i<out.rgba.size();i+=4)check(out.rgba[i]==out.rgba[0],"flat optical field spatially constant");}
 for(int y=0;y<src.height;y++)for(int x=0;x<src.width;x++){auto n=(y*src.width+x)*4;src.rgba[n]=float(x)/src.width;src.rgba[n+1]=float(y)/src.height;src.rgba[n+2]=float((x+y)%31)/31;}
 p={};p.intensity=0;p.refraction=0;check(render(src,p,out)&&difference(src,out)<1e-7,"zero light and refraction leave source");auto clean=out;p.refraction=100;p.focus=0;check(render(src,p,out)&&difference(clean,out)>.005,"refraction operates independently of ray focus/intensity");auto warped=out;
 p.refraction=0;p.focus=1.5;p.intensity=100;check(render(src,p,out)&&difference(clean,out)>.001,"light independent of refraction");p.refractive_index=1;p.refraction=100;p.intensity=0;check(render(src,p,out)&&difference(src,out)<1e-7,"index one disables refraction");
 for(int mode=0;mode<4;mode++)for(int blend=0;blend<4;blend++){p={};p.output=mode;p.blend=blend;p.refraction=mode==0?100:0;check(render(src,p,out),"all output/blend combinations");check(out.rgba.size()==src.rgba.size(),"output dimensions retained");}
 Image map=solid(193,127,.5f);p={};p.surface=2;p.threshold=0;p.output=1;check(render(src,p,out,&map),"external flat map");double maxdiff=0;for(std::size_t i=0;i<out.rgba.size();i+=4)maxdiff=std::max(maxdiff,std::abs(double(out.rgba[i])-out.rgba[0]));check(maxdiff<.025,"flat map has no artificial grid texture");
 for(int y=0;y<map.height;y++)for(int x=0;x<map.width;x++){auto n=(y*map.width+x)*4;map.rgba[n]=map.rgba[n+1]=map.rgba[n+2]=float(.5+.4*std::sin(x*.06)*std::cos(y*.08));}
 Image a,b;p.threshold=1;check(render(src,p,a,&map),"external wave map");p.channel=1;check(render(src,p,b,&map)&&difference(a,b)<1e-6,"grayscale map luma equals red");p.surface=1;p.channel=0;check(render(map,p,a),"current layer height source");p.surface=2;check(render(map,p,b,&map)&&equal(a,b),"current/reference same height pixels match");
 auto hidden=map;for(std::size_t n=0;n<map.rgba.size();n+=4){map.rgba[n+3]=hidden.rgba[n+3]=0;hidden.rgba[n]=hidden.rgba[n+1]=hidden.rgba[n+2]=100;}
 check(render(src,p,a,&map)&&render(src,p,b,&hidden)&&equal(a,b),"hidden reference RGB never drives wave height");
 auto transparent=src;for(std::size_t i=3;i<src.rgba.size();i+=4)transparent.rgba[i]=0;p={};p.output=2;check(render(transparent,p,a),"transparent output alpha");for(float v:a.rgba)check(v==0,"no light outside retained source alpha");p.preserve_alpha=false;check(render(transparent,p,b)&&difference(a,b)>.01,"standalone light can fill transparent canvas");
 auto hdr=solid(32,24,2);p={};p.blend=1;check(render(hdr,p,a),"HDR light");for(std::size_t n=0;n<a.rgba.size();n+=4)check(a.rgba[n]>=2,"HDR unclipped");
 auto big=solid(384,256,.2f);p={};p.output=1;p.scale=130;p.softness=3;check(render(big,p,a),"full geometry");auto small=solid(192,128,.2f);check(render(small,p,b,nullptr,{}, {0,0,384,256,2}),"reduced native geometry");double mae=0;for(int y=0;y<128;y++)for(int x=0;x<192;x++){double value=0;for(int j=0;j<2;j++)for(int i=0;i<2;i++)value+=a.rgba[((y*2+j)*384+x*2+i)*4]*.25;mae+=std::abs(value-b.rgba[(y*192+x)*4]);}mae/=192*128;check(mae<.06,"draft caustic field stays close to area-filtered native render");
 p={};p.output=3;big.left=-17;big.top=31;check(render(big,p,a),"offset native geometry");auto crop=solid(32,24);crop.left=6;crop.top=60;check(render(crop,p,b),"crop anchored geometry");for(int y=0;y<24;y++)for(int x=0;x<32;x++)check(std::abs(a.rgba[((y+29)*384+x+23)*4]-b.rgba[(y*32+x)*4])<.0001,"surface anchored to document pixels");
 auto previous=out;p={};p.scale=0;check(!render(src,p,out)&&equal(out,previous),"invalid render atomic");p={};p.depth=std::numeric_limits<double>::quiet_NaN();check(!valid(p)&&!render(src,p,out)&&equal(out,previous),"NaN rejected");p={};p.surface=2;check(!render(src,p,out)&&equal(out,previous),"missing reference rejected");p={};std::atomic<int> calls=0;check(!render(src,p,out,nullptr,[&]{return ++calls>7;})&&equal(out,previous),"cancel preserves published output");
 std::ofstream raw(argc>1?argv[1]:"caustics.f32",std::ios::binary);for(int i=0;i<6;i++){p=preset(i);p.seed=345+i*17;check(render(src,p,out),"deterministic fixture");raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));}check(raw.good(),"float fixture saved");
 const auto t=std::chrono::steady_clock::now();auto large=solid(768,768);check(render(large,Parameters{},out),"768px render");const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();
 std::cout<<"PASS: "<<checks<<" assertions; ray energy, independent refraction, six presets, four outputs/blends, native/draft geometry (MAE="<<mae<<"), reference alpha, HDR, cancellation; 768px "<<ms<<"ms\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
