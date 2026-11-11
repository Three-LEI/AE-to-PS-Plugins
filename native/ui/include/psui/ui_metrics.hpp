#pragma once
#include <algorithm>
#include <cmath>
namespace psui {
inline float raster_font_pixels(float scale) {
  return std::round(14.f * scale);
}
struct RailGeometry {
  float left, right, center, y, radius, thickness;
};
// Match ImGui's floating-point SliderScalar grab travel. The hit rectangle
// remains full height; only the visual rail is thin.
inline RailGeometry rail_geometry(float x, float y, float width, float height,
                                  float scale, double value, double lo,
                                  double hi) {
  const float padding = 2.f,
              grab = std::min(12.f * scale, std::max(0.f, width - 2 * padding));
  const float left = x + padding + grab * .5f,
              right = std::max(left, x + width - padding - grab * .5f);
  const float fraction =
      hi <= lo
          ? 0.f
          : static_cast<float>(std::clamp((value - lo) / (hi - lo), 0., 1.));
  return {left,
          right,
          left + (right - left) * fraction,
          std::floor(y + height * .5f) + .5f,
          5.5f * scale,
          std::max(1.f, 2.f * scale)};
}
} // namespace psui
