#pragma once
#include "glow_core.hpp"
#include "../../native/common/stage_cache.hpp"
namespace glow {
struct PreparedGlow {Image seed,light;double total_weight=0;};
struct GlowCacheKey {
 Parameters parameters;
 const float* source=nullptr;
 const float* mask=nullptr;
 std::array<int,8> geometry{};
 int rounding=0;
 unsigned simd_control=0;
 bool operator==(const GlowCacheKey&)const=default;
};
// Preview-only snapshot cache. Final native rendering remains uncached.
class RenderCache {
 pscpu::StageCache<GlowCacheKey,PreparedGlow> stages_;
public:
 explicit RenderCache(std::size_t budget=96u*1024*1024):stages_(budget){}
 void clear(){stages_.clear();}
 std::size_t hits()const{return stages_.hits();}
 std::size_t builds()const{return stages_.builds();}
 std::size_t bytes()const{return stages_.bytes();}
 bool render(const Image&,const Parameters&,Image&,const Image* mask=nullptr,const Image* texture=nullptr,const Cancel& cancel={});
};
}
