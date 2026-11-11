#include "fractal_core.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>
#include <set>
#include <cmath>
#include <cstring>
std::uint64_t hash(const std::vector<std::uint8_t>& v){std::uint64_t h=14695981039346656037ULL;for(auto x:v)h=(h^x)*1099511628211ULL;return h;}
void check(bool b,const char* text){if(!b){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
int main(){
 constexpr int w=137,h=83,c=4,row=w*c+9;
 std::vector<std::uint8_t> src(row*h),out(row*h,219),repeat(row*h,219),tiled(row*h,219);
 for(std::size_t i=0;i<src.size();++i)src[i]=static_cast<std::uint8_t>(i*73);
 fractal::Parameters p;
 auto run=[&](fractal::Parameters q,std::vector<std::uint8_t>& target){return fractal::process(src.data(),src.size(),row,target.data(),target.size(),row,w,h,c,q,{-33,19,1});};
 check(run(p,out)&&run(p,repeat)&&out==repeat,"deterministic output");
 for(int y=0;y<h;y+=17)for(int x=0;x<w;x+=29){const auto tw=std::min(29,w-x),th=std::min(17,h-y),start=y*row+x*c;check(fractal::process(src.data()+start,src.size()-start,row,tiled.data()+start,tiled.size()-start,row,tw,th,c,p,{-33.0+x,19.0+y,1}),"tile render");}
 check(tiled==out,"arbitrary tiling and negative origins are seamless");
 for(int y=0;y<h;++y){for(int x=0;x<w;++x)check(out[y*row+x*c+3]==src[y*row+x*c+3],"alpha exact");for(int i=w*c;i<row;++i)check(out[y*row+i]==219,"row padding untouched");}
 p.opacity=0;check(run(p,repeat),"zero opacity");for(int y=0;y<h;++y)check(std::equal(src.begin()+y*row,src.begin()+y*row+w*c,repeat.begin()+y*row),"zero is exact source");
 p={};p.contrast=0;check(run(p,repeat),"zero contrast");check(repeat[0]==128&&repeat[1]==128&&repeat[2]==128,"zero contrast is middle gray");
 p={};p.seed=91;check(run(p,repeat)&&repeat!=out,"seed changes pattern");
 p={};p.evolution=1;check(run(p,repeat)&&repeat!=out,"evolution changes pattern continuously");
 p={};p.complexity=3.5;check(run(p,repeat),"fractional complexity");
 p={};p.sub_influence=0;check(run(p,repeat),"zero sub influence");p.complexity=1;check(run(p,tiled)&&repeat==tiled,"zero influence equals single octave");
 p={};auto in_place=src;check(fractal::process(in_place.data(),in_place.size(),row,in_place.data(),in_place.size(),row,w,h,c,p,{-33,19,1}),"in place");for(int y=0;y<h;++y)check(std::equal(out.begin()+y*row,out.begin()+y*row+w*c,in_place.begin()+y*row),"in place equals disjoint");
 p.scale=0;check(!run(p,repeat),"zero scale rejected");p={};p.evolution=std::numeric_limits<double>::quiet_NaN();check(!run(p,repeat),"NaN rejected");p={};check(!fractal::process(src.data(),src.size(),row,src.data()+1,src.size()-1,row,w,h,c,p),"partial overlap rejected");
 std::set<std::uint64_t> patterns;
 for(int type=0;type<static_cast<int>(fractal::FractalType::count);++type){p={};p.fractal=static_cast<fractal::FractalType>(type);p.evolution=73;p.sub_rotation=17;p.sub_offset_x=31;check(run(p,repeat),"all fractal models render");patterns.insert(hash(repeat));}
 check(patterns.size()==static_cast<int>(fractal::FractalType::count),"fractal models are distinct");
 patterns.clear();for(int i=0;i<4;++i){p={};p.noise=static_cast<fractal::NoiseType>(i);check(run(p,repeat),"all interpolation models render");patterns.insert(hash(repeat));}check(patterns.size()==4,"four distinct interpolations");
 for(int i=0;i<static_cast<int>(fractal::FractalType::count);++i){p={};p.fractal=static_cast<fractal::FractalType>(i);p.cycle_evolution=true;p.cycles=2;p.evolution=-71.5;check(run(p,repeat),"negative cyclic evolution");p.evolution+=720;check(run(p,tiled)&&repeat==tiled,"cycle repeats exactly for every model");}
 p={};p.cycle_evolution=true;check(run(p,repeat),"single cycle starts");p.evolution=70;check(run(p,tiled)&&repeat!=tiled,"single revolution still animates");p.evolution=360;check(run(p,tiled)&&repeat==tiled,"single cycle joins");
 p={};p.uniform_scale=false;p.scale_width=73;p.scale_height=251;check(run(p,repeat)&&repeat!=out,"anisotropic scaling");p.scale=900;check(run(p,tiled)&&repeat==tiled,"uniform scale inactive when disabled");
 p={};p.sub_rotation=30;check(run(p,repeat)&&repeat!=out,"sub rotation active");p={};p.sub_offset_y=100;check(run(p,repeat)&&repeat!=out,"sub offset active");p={};p.center_subscale=true;check(run(p,repeat)&&repeat!=out,"center subscale active");p={};p.offset_x=20;check(run(p,repeat),"offset base");p.perspective_offset=true;check(run(p,tiled)&&tiled!=repeat,"perspective offset active");
 for(int i=0;i<static_cast<int>(fractal::Blend::count);++i){p={};p.blend=static_cast<fractal::Blend>(i);p.opacity=62.5;check(run(p,repeat),"all blend modes render");}
 p={};p.cycles=1.5;check(!run(p,repeat),"fractional cycle rejected");p={};p.fractal=static_cast<fractal::FractalType>(-1);check(!run(p,repeat),"invalid enum rejected");
 {
  std::uint16_t s[]={32768,16384,0,12345},d[4]{};p={};p.contrast=0;
  check(fractal::process(s,sizeof(s),sizeof(s),d,sizeof(d),sizeof(d),1,1,4,p,{},fractal::SampleType::photoshop_u16),"PS16 render");check(d[0]==16384&&d[1]==16384&&d[3]==12345,"PS16 normalization and exact alpha");p.opacity=0;check(fractal::process(s,sizeof(s),sizeof(s),d,sizeof(d),sizeof(d),1,1,4,p,{},fractal::SampleType::photoshop_u16)&&std::memcmp(s,d,sizeof(s))==0,"PS16 identity");
  float fs[]={.2f,.4f,.6f,.123f},fd[4]{};p={};p.contrast=0;p.brightness=200;p.overflow=fractal::Overflow::hdr;
  check(fractal::process(fs,sizeof(fs),sizeof(fs),fd,sizeof(fd),sizeof(fd),1,1,4,p,{},fractal::SampleType::f32)&&fd[0]==2.5f&&fd[3]==fs[3],"float HDR above white and alpha");
  p.brightness=-200;check(fractal::process(fs,sizeof(fs),sizeof(fs),fd,sizeof(fd),sizeof(fd),1,1,4,p,{},fractal::SampleType::f32)&&fd[0]==-1.5f,"float HDR below black");
  p.overflow=fractal::Overflow::clip;check(fractal::process(fs,sizeof(fs),sizeof(fs),fd,sizeof(fd),sizeof(fd),1,1,4,p,{},fractal::SampleType::f32)&&fd[0]==0,"clip overflow");
  p.overflow=fractal::Overflow::wrap_back;check(fractal::process(fs,sizeof(fs),sizeof(fs),fd,sizeof(fd),sizeof(fd),1,1,4,p,{},fractal::SampleType::f32)&&fd[0]==.5f,"wrap overflow reflects negative");
  p={};p.contrast=0;p.blend=fractal::Blend::multiply;check(fractal::process(fs,sizeof(fs),sizeof(fs),fd,sizeof(fd),sizeof(fd),1,1,4,p,{},fractal::SampleType::f32)&&std::abs(fd[0]-.1f)<1e-6,"multiply formula");
  p.blend=fractal::Blend::hue;check(fractal::process(fs,sizeof(fs),sizeof(fs),fd,sizeof(fd),sizeof(fd),1,1,4,p,{},fractal::SampleType::f32)&&std::abs((std::max({fd[0],fd[1],fd[2]})+std::min({fd[0],fd[1],fd[2]}))-.8f)<1e-6,"AE hue preserves HSL lightness");
 }
 constexpr int pw=512,ph=512;std::vector<std::uint8_t> pixels(pw*ph*3,0);p={};p.offset_x=p.offset_y=256;
 {
  const double cx=100.5,cy=75.5;float source[3]{},before[3]{},after[3]{};
  fractal::Parameters q;q.offset_x=17;q.offset_y=43;
  auto sample=[&](const fractal::Parameters& values,double x,double y,float* target){return fractal::process(source,sizeof(source),sizeof(source),target,sizeof(after),sizeof(after),1,1,3,values,{x-.5,y-.5,1,cx,cy},fractal::SampleType::f32);};
  check(sample(q,cx,cy,before),"center reference sample");
  for(double angle:{-90.0,37.0,180.0})for(double scale:{50.0,173.0,600.0}){auto transformed=q;transformed.rotation=angle;transformed.scale=scale;check(sample(transformed,cx,cy,after)&&std::memcmp(before,after,sizeof(before))==0,"scale and rotation preserve fixed document center after offset changes");}
  for(int model=0;model<static_cast<int>(fractal::FractalType::count);++model)for(bool perspective:{false,true}){
   auto base=q;base.fractal=static_cast<fractal::FractalType>(model);base.perspective_offset=perspective;base.evolution=73;base.sub_rotation=19;
   auto changed=base;changed.rotation=37;changed.uniform_scale=false;changed.scale_width=211;changed.scale_height=63;
   check(sample(base,cx,cy,before)&&sample(changed,cx,cy,after)&&std::memcmp(before,after,sizeof(before))==0,"every model preserves document center, including perspective offsets");
  }
  auto transformed=q;transformed.rotation=37;transformed.uniform_scale=false;transformed.scale_width=173;transformed.scale_height=51;
  const double wx=151.25,wy=21.75,a=37*3.14159265358979323846/180;
  const double tx=cx+((wx-cx)*std::cos(a)+(wy-cy)*std::sin(a))*100/173;
  const double ty=cy+(-(wx-cx)*std::sin(a)+(wy-cy)*std::cos(a))*100/51;
  check(sample(q,tx,ty,before)&&sample(transformed,wx,wy,after)&&std::abs(before[0]-after[0])<1e-6,"anisotropic rotation matches inverse transform around document center");
  std::vector<std::uint8_t> whole(row*h,219),parts(row*h,219);
  check(fractal::process(src.data(),src.size(),row,whole.data(),whole.size(),row,w,h,c,transformed,{-33,19,1,cx,cy}),"centered whole frame");
  for(int y=0;y<h;y+=17)for(int x=0;x<w;x+=29){const int start=y*row+x*c;check(fractal::process(src.data()+start,src.size()-start,row,parts.data()+start,parts.size()-start,row,std::min(29,w-x),std::min(17,h-y),c,transformed,{-33.0+x,19.0+y,1,cx,cy}),"centered tile");}
  check(whole==parts,"tile and selection origins do not change transform center");
  std::cout<<"PASS: fixed document-center scale/rotation after offset, inverse mapping, nonuniform scale and seamless crops.\n";
 }
 check(fractal::process(pixels.data(),pixels.size(),pw*3,pixels.data(),pixels.size(),pw*3,pw,ph,3,p),"fixture");
 std::cout<<"BASELINE FNV64="<<hash(pixels)<<'\n';
 check(hash(pixels)==10301387807941917792ULL,"default output remains identical to verified 0.1 baseline");
 std::ofstream fixture("fractal-default.ppm",std::ios::binary);fixture<<"P6\n512 512\n255\n";fixture.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
 constexpr int bw=1920,bh=1080;std::vector<std::uint8_t> bench(bw*bh*3);const auto start=std::chrono::steady_clock::now();check(fractal::process(bench.data(),bench.size(),bw*3,bench.data(),bench.size(),bw*3,bw,bh,3,p),"benchmark");
 std::cout<<"PASS: 17 distinct fractal models; 4 interpolation modes; 29 blends; PS16/float HDR; cycle period and motion; transforms; deterministic seed; fractional complexity; seamless arbitrary tiles; alpha; padding; in-place; guards.\nBENCH: 1920x1080 RGB8, 6 octaves, automatic CPU scheduling, ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<'\n';
}
