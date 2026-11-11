#pragma once
#include "imgui.h"
namespace psui {
struct ScrubResult {
  bool changed = false;
  bool consumed_escape = false;
};
// Shared by scalar parameters, point coordinates and preview zoom. Speeds are
// units per logical pixel, independent of the host's DPI. Click edits; drag
// scrubs; Shift multiplies speed by ten and Ctrl divides it by ten.
ImVec4 numeric_color(const ImVec4 &background);
ScrubResult scrub_number(const char *id, double &value, double minimum,
                         double maximum, double units_per_pixel, float scale,
                         bool integer = false, const char *format = "%.8g");
} // namespace psui
