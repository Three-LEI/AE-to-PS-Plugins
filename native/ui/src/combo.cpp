#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include "psui/combo.hpp"
#include "psui/paint.hpp"
#include "psui/win_compat.hpp"
#include <new>
namespace psui {
namespace {
constexpr auto key = L"ZaYu.PSUI.Combo.State";
struct State {
  WNDPROC original;
  const Theme *theme;
};
void paint(HWND w, const Theme &t, HDC dc) {
  RECT r;
  GetClientRect(w, &r);
  FillRect(dc, &r, t.field());
  auto brush = CreateSolidBrush(t.colors().border);
  FrameRect(dc, &r, brush);
  DeleteObject(brush);
  const int arrow = MulDiv(22, wincompat::window_dpi(w), 96), x = r.right - arrow / 2,
            y = r.bottom / 2;
  POINT points[] = {{x - 4, y - 2}, {x + 4, y - 2}, {x, y + 3}};
  brush = CreateSolidBrush(IsWindowEnabled(w) ? t.colors().text
                                              : t.colors().border);
  auto old = SelectObject(dc, brush);
  auto pen = SelectObject(dc, GetStockObject(NULL_PEN));
  Polygon(dc, points, 3);
  SelectObject(dc, pen);
  SelectObject(dc, old);
  DeleteObject(brush);
  auto font = SelectObject(
      dc, reinterpret_cast<HFONT>(SendMessageW(w, WM_GETFONT, 0, 0)));
  wchar_t text[256];
  GetWindowTextW(w, text, 256);
  r.left += 6;
  r.right -= arrow;
  SetBkMode(dc, TRANSPARENT);
  SetTextColor(dc, IsWindowEnabled(w) ? t.colors().text : t.colors().border);
  DrawTextW(dc, text, -1, &r, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
  SelectObject(dc, font);
}
LRESULT CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  auto *s = static_cast<State *>(GetPropW(w, key));
  if (!s)
    return DefWindowProcW(w, msg, wp, lp);
  auto original = s->original;
  if (msg == WM_NCDESTROY) {
    RemovePropW(w, key);
    SetWindowLongPtrW(w, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
    delete s;
    return CallWindowProcW(original, w, msg, wp, lp);
  }
  if (msg == WM_ERASEBKGND)
    return 1;
  if (msg == WM_PAINT) {
    PAINTSTRUCT ps;
    auto dc = BeginPaint(w, &ps);
    RECT rect;GetClientRect(w,&rect);buffered_paint(dc,rect,*s->theme,[&](HDC memory){paint(w,*s->theme,memory);});
    EndPaint(w, &ps);
    return 0;
  }
  if (msg == WM_PRINTCLIENT || (msg == WM_PRINT && (lp & PRF_CLIENT))) {
    RECT rect;GetClientRect(w,&rect);buffered_paint(reinterpret_cast<HDC>(wp),rect,*s->theme,[&](HDC memory){paint(w,*s->theme,memory);});
    return 0;
  }
  const auto result = CallWindowProcW(original, w, msg, wp, lp);
  if (msg == CB_SETCURSEL || msg == WM_SETFOCUS || msg == WM_KILLFOCUS ||
      msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP || msg == WM_KEYDOWN ||
      msg == WM_ENABLE)
    InvalidateRect(w, nullptr, FALSE);
  return result;
}
} // namespace
bool combo_bind(HWND w, const Theme *theme) {
  if (GetPropW(w, key))
    return true;
  auto *s = new (std::nothrow) State{
      reinterpret_cast<WNDPROC>(GetWindowLongPtrW(w, GWLP_WNDPROC)), theme};
  if (!s)
    return false;
  if (!SetPropW(w, key, s)) {
    delete s;
    return false;
  }
  SetWindowLongPtrW(w, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(proc));
  InvalidateRect(w, nullptr, FALSE);
  return true;
}
} // namespace psui
