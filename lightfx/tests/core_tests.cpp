#include "lightfx_core.hpp"
#include "psui/coalesced_preview.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <thread>
using namespace lightfx;
std::size_t checks=0;void check(bool v,const char* why){++checks;if(!v){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
Image fixture(int w,int h){Image src{w,h,-13,7,1,std::vector<float>(std::size_t(w)*h*4)};for(int y=0;y<h;y++)for(int x=0;x<w;x++){const auto i=(std::size_t(y)*w+x)*4;src.rgba[i]=float(x)/std::max(1,w-1)*2-.25f;src.rgba[i+1]=float(y)/std::max(1,h-1);src.rgba[i+2]=.2f;src.rgba[i+3]=x>2&&y>2&&x<w-3&&y<h-3?1.f:0.f;}return src;}
void ppm(const Image& a,const std::filesystem::path& path){std::ofstream f(path,std::ios::binary);f<<"P6\n"<<a.width<<' '<<a.height<<"\n255\n";for(std::size_t i=0;i<a.rgba.size();i+=4)for(int k=0;k<3;k++){const auto v=std::uint8_t(std::lround(std::clamp(a.rgba[i+k]*a.rgba[i+3],0.f,1.f)*255));f.write(reinterpret_cast<const char*>(&v),1);}}
int main(int argc,char** argv){
 const auto folder=std::filesystem::path(argc>1?argv[1]:"build/core");std::filesystem::create_directories(folder);auto src=fixture(129,97);Image out,again;
 for(Kind kind:{Kind::starglow,Kind::shine}){
  auto p=defaults(kind,src.width,src.height,src.left,src.top);check(valid(p),"factory validity");check(p.cx==51.5&&p.cy==55.5,"document center");p.mix=0;check(render(src,p,out)&&out.rgba==src.rgba,"zero mix is bit-exact including hidden RGB/HDR");p.mix=100;p.length=0;check(render(src,p,out)&&out.rgba==src.rgba,"zero length neutral");p.length=kind==Kind::shine?1:40;p.boost=0;check(render(src,p,out)&&out.rgba==src.rgba,"zero boost neutral");p.boost=1;
  const auto previous=out;check(!render(src,p,out,[]{return true;})&&out.rgba==previous.rgba,"cancellation is atomic");p.length=-1;check(!valid(p)&&!render(src,p,out),"invalid length rejected");p.length=1;p.cx=std::numeric_limits<double>::infinity();check(!valid(p),"nonfinite coordinate rejected");
  p=defaults(kind,src.width,src.height,src.left,src.top);p.shimmer=55;p.seed=0xffffffffu;p.map_mode=2;p=preset(p,3);p.shimmer=55;
  auto seed=prepare(src,p);check(seed&&render_prepared(src,*seed,p,out)&&render(src,p,again)&&out.rgba==again.rgba,"prepared path is bit-exact to final");check(render(src,p,again)&&out.rgba==again.rgba,"noise deterministic");p.phase=80;check(render(src,p,again)&&out.rgba!=again.rgba,"phase changes shimmer");p.cycle=true;p.phase=0;p.loops=2;check(render(src,p,out),"loop start");p.phase=720;check(render(src,p,again)&&out.rgba==again.rgba,"loop full period bit-exact");
  p.preserve_alpha=true;check(render(src,p,out),"preserved alpha render");for(std::size_t i=3;i<src.rgba.size();i+=4)check(out.rgba[i]==src.rgba[i],"preserve alpha exact");for(float v:out.rgba)check(std::isfinite(v),"finite HDR result");
  auto alias=src;check(render(alias,p,alias)&&alias.rgba==out.rgba,"in-place rendering");p=defaults(kind,src.width,src.height,src.left,src.top);auto cached=p;p.cx+=12;check(same_extraction(p,cached),"center drag reuses extraction");p.threshold+=.1;check(!same_extraction(p,cached),"threshold invalidates extraction");
 }
 Image impulse{17,17,0,0,1,std::vector<float>(17*17*4)};auto p=defaults(Kind::starglow,17,17);p.threshold=0;p.softness=0;p.length=4;p.boost=1;p.lengths.fill(0);p.lengths[0]=100;const auto center=(8*17+8)*4;impulse.rgba[center]=impulse.rgba[center+1]=impulse.rgba[center+2]=impulse.rgba[center+3]=1;
 check(render(impulse,p,out),"analytic impulse");const double q=std::exp(-.75),gain=.5;for(int x=9;x<17;x++){const auto i=(8*17+x)*4;check(std::abs(out.rgba[i]*out.rgba[i+3]-(-std::expm1(-std::pow(q,x-8)*gain)))<1e-6,"one-sided exponential profile");}check(out.rgba[(8*17+7)*4+3]==0,"no backwards streak");
 p.lengths.fill(100);for(int angle:{0,13,45,90,137}){p.rotation=angle;check(render(impulse,p,out),"rotated eight-direction star");for(float v:out.rgba)check(std::isfinite(v),"rotation finite");}
 auto tiny=fixture(1,1);for(auto kind:{Kind::starglow,Kind::shine}){p=defaults(kind,1,1);check(render(tiny,p,out),"single pixel render");p.cx=-1e7;p.cy=1e7;check(render(tiny,p,out),"off-canvas source point safe");}
 Image black{4,4,0,0,1,std::vector<float>(64)};for(std::size_t i=3;i<64;i+=4)black.rgba[i]=1;for(auto kind:{Kind::starglow,Kind::shine}){p=defaults(kind,4,4);p.threshold=0;p.softness=1;check(render(black,p,out)&&out.rgba==black.rgba,"soft threshold never creates light from black");}
 auto extreme=fixture(4,4);for(auto& v:extreme.rgba)v=std::numeric_limits<float>::max();for(auto kind:{Kind::starglow,Kind::shine}){p=defaults(kind,4,4);p.boost=100;check(render(extreme,p,out),"maximum finite HDR render");for(float v:out.rgba)check(std::isfinite(v),"maximum finite HDR stays finite");}
 auto small=downsample(src,32);check(small.valid()&&small.step==5&&small.left==src.left&&small.width==26,"draft keeps document-space scale");auto extract=prepare(src,defaults(Kind::starglow,129,97,-13,7));auto reduced=downsample(*extract,32,{},true);double a=0,b=0;for(std::size_t i=3;i<extract->rgba.size();i+=4)a+=extract->rgba[i];for(int y=0;y<reduced.height;y++)for(int x=0;x<reduced.width;x++)b+=reduced.rgba[(std::size_t(y)*reduced.width+x)*4+3]*std::min(5,129-x*5)*std::min(5,97-y*5);check(std::abs(a-b)<.001,"draft emission conserves highlight energy");
 std::atomic<int> started=0,finished=0;psui::CoalescedPreview<int> worker;auto start=std::chrono::steady_clock::now();
 for(int i=0;i<50;i++)worker.submit(true,[&,i](const Cancel& cancel){++started;for(int j=0;j<8;j++){if(cancel())return -1;std::this_thread::sleep_for(std::chrono::milliseconds(1));}++finished;return i;});
 check(std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<100,"submitting never joins active work");for(int i=0;i<200&&worker.pending();i++)std::this_thread::sleep_for(std::chrono::milliseconds(5));int value=0;bool draft=false;std::uint64_t revision=0;check(worker.take(value,draft,revision)&&value==49&&draft&&started<10,"queued requests coalesce to newest");
 worker.submit(false,[&](const Cancel& cancel){++started;while(!cancel())std::this_thread::yield();return -10;});std::this_thread::sleep_for(std::chrono::milliseconds(2));const auto final=worker.submit(false,[](const Cancel&){return 777;});for(int i=0;i<100&&worker.pending();i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));check(worker.take(value,draft,revision)&&value==777&&!draft&&revision==final,"obsolete full job cancels and newest full result wins");
 auto bench=fixture(1536,1024);std::ofstream raw(folder/"deterministic.f32",std::ios::binary);
 for(auto kind:{Kind::starglow,Kind::shine}){
  p=defaults(kind,1536,1024,-13,7);auto seed=prepare(bench,p);auto draft_src=downsample(bench,256),draft_seed=downsample(*seed,256,{},true);
  for(bool draft:{true,false}){const auto t=std::chrono::steady_clock::now();check(render_prepared(draft?draft_src:bench,draft?draft_seed:*seed,p,out),"benchmark render");const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();std::cout<<"BENCH: "<<(kind==Kind::starglow?"Starglow":"Shine")<<" "<<(draft?"draft256":"full1536x1024")<<" ms="<<ms<<'\n';}
  ppm(out,folder/(kind==Kind::starglow?"starglow.ppm":"shine.ppm"));
  for(int i=0;i<8;i++){auto q=preset(p,i%5);q.rotation=i*13;q.phase=i*17;q.shimmer=i*9;q.cx+=i*7;check(render(src,q,out),"serial/parallel fixture");raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*4);}
 }
 std::cout<<"PASS: "<<checks<<" assertions; star impulse/rotation; logarithmic rays; alpha/HDR; cache/final identity; emission-preserving drafts; cancellation; latest-request scheduling.\n";
}
