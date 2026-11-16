#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
namespace lightfx {
enum class Kind:int{starglow,shine};
using Color=std::array<double,3>;
using Map=std::array<Color,5>;
struct Parameters {
 Kind kind=Kind::starglow;
 int input=1,blend=0,map_mode=0,samples=64;
 bool source_color=false,preserve_alpha=false,use_mask=false,cycle=false;
 double threshold=.75,softness=.15,boost=1,length=1,rotation=0,falloff=1;
 std::array<double,8> lengths{100,100,100,100,100,100,100,100};
 std::array<int,8> maps{0,0,0,0,0,0,0,0};
 std::array<Map,3> colors{};
 double cx=0,cy=0,mask_cx=0,mask_cy=0,mask_radius=200,mask_feather=100;
 double shimmer=0,detail=40,phase=0,loops=1,noise_x=0,noise_y=0;
 std::uint32_t seed=0;
 double source_opacity=100,effect_opacity=100,mix=100;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;
 double step=1;
 std::vector<float> rgba;
 bool valid()const;
};
using Cancel=std::function<bool()>;
Parameters defaults(Kind,int width,int height,int left=0,int top=0);
Parameters preset(const Parameters&,int index);
bool valid(const Parameters&);
// Drafts use a box-filtered source with document coordinates and pixel spacing.
// The final host path always uses the original image at step=1.
Image downsample(const Image&,int max_side,const Cancel& cancel={},bool emission=false);
bool same_extraction(const Parameters&,const Parameters&);
std::shared_ptr<const Image> prepare(const Image&,const Parameters&,const Cancel& cancel={});
bool render_prepared(const Image&,const Image& emission,const Parameters&,Image&,const Cancel& cancel={});
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={});
} // namespace lightfx
