#pragma once
#include <cstdint>
#include <vector>
namespace displacement {
enum class Channel : int {red,green,blue,alpha,luminance,hue,lightness,saturation,none};
enum class Placement : int {center,stretch,tile};
struct Parameters {
  std::uint32_t layer_id=0xFFFFFFFFu;
  std::uint64_t layer_name_key=0;
  Channel horizontal=Channel::red, vertical=Channel::green;
  double amount_x=5, amount_y=5;
  Placement placement=Placement::center;
  bool wrap=false;
};
// Linear storage of unassociated, native-document colour values. RGB may be HDR.
struct Image {
  int width=0,height=0;
  double left=0,top=0,step=1,midpoint=.5;
  std::vector<float> rgba;
  bool valid() const;
};
struct Region {double left=0,top=0,step=1; int width=0,height=0;};
bool valid(const Parameters&);
// Output is unassociated RGBA. Source lookup interpolates premultiplied colour.
bool render(const Image& source,const Image& map,const Parameters&,Region,float* output);
}
