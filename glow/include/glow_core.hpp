#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace glow {
enum class Kind : int {classic,deep};
enum class Blend : int {add,screen,normal,lighten,multiply};
struct Parameters {
 Kind kind=Kind::classic;
 int based_on=0,composite=0,colors=0,looping=0,dimensions=0;
 Blend blend=Blend::add;
 double threshold=60,radius=10,intensity=1,loops=1,phase=0,midpoint=50;
 std::array<double,3> color_a{1,1,1},color_b{0,0,0};
 int falloff=0,threshold_mode=0,tint=0,tone=0,quality=1,view=0;
 double smoothness=0,saturation_bias=0,gamma=2.2,gamma_amount=100;
 double aspect=1,angle=0,chromatic_shift=0,chromatic_spread=0;
 double tint_amount=100,source_opacity=100;
 std::uint64_t mask_key=0,texture_key=0;
 bool invert_mask=false,unmult=true,repeat_edges=true;
 int mask_mode=0;
 double texture_opacity=100,texture_glow=0,texture_exposure=0,texture_gamma=1;
 bool operator==(const Parameters&) const = default;
};
struct Image {
 int width=0,height=0,left=0,top=0;
 std::vector<float> rgba; // unassociated, native document RGB; alpha in [0,1]
 bool valid() const;
};
Parameters defaults(Kind);
bool valid(const Parameters&);
using Cancel=std::function<bool()>;
// Linear-light emission blur + encoded light/coverage compositing for deep;
// three-box Gaussian approximation for classic.
// External inputs are resampled to the source canvas. No proprietary code/assets.
bool render(const Image&,const Parameters&,Image&,const Image* mask=nullptr,
            const Image* texture=nullptr,const Cancel& cancel={});
}
