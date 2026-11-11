#pragma once
#include "widgets.hpp"
namespace psui {
inline constexpr wchar_t scroll_panel_class[] = L"ZaYu.PSUI.ScrollPanel.1";
bool register_scroll_panel(HINSTANCE);
void release_scroll_panel(HINSTANCE);
void scroll_bind(HWND, const Theme *);
// Reflow document coordinates without losing keyboard focus or field values.
void scroll_begin(HWND);
void scroll_place(HWND, HWND child, const RECT &document_rect,
                  bool visible = true);
void scroll_end(HWND, int content_height);
void scroll_ensure_visible(HWND, HWND child);
int scroll_position(HWND);
int scroll_content_width(HWND);
void scroll_to(HWND, int position);
void draw_section(const DRAWITEMSTRUCT &, const Theme &, bool expanded);
} // namespace psui
