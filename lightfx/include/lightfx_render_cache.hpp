#pragma once
#include "lightfx_core.hpp"
#include "../../native/common/stage_cache.hpp"
namespace lightfx {
struct RaysCacheKey {
 Parameters parameters;
 const float* source=nullptr;
 const float* emission=nullptr;
 std::array<int,4> geometry{};
 double step=1;
 int rounding=0;
 unsigned simd_control=0;
 bool operator==(const RaysCacheKey&)const=default;
};
// Owned by the preview render thread. Inputs are immutable snapshots; clear
// before replacing either snapshot. Final native rendering remains uncached.
class RenderCache {
 pscpu::StageCache<RaysCacheKey,Image> stages_;
public:
 explicit RenderCache(std::size_t budget=96u*1024*1024):stages_(budget){}
 void clear(){stages_.clear();}
 std::size_t hits()const{return stages_.hits();}
 std::size_t builds()const{return stages_.builds();}
 std::size_t bytes()const{return stages_.bytes();}
 bool render(const Image&,const Image& emission,const Parameters&,Image&,const Cancel& cancel={});
};
}
