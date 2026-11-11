#pragma once
#include "widgets.hpp"
namespace psui {
// Retains native combo keyboard and popup behavior, paints closed field in PS
// colors. Call once after creating a CBS_DROPDOWNLIST control. Theme must
// outlive it.
bool combo_bind(HWND, const Theme *);
} // namespace psui
