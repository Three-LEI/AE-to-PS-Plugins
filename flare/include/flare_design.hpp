#pragma once
#include "flare_core.hpp"
#include <string>
namespace flare {
enum class ElementType:int {hotspot,glow,rays,streak,ring,aperture,arc,multi_aperture,glint,sparkle,hoop,spectrum_band};
inline constexpr int element_type_count=12,builtin_preset_count=12;
inline constexpr std::size_t max_elements=64,max_design_bytes=65536;
struct ElementValues {
 int type=0,count=12,blades=6;bool enabled=true;
 double brightness=1,size=1,aspect=1,axis=0,x=0,y=0,rotation=0,softness=.18,spread=1,chromatic=0;
 Color color{1,1,1};
 bool operator==(const ElementValues&)const=default;
};
struct Element:ElementValues {
 std::string name;bool custom_name=false;int name_index=0,copies=0;
 bool operator==(const Element&)const=default;
};
struct Design {bool enabled=false;std::vector<Element> elements;bool operator==(const Design&)const=default;};
bool valid_design(const Design&);
Element make_element(int type);
Element duplicate_element(const Element&,bool chinese=false);
std::string element_name(const Element&,bool chinese);
Design design_from_legacy(const Parameters&);
std::vector<std::uint8_t> encode_design(const Design&);
bool decode_design(const std::vector<std::uint8_t>&,Design&);
struct FlarePreset {Parameters style;Design design;};
FlarePreset design_preset(int index,int width,int height,int left=0,int top=0);
std::vector<std::uint8_t> encode_preset(const FlarePreset&);
bool decode_preset(const std::vector<std::uint8_t>&,FlarePreset&);
// Positions, masks and output controls belong to the receiving document.
void apply_preset(const FlarePreset&,Parameters&,Design&);
}
