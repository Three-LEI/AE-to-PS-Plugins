#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#include "psui/scroll_panel.hpp"
#include <algorithm>
#include <new>
#include <vector>
#include <windowsx.h>
namespace psui {
namespace {
constexpr UINT bind = WM_USER + 200, begin = WM_USER + 201,
               place = WM_USER + 202, end = WM_USER + 203,
               ensure = WM_USER + 204, position = WM_USER + 205,
               go = WM_USER + 206, content_width = WM_USER + 207;
struct Child {
  HWND w;
  RECT r;
  bool visible;
};
struct Panel {
  const Theme *theme = nullptr;
  std::vector<Child> children;
  int height = 0, offset = 0, wheel = 0, drag_y = 0, drag_offset = 0;
  bool dragging = false;
};
int strip(HWND w) { return MulDiv(12, GetDpiForWindow(w), 96); }
RECT thumb(HWND w, Panel &p) {
  RECT r;
  GetClientRect(w, &r);
  const int size =
      p.height > r.bottom
          ? std::min<int>(r.bottom, std::max(MulDiv(24, GetDpiForWindow(w), 96),
                                             static_cast<int>(
                                                 static_cast<double>(r.bottom) *
                                                 r.bottom / p.height)))
          : r.bottom;
  const int top =
      p.height > r.bottom
          ? static_cast<int>(static_cast<double>(p.offset) * (r.bottom - size) /
                             (p.height - r.bottom))
          : 0;
  return {r.right - strip(w) + strip(w) / 3, top, r.right - strip(w) / 3,
          top + size};
}
void paint(HWND w, Panel &p, HDC dc) {
  RECT r;
  GetClientRect(w, &r);
  const auto color = p.theme ? p.theme->colors() : palette(3);
  auto brush = CreateSolidBrush(color.bg);
  FillRect(dc, &r, brush);
  DeleteObject(brush);
  if (p.height <= r.bottom)
    return;
  r = thumb(w, p);
  brush = CreateSolidBrush(p.dragging ? color.text : color.border);
  auto old = SelectObject(dc, brush);
  auto pen = SelectObject(dc, GetStockObject(NULL_PEN));
  RoundRect(dc, r.left, r.top + 2, r.right, r.bottom - 2, r.right - r.left,
            r.right - r.left);
  SelectObject(dc, pen);
  SelectObject(dc, old);
  DeleteObject(brush);
}
void layout(HWND w, Panel &p) {
  RECT r;
  GetClientRect(w, &r);
  p.offset = std::clamp(p.offset, 0,
                        std::max(0, p.height - static_cast<int>(r.bottom)));
  // Move the child hierarchy as one layout transaction. Focus notifications
  // must not invoke this path when the requested field is already visible.
  auto batch=BeginDeferWindowPos(static_cast<int>(p.children.size()));
  for (auto &c : p.children){
    const auto flags=SWP_NOZORDER|SWP_NOACTIVATE|(c.visible?SWP_SHOWWINDOW:SWP_HIDEWINDOW);
    if(batch)batch=DeferWindowPos(batch,c.w,nullptr,c.r.left,c.r.top-p.offset,c.r.right-c.r.left,c.r.bottom-c.r.top,flags);
    else SetWindowPos(c.w,nullptr,c.r.left,c.r.top-p.offset,c.r.right-c.r.left,c.r.bottom-c.r.top,flags);
  }
  if(batch)EndDeferWindowPos(batch);
  RedrawWindow(w, nullptr, nullptr,
               RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}
LRESULT CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  auto *p = reinterpret_cast<Panel *>(GetWindowLongPtrW(w, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    p = new (std::nothrow) Panel;
    if (!p)
      return FALSE;
    SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
    return TRUE;
  }
  if (!p)
    return DefWindowProcW(w, msg, wp, lp);
  switch (msg) {
  case WM_NCDESTROY:
    delete p;
    SetWindowLongPtrW(w, GWLP_USERDATA, 0);
    return DefWindowProcW(w, msg, wp, lp);
  case bind:
    p->theme = reinterpret_cast<const Theme *>(lp);
    return 0;
  case begin:
    p->children.clear();
    return 0;
  case place:
    try {
      p->children.push_back(*reinterpret_cast<const Child *>(lp));
    } catch (...) {
      return FALSE;
    }
    return TRUE;
  case end:
    p->height = static_cast<int>(wp);
    layout(w, *p);
    return 0;
  case position:
    return p->offset;
  case content_width: {
    RECT r;
    GetClientRect(w, &r);
    return std::max(0L, r.right - strip(w));
  }
  case go:{
    RECT r;GetClientRect(w,&r);const auto next=std::clamp(static_cast<int>(wp),0,std::max(0,p->height-static_cast<int>(r.bottom)));
    if(next!=p->offset){p->offset=next;layout(w,*p);}
    return 0;
  }
  case ensure: {
    RECT viewport;
    GetClientRect(w, &viewport);
    for (auto &c : p->children)
      if (c.w == reinterpret_cast<HWND>(lp) && c.visible) {
        const int previous=p->offset;
        RECT actual;
        GetWindowRect(c.w, &actual);
        const int bottom = c.r.top + actual.bottom - actual.top;
        if (c.r.top < p->offset)
          p->offset = c.r.top;
        else if (bottom > p->offset + viewport.bottom)
          p->offset = bottom - viewport.bottom;
        if(p->offset!=previous)layout(w, *p);
        break;
      }
    return 0;
  }
  case WM_SIZE:
    layout(w, *p);
    return 0;
  case WM_VSCROLL: {
    RECT r;
    GetClientRect(w, &r);
    int n = p->offset;
    switch (LOWORD(wp)) {
    case SB_TOP:
      n = 0;
      break;
    case SB_BOTTOM:
      n = p->height;
      break;
    case SB_LINEUP:
      n -= 24;
      break;
    case SB_LINEDOWN:
      n += 24;
      break;
    case SB_PAGEUP:
      n -= r.bottom;
      break;
    case SB_PAGEDOWN:
      n += r.bottom;
      break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
      n = HIWORD(wp);
      break;
    default:
      return 0;
    }
    p->offset = n;
    layout(w, *p);
    return 0;
  }
  case WM_MOUSEWHEEL: {
    UINT lines = 3;
    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    p->wheel += GET_WHEEL_DELTA_WPARAM(wp);
    const int steps = p->wheel / WHEEL_DELTA;
    p->wheel %= WHEEL_DELTA;
    if (lines == WHEEL_PAGESCROLL) {
      RECT r;
      GetClientRect(w, &r);
      p->offset -= steps * r.bottom;
    } else
      p->offset -= steps * static_cast<int>(lines) * 24;
    layout(w, *p);
    return 0;
  }
  case WM_LBUTTONDOWN: {
    RECT r;
    GetClientRect(w, &r);
    if (GET_X_LPARAM(lp) >= r.right - strip(w) && p->height > r.bottom) {
      const auto bar = thumb(w, *p);
      if (GET_Y_LPARAM(lp) >= bar.top && GET_Y_LPARAM(lp) < bar.bottom) {
        p->dragging = true;
        p->drag_y = GET_Y_LPARAM(lp);
        p->drag_offset = p->offset;
        SetCapture(w);
      } else {
        p->offset += GET_Y_LPARAM(lp) < bar.top ? -r.bottom : r.bottom;
        layout(w, *p);
      }
      InvalidateRect(w, nullptr, FALSE);
    }
    return 0;
  }
  case WM_MOUSEMOVE:
    if (p->dragging) {
      RECT r;
      GetClientRect(w, &r);
      const auto bar = thumb(w, *p);
      p->offset =
          p->drag_offset +
          static_cast<int>(static_cast<double>(GET_Y_LPARAM(lp) - p->drag_y) *
                           (p->height - r.bottom) /
                           std::max(1L, r.bottom - (bar.bottom - bar.top)));
      layout(w, *p);
    }
    return 0;
  case WM_LBUTTONUP:
    if (p->dragging) {
      p->dragging = false;
      ReleaseCapture();
      InvalidateRect(w, nullptr, FALSE);
    }
    return 0;
  case WM_CAPTURECHANGED:
    p->dragging = false;
    InvalidateRect(w, nullptr, FALSE);
    return 0;
  case WM_ERASEBKGND:
    paint(w, *p, reinterpret_cast<HDC>(wp));
    return 1;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    auto dc = BeginPaint(w, &ps);
    paint(w, *p, dc);
    EndPaint(w, &ps);
    return 0;
  }
  case WM_PRINTCLIENT:
    paint(w, *p, reinterpret_cast<HDC>(wp));
    return 0;
  case WM_COMMAND:
  case WM_HSCROLL:
  case WM_DRAWITEM:
  case WM_MEASUREITEM:
  case WM_CTLCOLORSTATIC:
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORBTN:
    return SendMessageW(GetParent(w), msg, wp, lp);
  default:
    return DefWindowProcW(w, msg, wp, lp);
  }
}
} // namespace
bool register_scroll_panel(HINSTANCE m) {
  WNDCLASSW old{};
  if (GetClassInfoW(m, scroll_panel_class, &old))
    return true;
  WNDCLASSW c{};
  c.hInstance = m;
  c.lpfnWndProc = proc;
  c.lpszClassName = scroll_panel_class;
  c.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  return RegisterClassW(&c) != 0;
}
void release_scroll_panel(HINSTANCE m) {
  UnregisterClassW(scroll_panel_class, m);
}
void scroll_bind(HWND w, const Theme *t) {
  SendMessageW(w, bind, 0, reinterpret_cast<LPARAM>(t));
}
void scroll_begin(HWND w) { SendMessageW(w, begin, 0, 0); }
void scroll_place(HWND w, HWND child, const RECT &r, bool visible) {
  const Child c{child, r, visible};
  if (!SendMessageW(w, place, 0, reinterpret_cast<LPARAM>(&c)))
    throw std::bad_alloc();
}
void scroll_end(HWND w, int height) { SendMessageW(w, end, height, 0); }
void scroll_ensure_visible(HWND w, HWND child) {
  SendMessageW(w, ensure, 0, reinterpret_cast<LPARAM>(child));
}
int scroll_position(HWND w) {
  return static_cast<int>(SendMessageW(w, position, 0, 0));
}
int scroll_content_width(HWND w) {
  return static_cast<int>(SendMessageW(w, content_width, 0, 0));
}
void scroll_to(HWND w, int pos) { SendMessageW(w, go, pos, 0); }
void draw_section(const DRAWITEMSTRUCT &d, const Theme &t, bool expanded) {
  FillRect(d.hDC, &d.rcItem, t.background());
  const auto color = t.colors().text;
  const auto pen = CreatePen(PS_SOLID, 1, t.colors().track);
  auto old = SelectObject(d.hDC, pen);
  MoveToEx(d.hDC, d.rcItem.left, d.rcItem.bottom - 1, nullptr);
  LineTo(d.hDC, d.rcItem.right, d.rcItem.bottom - 1);
  SelectObject(d.hDC, old);
  DeleteObject(pen);
  const int x = d.rcItem.left + 5, y = (d.rcItem.top + d.rcItem.bottom) / 2;
  POINT points[3];
  if (expanded) {
    points[0] = {x, y - 2};
    points[1] = {x + 8, y - 2};
    points[2] = {x + 4, y + 3};
  } else {
    points[0] = {x + 1, y - 4};
    points[1] = {x + 1, y + 4};
    points[2] = {x + 6, y};
  }
  const auto brush = CreateSolidBrush(color);
  old = SelectObject(d.hDC, brush);
  const auto oldpen = SelectObject(d.hDC, GetStockObject(NULL_PEN));
  Polygon(d.hDC, points, 3);
  SelectObject(d.hDC, oldpen);
  SelectObject(d.hDC, old);
  DeleteObject(brush);
  RECT text = d.rcItem;
  text.left += 20;
  wchar_t label[128];
  GetWindowTextW(d.hwndItem, label, 128);
  const auto font = SelectObject(d.hDC, reinterpret_cast<HFONT>(SendMessageW(
                                            d.hwndItem, WM_GETFONT, 0, 0)));
  SetTextColor(d.hDC, color);
  SetBkMode(d.hDC, TRANSPARENT);
  DrawTextW(d.hDC, label, -1, &text,
            DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
  SelectObject(d.hDC, font);
}
} // namespace psui
