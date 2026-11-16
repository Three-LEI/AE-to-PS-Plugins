#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "energy_presets.hpp"
namespace energy {
struct Point {double x=0,y=0;bool operator==(const Point&)const=default;};
struct Knot {Point before,anchor,after;bool operator==(const Knot&)const=default;};
struct Curve {bool closed=false;std::vector<Knot> knots;bool operator==(const Curve&)const=default;};
struct Path {std::vector<Curve> curves;bool operator==(const Path&)const=default;};
struct Parameters {
 int input=0,composite=0;
 double start_x=64,start_y=128,end_x=448,end_y=128;
 double width=3,brightness=1,core_white=85,glow_radius=32,glow_intensity=1;
 std::array<double,3> color{.08,.5,1};
 double trim_start=0,trim_end=100,taper_start=0,taper_end=0,offset_x=0,offset_y=0;
 // Appended in 0.2. The complete 0.1 prefix and descriptor IDs stay stable.
 int style=0,detail=3,branches=3;
 std::uint32_t seed=0;
 double alpha_threshold=50,distortion=24,wavelength=96,evolution=0;
 double branch_length=48,fire_height=96,fire_spread=24,fire_direction=-90;
 std::array<double,3> tip_color{1,.06,.01};
 // Appended in 0.3. Zero selects the frozen 0.1/0.2 light compositor.
 int glow_model=0;
 double halo_intensity=2,halo_radius=10,glow_gamma=1,glow_bias=.5;
 std::array<double,4> glow_sizes{1,19.9/60,8./60,2.9/60};
 std::array<double,4> glow_gains{2.6182,.61563,.12917,.001};
 // 0.4 appends a separate textured-electric style; older styles stay frozen.
 int glow_detail=4,filament_count=3;std::uint32_t glow_seed=5000;
 double glow_distortion=24,glow_noise_scale=40,glow_width=8;
 double glow_texture=1,filament_strength=.8,glow_evolution=0;
 // 0.5 adds path-wrapped burning, leaving the directional legacy flame intact.
 int burn_detail=5;std::uint32_t burn_seed=5000;
 double burn_width=10,burn_turbulence=24,burn_scale=56,burn_density=1;
 double burn_filaments=.7,burn_wind=0,burn_evolution=0;
 // 0.6 uses public stages instead of a global material selector. Zero keeps
 // all previously saved engines intact; one enables the common pipeline.
 int engine=0,core_noise=1,glow_noise=1;
 double color_mix=0;
 bool operator==(const Parameters&)const=default;
};
struct Image {int width=0,height=0,left=0,top=0;std::vector<float> rgba;bool valid()const;};
using Cancel=std::function<bool()>;
Parameters defaults(int width,int height,int left=0,int top=0);
Parameters calibrated_defaults(int width,int height,int left=0,int top=0);
void reset_glow(Parameters&);
void reset_electric(Parameters&);
void reset_burning(Parameters&);
void reset_unified(Parameters&);
Parameters unified_defaults(int width,int height,int left=0,int top=0);
Parameters unified_preset(int,const Parameters& geometry);
Parameters to_unified(const Parameters&);
bool valid(const Parameters&);
bool valid_path(const Path&);
// Adobe path-resource records: big endian, 26 bytes, vertical Q8.24 first.
bool parse_path(const std::vector<std::uint8_t>&,double width,double height,Path&,std::string& error);
// Portable versioned geometry snapshot, with strict limits and no native padding.
std::vector<std::uint8_t> encode_path(const Path&);
bool decode_path(const std::vector<std::uint8_t>&,Path&);
Path line_path(const Parameters&);
struct Segment {Point a,b;double u0=0,u1=0;};
bool flatten(const Path&,double tolerance,std::vector<Segment>&,const Cancel& cancel={});
bool render(const Image&,const Parameters&,const Path&,Image&,const Cancel& cancel={});
bool trace_alpha(const Image&,double threshold_percent,Path&,const Cancel& cancel={});
void reset_extension(Parameters&);
Parameters material_preset(int,const Parameters& geometry);
Parameters draft_parameters(Parameters,double left,double top,double step);
Path draft_path(Path,double left,double top,double step);
}
