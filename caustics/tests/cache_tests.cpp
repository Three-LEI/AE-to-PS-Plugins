#include "caustics_core.hpp"
#include <cfenv>
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace caustics;
int checks=0;void check(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
Image fixture(int w,int h){Image s{w,h,-13,7,{}};s.rgba.resize(std::size_t(w)*h*4);for(std::size_t n=0;n<s.rgba.size();n++)s.rgba[n]=float((n*17)%131)/(n%4==3?130:87);return s;}
bool same(const Image& a,const Image& b){return a.width==b.width&&a.height==b.height&&a.left==b.left&&a.top==b.top&&a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*sizeof(float));}
int main(){try{
 Image src=fixture(97,61),map=fixture(103,67),a,b;Parameters base;RenderCache cache;
 auto compare=[&](const Parameters& p,const Image* height=nullptr,RenderView view={}){check(render(src,p,a,height,{},view,&cache)&&render(src,p,b,height,{},view)&&same(a,b),"cached/direct pixels differ");};
 compare(base);check(cache.stats().builds==1&&cache.stats().bytes>0,"initial transport retained");const auto initial=cache.stats().builds;
 for(int control=0;control<10;control++)for(int value=0;value<3;value++){
  auto p=base;if(control==0)p.intensity=37+value*70;if(control==1)p.threshold=value*.7;if(control==2)p.mix=15+value*35;if(control==3)p.background=20+value*67;
  if(control==4)p.blend=value;if(control==5)p.output=value+1;if(control==6)p.light={.12*value,.4,.8};if(control==7)p.dark={.1,.2*value,.3};if(control==8)p.preserve_alpha=value!=0;if(control==9){p.blend=3;p.output=value;}
  compare(p);check(cache.stats().builds==initial,"composition control rebuilt transport");
 }
 std::vector<std::function<void(Parameters&)>> mutations;
 auto number=[&](double Parameters::*field,double value){mutations.push_back([=](Parameters& p){p.*field=value;});};
 number(&Parameters::scale,117);number(&Parameters::complexity,4.2);number(&Parameters::height,29);number(&Parameters::evolution,19);number(&Parameters::shift_x,-19);number(&Parameters::shift_y,41);number(&Parameters::map_softness,9);number(&Parameters::depth,53);number(&Parameters::refractive_index,1.6);number(&Parameters::focus,.7);number(&Parameters::softness,5);number(&Parameters::refraction,75);
 for(int i=0;i<3;i++)mutations.push_back([=](Parameters& p){p.wave=i;});for(int i=0;i<5;i++)mutations.push_back([=](Parameters& p){p.channel=i;});for(int i=0;i<4;i++)mutations.push_back([=](Parameters& p){p.border=i;});for(int i=1;i<=3;i++)mutations.push_back([=](Parameters& p){p.quality=i;});mutations.push_back([](Parameters& p){p.seed=0xffffffffu;});
 for(int source=0;source<3;source++)for(const auto& change:mutations){auto p=base;p.surface=source;change(p);compare(p,source?&map:nullptr);}
 auto p=base;p.surface=2;compare(p,&map);auto other=map;other.rgba[37]=.1f;compare(p,&other);check(cache.stats().hits>0,"cache hit path covered");
 auto changed_source=src;changed_source.rgba[41]=.9f;check(render(changed_source,base,a,nullptr,{}, {},&cache)&&render(changed_source,base,b)&&same(a,b),"new source snapshot invalidates refracted bottom");
 cache.clear();src.rgba[44]=.2f;compare(base);
 cache.clear();check(cache.stats().bytes==0,"clear releases bytes");compare(base);const auto built=cache.stats().builds;auto proxy=fixture(49,31);check(render(proxy,base,a,nullptr,{}, {-13,7,97,61,2},&cache),"draft transport");compare(base);check(cache.stats().builds==built+1,"full and draft both retained");
 Image saved=a;cache.clear();int polls=0;check(!render(src,base,a,nullptr,[&]{return ++polls>4;},{},&cache)&&same(saved,a)&&cache.stats().bytes==0,"cancelled build publishes no pixels or cache");compare(base);saved=a;const auto bytes=cache.stats().bytes;check(!render(src,base,a,nullptr,[]{return true;},{},&cache)&&same(saved,a)&&cache.stats().bytes==bytes,"cancelled hit preserves valid cache");
 RenderCache tiny(1);check(render(src,base,a,nullptr,{}, {},&tiny)&&render(src,base,b)&&same(a,b)&&tiny.stats().bytes==0,"budget fallback exact");
 const auto rounding=std::fegetround();cache.clear();for(int mode:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}){std::fesetround(mode);compare(base);}std::fesetround(rounding);
 cache.clear();for(int i=0;i<5;i++){p=base;p.seed+=i;compare(p);check(cache.stats().bytes<=96*1024*1024,"bounded LRU");}
 std::cout<<"PASS: "<<checks<<" cache assertions; composition reuse, every transport control, native reference snapshots, full/draft LRU, budget, rounding, cancellation and recovery\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
