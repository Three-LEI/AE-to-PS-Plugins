#include "glow_render_cache.hpp"
#include "../native/common/range_pool.hpp"
#include <chrono>
#include <cfenv>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
void check(bool ok,const std::string& message){if(!ok)throw std::runtime_error(message);}
glow::Image fixture(int w,int h){glow::Image a{w,h,-7,11,{}};a.rgba.resize(std::size_t(w)*h*4);for(std::size_t i=0;i<a.rgba.size();i++)a.rgba[i]=float((i*17)%131)/97;return a;}
bool same(const glow::Image& a,const glow::Image& b){return a.width==b.width&&a.height==b.height&&a.left==b.left&&a.top==b.top&&a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*4);}
#ifdef AEPS_VERIFY_FROZEN_GLOW
bool baseline_glow_render(const glow::Image&,const glow::Parameters&,glow::Image&,const glow::Image*,const glow::Image*);
#endif
int main(){try{
 pscpu::ParallelScope parallel;
 std::size_t cases=0;
 auto src=fixture(97,61),mask=fixture(31,47),texture=fixture(19,13);
 using P=glow::Parameters;
 std::vector<std::pair<std::string,std::function<void(P&)>>> changes={
 {"based",[](P& p){p.based_on=1;}},{"threshold",[](P& p){p.threshold=23;}},{"radius",[](P& p){p.radius=13.7;}},
 {"zero intensity",[](P& p){p.intensity=0;}},{"intensity",[](P& p){p.intensity=1.7;}},{"source view",[](P& p){p.view=2;}},{"glow view",[](P& p){p.view=1;}},
 {"screen",[](P& p){p.blend=glow::Blend::screen;}},{"add",[](P& p){p.blend=glow::Blend::add;}},{"multiply",[](P& p){p.blend=glow::Blend::multiply;}},{"normal",[](P& p){p.blend=glow::Blend::normal;}},{"lighten",[](P& p){p.blend=glow::Blend::lighten;}},
 {"composite",[](P& p){p.composite=2;}},{"colors",[](P& p){p.colors=1;}},{"looping",[](P& p){p.looping=2;}},{"loops",[](P& p){p.loops=2.3;}},{"phase",[](P& p){p.phase=23;}},{"midpoint",[](P& p){p.midpoint=23;}},
 {"color a",[](P& p){p.color_a={.2,.7,.3};}},{"color b",[](P& p){p.color_b={.8,.3,.2};}},{"dimensions",[](P& p){p.dimensions=2;}},{"falloff",[](P& p){p.falloff=1;}},{"threshold mode",[](P& p){p.threshold_mode=2;}},
 {"tint",[](P& p){p.tint=2;}},{"tone",[](P& p){p.tone=1;}},{"quality",[](P& p){p.quality=0;}},{"smoothness",[](P& p){p.smoothness=27;}},{"saturation",[](P& p){p.saturation_bias=33;}},
 {"gamma",[](P& p){p.gamma=1.7;}},{"gamma amount",[](P& p){p.gamma_amount=50;}},{"aspect",[](P& p){p.aspect=1.7;}},{"angle",[](P& p){p.angle=31;}},{"shift",[](P& p){p.chromatic_shift=2.3;}},{"spread",[](P& p){p.chromatic_spread=7;}},
 {"tint amount",[](P& p){p.tint_amount=32;}},{"source opacity",[](P& p){p.source_opacity=37;}},{"mask key",[](P& p){p.mask_key=123;}},{"texture key",[](P& p){p.texture_key=124;}},
 {"invert mask",[](P& p){p.invert_mask=true;}},{"unmult",[](P& p){p.unmult=false;}},{"edge",[](P& p){p.repeat_edges=false;}},{"mask mode",[](P& p){p.mask_mode=1;}},
 {"texture opacity",[](P& p){p.texture_opacity=43;}},{"texture glow",[](P& p){p.texture_glow=37;}},{"texture exposure",[](P& p){p.texture_exposure=1.2;}},{"texture gamma",[](P& p){p.texture_gamma=1.6;}}
 };
 for(auto kind:{glow::Kind::classic,glow::Kind::deep})for(bool external:{false,true}){
  glow::RenderCache cache;auto base=glow::defaults(kind);base.radius=23;base.colors=1;base.tint=2;
  auto verify=[&](const P& p){glow::Image cached,fresh;check(cache.render(src,p,cached,external?&mask:nullptr,external?&texture:nullptr),"cached render");check(glow::render(src,p,fresh,external?&mask:nullptr,external?&texture:nullptr),"fresh render");check(same(cached,fresh),"cache pixel mismatch");
#ifdef AEPS_VERIFY_FROZEN_GLOW
   glow::Image old;check(baseline_glow_render(src,p,old,external?&mask:nullptr,external?&texture:nullptr)&&same(fresh,old),"frozen pre-optimization kernel mismatch");
#endif
   ++cases;};
  verify(base);auto builds=cache.builds();verify(base);check(cache.builds()==builds&&cache.hits()>0,"same frame missed cache");
  auto intensity=base;intensity.intensity=1.7;verify(intensity);check(cache.builds()==builds,"nonzero intensity rebuilt blur");
  for(const auto& change:changes){verify(base);auto p=base;change.second(p);try{verify(p);}catch(const std::exception& e){throw std::runtime_error(change.first+": "+e.what());}}
  auto src2=src;src2.rgba[0]=.25f;glow::Image cached,fresh;check(cache.render(src2,base,cached)&&glow::render(src2,base,fresh)&&same(cached,fresh),"new source snapshot reused old pixels");++cases;
  auto failed=base;failed.radius=35;cache.clear();int polls=0;check(!cache.render(src,failed,cached,nullptr,nullptr,[&]{return ++polls>3;}),"cancel ignored");check(cache.bytes()==0,"cancelled stage retained");verify(failed);
  auto old=std::fegetround();std::fesetround(FE_DOWNWARD);verify(base);std::fesetround(FE_UPWARD);verify(base);std::fesetround(old);
 }
 glow::RenderCache tiny(8);glow::Image cached,fresh;auto p=glow::defaults(glow::Kind::deep);
 check(tiny.render(src,p,cached)&&glow::render(src,p,fresh)&&same(cached,fresh)&&tiny.bytes()==0,"over-budget fallback");
 pscpu::StageCache<int,std::vector<float>> bounded(80);auto build=[](auto& value){value.resize(10);return true;};
 check(bounded.get(1,40,build)&&bounded.get(2,40,build)&&bounded.bytes()==80,"two-slot budget");
 check(bounded.get(3,80,build)&&bounded.bytes()==80,"LRU eviction");
 check(!bounded.get(4,81,build),"oversize admitted");
 check(!bounded.get(5,40,[](auto&){return false;}),"failed builder committed");
 check(bounded.get(6,40,build)&&bounded.bytes()<=80,"builder recovery");
 auto big=fixture(768,512);glow::RenderCache warm;auto setting=glow::defaults(glow::Kind::deep);setting.radius=100;
 auto measure=[&](auto work){auto t=std::chrono::steady_clock::now();work();return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();};
 check(warm.render(big,setting,cached),"warmup");
 const auto uncached_ms=measure([&]{for(int i=0;i<5;i++){setting.intensity=.8+i*.13;check(glow::render(big,setting,fresh),"benchmark fresh");}})/5;
 const auto cached_ms=measure([&]{for(int i=0;i<5;i++){setting.intensity=.8+i*.13;check(warm.render(big,setting,cached),"benchmark cached");}})/5;
 check(same(cached,fresh),"benchmark result differs");
 std::cout<<"PASS: "<<cases<<" cached/fresh float images, all parameter dependencies, snapshots, cancellation, rounding, budget and recovery.\n";
#ifdef AEPS_VERIFY_FROZEN_GLOW
 std::cout<<"PASS: every parameter variant also matches the frozen pre-optimization Glow kernel byte-for-byte.\n";
#endif
 std::cout<<"DeepGlow 768x512 intensity changes: fresh="<<uncached_ms<<" ms; cached="<<cached_ms<<" ms; retained="<<warm.bytes()<<" bytes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
