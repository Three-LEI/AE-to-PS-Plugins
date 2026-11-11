// Scalar activation/rendering adapted from Dear ImGui 1.91.9b DragScalar.
// Copyright (c) 2014-2025 Omar Cornut. MIT license: see the bundled imgui/LICENSE.txt.
// Unlike stock DragScalar, Ctrl is reserved for fine scrubbing, and an ordinary
// click enters the text editor on release. No global keyboard state is altered.
#define IMGUI_DEFINE_MATH_OPERATORS
#include "psui/scrub_control.hpp"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>
#include <cstring>

static_assert(IMGUI_VERSION_NUM == 19191,
              "Review scalar activation when upgrading the pinned ImGui.");
namespace psui {
ImVec4 numeric_color(const ImVec4 &background) {
  const float brightness = .2126f * background.x + .7152f * background.y +
                           .0722f * background.z;
  if (brightness < .25f)
    return ImVec4(112 / 255.f, 190 / 255.f, 1, 1);
  if (brightness < .5f)
    return ImVec4(175 / 255.f, 235 / 255.f, 1, 1);
  return ImVec4(0, 50 / 255.f, 115 / 255.f, 1);
}

ScrubResult scrub_number(const char *label, double &value, double minimum,
                         double maximum, double units_per_pixel, float scale,
                         bool integer, const char *format) {
  using namespace ImGui;
  ScrubResult result;
  ImGuiWindow *window = GetCurrentWindow();
  if (window->SkipItems)
    return result;
  ImGuiContext &g = *GImGui;
  const ImGuiStyle &style = g.Style;
  const ImGuiID id = window->GetID(label);
  const ImRect box(window->DC.CursorPos,
                   window->DC.CursorPos +
                       ImVec2(CalcItemWidth(), GetFrameHeight()));
  ItemSize(box, style.FramePadding.y);
  if (!ItemAdd(box, id, &box, ImGuiItemFlags_Inputable))
    return result;
  const bool hovered = ItemHoverable(box, id, g.LastItemData.ItemFlags);
  bool editing = TempInputIsActive(id);
  if (!editing) {
    const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
    const bool double_clicked = hovered && g.IO.MouseClickedCount[0] == 2 &&
                                TestKeyOwner(ImGuiKey_MouseLeft, id);
    const bool activate = clicked || double_clicked || g.NavActivateId == id;
    if (activate && (clicked || double_clicked))
      SetKeyOwner(ImGuiKey_MouseLeft, id);
    if (activate) {
      std::memcpy(&g.ActiveIdValueOnActivation, &value, sizeof(value));
      editing = double_clicked ||
                (g.NavActivateId == id &&
                 (g.NavActivateFlags & ImGuiActivateFlags_PreferInput));
    }
    if (!editing && g.ActiveId == id && hovered && g.IO.MouseReleased[0] &&
        !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * .5f)) {
      g.NavActivateId = id;
      g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
      editing = true;
    }
    if (activate && !editing) {
      SetActiveID(id, window);
      SetFocusID(id, window);
      FocusWindow(window);
      g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }
  }
  // Reserve Escape so ImGui navigation and the outer filter do not close the
  // entire dialog when the user merely cancels an edit or a scrub gesture.
  if (g.ActiveId == id) {
    SetKeyOwner(ImGuiKey_Escape, id);
    if (IsKeyPressed(ImGuiKey_Escape, ImGuiInputFlags_None, id)) {
      double original;
      std::memcpy(&original, &g.ActiveIdValueOnActivation, sizeof(original));
      result.changed = value != original;
      value = original;
      if (result.changed)
        MarkItemEdited(id);
      ClearActiveID();
      editing = false;
      result.consumed_escape = true;
    }
  }
  const double before = value;
  PushStyleColor(ImGuiCol_Text, numeric_color(style.Colors[ImGuiCol_WindowBg]));
  if (editing) {
    result.changed |= TempInputScalar(box, id, label, ImGuiDataType_Double,
                                      &value, format, &minimum, &maximum);
    if (g.ActiveId == id)
      SetKeyOwner(ImGuiKey_Escape, id);
  } else {
    if (hovered || g.ActiveId == id)
      SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    const ImU32 bg = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive
                                   : hovered ? ImGuiCol_FrameBgHovered
                                             : ImGuiCol_FrameBg);
    RenderNavCursor(box, id);
    RenderFrame(box.Min, box.Max, bg, true, style.FrameRounding);
    const double modifier = (g.IO.KeyShift ? 10.0 : 1.0) *
                            (g.IO.KeyCtrl ? .1 : 1.0);
    const float speed = static_cast<float>(units_per_pixel * modifier /
                                          std::max(.25f, scale));
    // Fixed precision preserves sub-step accumulation during Ctrl scrubbing.
    const bool dragged = !result.consumed_escape &&
        DragBehavior(id, ImGuiDataType_Double, &value, speed, &minimum, &maximum,
                     integer ? "%.0f" : "%.3f",
                     ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoSpeedTweaks);
    result.changed |= dragged;
    if (dragged)
      MarkItemEdited(id);
    char buffer[64];
    const int length = DataTypeFormatString(buffer, IM_ARRAYSIZE(buffer),
                                            ImGuiDataType_Double, &value, format);
    RenderTextClipped(box.Min, box.Max, buffer, buffer + length, nullptr,
                       ImVec2(.5f, .5f));
  }
  PopStyleColor();
  if (result.changed) {
    if (!std::isfinite(value))
      value = std::isfinite(before) ? before : minimum;
    value = std::clamp(value, minimum, maximum);
    if (integer)
      value = std::round(value);
  }
  return result;
}
} // namespace psui
