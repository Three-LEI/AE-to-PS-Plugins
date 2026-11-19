#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <atomic>
#include <memory>
#include <vector>
namespace caustics {
struct Parameters {
 int surface=0,wave=0,channel=0,output=0,blend=0,border=1,quality=2;
 double scale=160,complexity=2.5,height=18,evolution=0,shift_x=0,shift_y=0,map_softness=4;
 double depth=120,refractive_index=1.333,focus=1.5,softness=1.5;
 double intensity=100,threshold=1,refraction=100,background=100,mix=100;
 std::array<double,3> light{.65,.9,1},dark{0,.04,.12};
 std::uint32_t seed=123;
 std::uint64_t map_key=0;
 bool preserve_alpha=true;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&width<=2000000&&height<=2000000&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
// All water geometry stays in native document pixels, including reduced previews.
struct RenderView {double left=0,top=0,width=0,height=0,step=1;};
using Cancel=std::function<bool()>;
struct CacheStats {std::size_t hits=0,builds=0,bytes=0;};
// One preview worker owns the cache. Input snapshots must remain immutable;
// stop the worker and clear before changing or replacing a reference snapshot.
class RenderCache {
 struct State;std::unique_ptr<State> state_;std::size_t budget_;
 std::atomic<std::size_t> hits_{0},builds_{0},bytes_{0};
 friend bool render(const Image&,const Parameters&,Image&,const Image*,const Cancel&,RenderView,RenderCache*);
public:
 explicit RenderCache(std::size_t budget=96*1024*1024);
 ~RenderCache();
 RenderCache(const RenderCache&)=delete;RenderCache& operator=(const RenderCache&)=delete;
 void clear();
 CacheStats stats()const{return {hits_.load(),builds_.load(),bytes_.load()};}
};
bool valid(const Parameters&);
Parameters preset(int);
bool render(const Image&,const Parameters&,Image&,const Image* map=nullptr,const Cancel& cancel={},RenderView view={},RenderCache* cache=nullptr);
}
