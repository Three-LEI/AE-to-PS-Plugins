#pragma once
#include <cstdint>
#include <functional>
#include <vector>
namespace vectorblur {
enum class Type:int {natural,constant,perpendicular,direction_center,direction_fading};
enum class Property:int {red,green,blue,alpha,luminance,lightness,hue,saturation};
enum class Edge:int {transparent,repeat,reflect,wrap};
struct Parameters {
 Type type=Type::natural;
 double amount=0,angle=0,ridge=1,revolutions=1,map_softness=15,mix=100;
 Property property=Property::lightness;
 std::uint64_t map_key=0;
 bool invert_map=false,follow=false,preserve_alpha=false,linear_light=false;
 Edge edge=Edge::transparent;
 int quality=1; // draft / standard / high / ultra
 std::uint64_t kernel_version=2; // appended; v0.1 saved parameters are migrated by the host adapter
 bool operator==(const Parameters&) const = default;
};
struct Image {
 int width=0,height=0,left=0,top=0;
 std::vector<float> rgba; // native RGB, unassociated alpha; HDR is retained
 bool valid() const;
};
struct Vec {float x=0,y=0;};
struct Field {
 int width=0,height=0;
 std::vector<Vec> vectors;
 Vec at(float x,float y) const;
};
using Cancel=std::function<bool()>;
bool valid(const Parameters&);
Parameters preset(int index); // 0 default, 1 flow, 2 contours, 3 wind, 4 ribbons
bool make_field(const Image&,const Parameters&,Field&,const Image* map=nullptr,const Cancel& cancel={});
// The optional field output is for diagnostic preview only. No host APIs in workers.
bool render(const Image&,const Parameters&,Image&,const Image* map=nullptr,const Cancel& cancel={},Field* field=nullptr);
}
