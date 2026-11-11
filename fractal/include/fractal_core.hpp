#pragma once
#include <cstddef>
#include <cstdint>
namespace fractal {
// Independent reconstruction models; not Adobe's implementation or pixel
// calibration.
enum class FractalType {
  basic,
  turbulent_smooth,
  turbulent_basic,
  turbulent_sharp,
  dynamic,
  dynamic_progressive,
  dynamic_twist,
  maximum,
  smeary,
  swirly,
  rocky,
  cloudy,
  terrain,
  subscale,
  small_bumps,
  strings,
  threads,
  count
};
enum class NoiseType { block, linear, soft_linear, spline, count };
enum class Overflow { clip, soft_clamp, wrap_back, hdr, count };
enum class Blend {
  normal,
  none,
  dissolve,
  dancing_dissolve,
  darken,
  multiply,
  color_burn,
  linear_burn,
  darker_color,
  lighten,
  screen,
  color_dodge,
  add,
  lighter_color,
  overlay,
  soft_light,
  hard_light,
  vivid_light,
  linear_light,
  pin_light,
  hard_mix,
  difference,
  exclusion,
  subtract,
  divide,
  hue,
  saturation,
  color,
  luminosity,
  count
};
enum class SampleType { u8, photoshop_u16, f32 };
struct Parameters {
  double contrast = 100, brightness = 0, scale = 100, rotation = 0,
         offset_x = 0, offset_y = 0;
  double complexity = 6, sub_influence = 70, sub_scale = 56, evolution = 0,
         opacity = 100;
  std::uint32_t seed = 0;
  bool invert = false;
  FractalType fractal = FractalType::basic;
  NoiseType noise = NoiseType::soft_linear;
  Overflow overflow = Overflow::hdr;
  Blend blend = Blend::normal;
  double scale_width = 100, scale_height = 100, sub_rotation = 0,
         sub_offset_x = 0, sub_offset_y = 0, cycles = 1;
  bool uniform_scale = true, perspective_offset = false,
       center_subscale = false, cycle_evolution = false;
};
bool valid(const Parameters &) noexcept;
struct Region {
  double x = 0, y = 0, step = 1;
  // Full document center, shared by every tile/selection/preview crop.
  double center_x = 0, center_y = 0;
};
// Interleaved RGB/RGBA, independent byte strides. PS16 white is 32768.
// Exact in-place or disjoint buffers; alpha samples are copied bit for bit.
bool process(const void *src, std::size_t src_bytes, std::size_t src_row,
             void *dst, std::size_t dst_bytes, std::size_t dst_row,
             std::size_t width, std::size_t height, unsigned channels,
             const Parameters &, Region = {},
             SampleType = SampleType::u8) noexcept;
} // namespace fractal
