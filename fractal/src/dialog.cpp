#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "fractal_core.hpp"
#include "psui/combo.hpp"
#include "psui/paint.hpp"
#include "psui/numeric.hpp"
#include "psui/photoshop_host.hpp"
#include "psui/scroll_panel.hpp"
#include <algorithm>
#include <array>
#include <commctrl.h>
#include <vector>
namespace fractal {
namespace {
struct Number {
  const wchar_t *text;
  double Parameters::*member;
  double lo, hi, slider_lo, slider_hi;
};
constexpr Number nums[] = {
    {L"对比度 / Contrast", &Parameters::contrast, 0, 1000, 0, 1000},
    {L"亮度 / Brightness", &Parameters::brightness, -1000, 1000, -100, 100},
    {L"缩放 / Scale", &Parameters::scale, 1, 10000, 1, 600},
    {L"复杂度 / Complexity", &Parameters::complexity, 1, 20, 1, 20},
    {L"演化(°) / Evolution", &Parameters::evolution, -1000000, 1000000, 0,
     3600},
    {L"随机值 / Seed", nullptr, 0, 4294967295.0, 0, 100000},
    {L"不透明度 / Opacity", &Parameters::opacity, 0, 100, 0, 100},
    {L"旋转(°) / Rotation", &Parameters::rotation, -100000, 100000, -360, 360},
    {L"偏移 X / Offset X", &Parameters::offset_x, -2000000, 2000000, -2000,
     2000},
    {L"偏移 Y / Offset Y", &Parameters::offset_y, -2000000, 2000000, -2000,
     2000},
    {L"子影响(%) / Influence", &Parameters::sub_influence, 0, 100, 0, 100},
    {L"子缩放(%) / Subscale", &Parameters::sub_scale, 10, 100, 10, 100},
    {L"缩放宽度 / Width", &Parameters::scale_width, 1, 10000, 1, 600},
    {L"缩放高度 / Height", &Parameters::scale_height, 1, 10000, 1, 600},
    {L"子旋转(°) / Rotation", &Parameters::sub_rotation, -100000, 100000, -360,
     360},
    {L"子位移 X / Offset X", &Parameters::sub_offset_x, -2000000, 2000000,
     -1000, 1000},
    {L"子位移 Y / Offset Y", &Parameters::sub_offset_y, -2000000, 2000000,
     -1000, 1000},
    {L"循环圈数 / Cycles", &Parameters::cycles, 1, 10000, 1, 100}};
constexpr int number_count = static_cast<int>(std::size(nums));
constexpr const wchar_t *groups[] = {
    L"基本 / Basic", L"变换 / Transform", L"子设置 / Sub Settings",
    L"演化选项 / Evolution", L"合成 / Compositing"};
constexpr const wchar_t *checks[] = {
    L"反转 / Invert", L"统一缩放 / Uniform Scale",
    L"透视位移 / Perspective Offset", L"中心辅助比例 / Center Subscale",
    L"循环演化 / Cycle Evolution"};
constexpr bool Parameters::*bools[] = {
    &Parameters::invert, &Parameters::uniform_scale,
    &Parameters::perspective_offset, &Parameters::center_subscale,
    &Parameters::cycle_evolution};
constexpr const wchar_t *fractals[] = {L"基本 / Basic",
                                       L"湍流平滑 / Turbulent Smooth",
                                       L"湍流基本 / Turbulent Basic",
                                       L"湍流锐化 / Turbulent Sharp",
                                       L"动态 / Dynamic",
                                       L"动态渐进 / Dynamic Progressive",
                                       L"动态扭曲 / Dynamic Twist",
                                       L"最大 / Max",
                                       L"涂抹 / Smeary",
                                       L"旋涡 / Swirly",
                                       L"岩石 / Rocky",
                                       L"多云 / Cloudy",
                                       L"地形 / Terrain",
                                       L"子缩放 / Subscale",
                                       L"小凸起 / Small Bumps",
                                       L"绳状 / Strings",
                                       L"线状 / Threads"};
constexpr const wchar_t *noises[] = {L"块 / Block", L"线性 / Linear",
                                     L"柔和线性 / Soft Linear",
                                     L"样条 / Spline"};
constexpr const wchar_t *overflows[] = {
    L"剪切 / Clip", L"柔和限制 / Soft Clamp", L"折回 / Wrap Back",
    L"允许 HDR / Allow HDR Results"};
constexpr const wchar_t *blends[] = {L"正常 / Normal",
                                     L"无 / None",
                                     L"溶解 / Dissolve",
                                     L"动态溶解 / Dancing Dissolve",
                                     L"变暗 / Darken",
                                     L"正片叠底 / Multiply",
                                     L"颜色加深 / Color Burn",
                                     L"线性加深 / Linear Burn",
                                     L"深色 / Darker Color",
                                     L"变亮 / Lighten",
                                     L"滤色 / Screen",
                                     L"颜色减淡 / Color Dodge",
                                     L"相加 / Add",
                                     L"浅色 / Lighter Color",
                                     L"叠加 / Overlay",
                                     L"柔光 / Soft Light",
                                     L"强光 / Hard Light",
                                     L"亮光 / Vivid Light",
                                     L"线性光 / Linear Light",
                                     L"点光 / Pin Light",
                                     L"实色混合 / Hard Mix",
                                     L"差值 / Difference",
                                     L"排除 / Exclusion",
                                     L"相减 / Subtract",
                                     L"相除 / Divide",
                                     L"色相 / Hue",
                                     L"饱和度 / Saturation",
                                     L"颜色 / Color",
                                     L"明度 / Luminosity"};
constexpr const wchar_t *combo_labels[] = {
    L"分形类型 / Fractal", L"杂色类型 / Noise", L"溢出 / Overflow",
    L"混合模式 / Blend"};
enum class Kind { header, number, check, combo };
struct Row {
  Kind kind;
  int index, group;
};
constexpr Row rows[] = {
    {Kind::header, 0, 0},  {Kind::combo, 0, 0},   {Kind::combo, 1, 0},
    {Kind::check, 0, 0},   {Kind::number, 0, 0},  {Kind::number, 1, 0},
    {Kind::combo, 2, 0},   {Kind::number, 3, 0},  {Kind::header, 1, 1},
    {Kind::number, 7, 1},  {Kind::check, 1, 1},   {Kind::number, 2, 1},
    {Kind::number, 12, 1}, {Kind::number, 13, 1}, {Kind::number, 8, 1},
    {Kind::number, 9, 1},  {Kind::check, 2, 1},   {Kind::header, 2, 2},
    {Kind::number, 10, 2}, {Kind::number, 11, 2}, {Kind::number, 14, 2},
    {Kind::number, 15, 2}, {Kind::number, 16, 2}, {Kind::check, 3, 2},
    {Kind::header, 3, 3},  {Kind::number, 4, 3},  {Kind::check, 4, 3},
    {Kind::number, 17, 3}, {Kind::number, 5, 3},  {Kind::header, 4, 4},
    {Kind::number, 6, 4},  {Kind::combo, 3, 4}};
struct Context {
  Parameters p, defaults;
  FilterRecord *f;
  psui::Theme theme;
  psui::ProxyImage source;
  HWND panel = nullptr;
  HINSTANCE module = nullptr;
  std::vector<std::uint8_t> output, display;
  bool updating = false, pending = false, preview = true;
  std::array<bool, 5> expanded{true, false, false, false, true};
  OSErr error = noErr;
  Region region;
  Context(Parameters v, FilterRecord *host) : p(v), f(host) {
    defaults.offset_x = host->bigDocumentData
                            ? host->bigDocumentData->imageSize32.h / 2.0
                            : host->imageSize.h / 2.0;
    defaults.offset_y = host->bigDocumentData
                            ? host->bigDocumentData->imageSize32.v / 2.0
                            : host->imageSize.v / 2.0;
  }
};
RECT units(HWND w, int x, int y, int width, int height) {
  RECT r{x, y, x + width, y + height};
  MapDialogRect(w, &r);
  return r;
}
void status(HWND w, const wchar_t *text) { SetDlgItemTextW(w, 204, text); }
void queue(HWND w, Context &c) {
  if (!c.pending)
    c.pending = SetTimer(w, 2, 40, nullptr) != 0;
}
int combo_value(const Parameters &p, int i) {
  switch (i) {
  case 0:
    return static_cast<int>(p.fractal);
  case 1:
    return static_cast<int>(p.noise);
  case 2:
    return static_cast<int>(p.overflow);
  default:
    return static_cast<int>(p.blend);
  }
}
void combo_set(Parameters &p, int i, int value) {
  switch (i) {
  case 0:
    p.fractal = static_cast<FractalType>(value);
    break;
  case 1:
    p.noise = static_cast<NoiseType>(value);
    break;
  case 2:
    p.overflow = static_cast<Overflow>(value);
    break;
  default:
    p.blend = static_cast<Blend>(value);
  }
}
double number(const Parameters &p, int i) {
  return nums[i].member ? p.*nums[i].member : p.seed;
}
void assign(Parameters &p, int i, double v) {
  if (nums[i].member)
    p.*nums[i].member = v;
  else
    p.seed = static_cast<std::uint32_t>(v);
}
HWND add(Context &c, const wchar_t *cls, const wchar_t *text, int id,
         DWORD style) {
  auto h = CreateWindowExW(0, cls, text, WS_CHILD | style, 0, 0, 1, 1, c.panel,
                           reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                           c.module, nullptr);
  if (!h)
    throw std::bad_alloc();
  SendMessageW(h, WM_SETFONT,
               SendMessageW(GetParent(c.panel), WM_GETFONT, 0, 0), FALSE);
  return h;
}
void create(Context &c) {
  for (const auto &row : rows) {
    const auto i = row.index;
    switch (row.kind) {
    case Kind::header:
      add(c, L"BUTTON", groups[i], 6100 + i, BS_OWNERDRAW | WS_TABSTOP);
      break;
    case Kind::check:
      add(c, L"BUTTON", checks[i], 2000 + i, BS_OWNERDRAW | WS_TABSTOP);
      break;
    case Kind::combo: {
      add(c, L"STATIC", combo_labels[i], 1100 + i, SS_LEFT | SS_CENTERIMAGE);
      auto box = add(c, L"COMBOBOX", L"", 1000 + i,
                     CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS |
                         WS_VSCROLL | WS_TABSTOP);
      auto fill = [&](const auto &labels) {
        for (auto label : labels)
          SendMessageW(box, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
      };
      switch (i) {
      case 0:
        fill(fractals);
        break;
      case 1:
        fill(noises);
        break;
      case 2:
        fill(overflows);
        break;
      default:
        fill(blends);
      }
      SendMessageW(box, CB_SETMINVISIBLE, 14, 0);
      if (!psui::combo_bind(box, &c.theme))
        throw std::bad_alloc();
      break;
    }
    case Kind::number:
      add(c, L"STATIC", nums[i].text, 600 + i, SS_LEFT | SS_CENTERIMAGE);
      add(c, L"STATIC", L"", 500 + i, SS_OWNERDRAW);
      SendMessageW(
          add(c, L"EDIT", L"", 300 + i, ES_RIGHT | ES_AUTOHSCROLL | WS_TABSTOP),
          EM_SETLIMITTEXT, 24, 0);
      psui::slider_bind(
          add(c, psui::slider_class, nums[i].text, 400 + i, WS_TABSTOP),
          &c.theme);
      break;
    }
  }
}
void reflow(HWND w, Context &c) {
  RECT client;
  GetClientRect(w, &client);
  const auto origin = units(w, 126, 8, 0, 0), bottom = units(w, 0, 0, 8, 18);
  SetWindowPos(c.panel, nullptr, origin.left, origin.top,
               std::max(1L, client.right - origin.left - bottom.right),
               std::max(1L, client.bottom - origin.top - bottom.bottom),
               SWP_NOZORDER | SWP_NOACTIVATE);
  RECT panel;
  GetClientRect(c.panel, &panel);
  const auto base = units(w, 0, 0, 286, 0);
  const int extra = psui::scroll_content_width(c.panel) - base.right;
  int y = 0;
  psui::scroll_begin(c.panel);
  auto place = [&](int id, int x, int top, int width, int height, bool show,
                   bool stretch = false) {
    auto r = units(w, x, top, width, height);
    if (stretch)
      r.right += extra;
    psui::scroll_place(c.panel, GetDlgItem(c.panel, id), r, show);
  };
  for (const auto &row : rows) {
    const int i = row.index;
    const bool show = row.kind == Kind::header || c.expanded[row.group];
    switch (row.kind) {
    case Kind::header:
      place(6100 + i, 0, y, 286, 16, true, true);
      y += 18;
      break;
    case Kind::check:
      place(2000 + i, 8, y, 278, 16, show, true);
      if (show)
        y += 17;
      break;
    case Kind::combo:
      place(1100 + i, 8, y, 96, 17, show);
      place(1000 + i, 106, y, 178, 180, show, true);
      if (show)
        y += 20;
      break;
    case Kind::number:
      place(600 + i, 8, y, 96, 15, show);
      place(500 + i, 106, y, 58, 15, show);
      place(300 + i, 109, y + 2, 52, 11, show);
      place(400 + i, 168, y, 116, 15, show, true);
      if (show)
        y += 17;
      break;
    }
  }
  psui::scroll_end(c.panel, units(w, 0, 0, 0, y + 3).bottom);
  for (int i = 0; i < 3; ++i) {
    const auto r = units(w, 8, 0, 110, 18);
    const auto gap = units(w, 0, 0, 0, 22);
    SetWindowPos(GetDlgItem(w, i == 0   ? IDOK
                               : i == 1 ? IDCANCEL
                                        : 209),
                 nullptr, r.left,
                 client.bottom - bottom.bottom - (3 - i) * gap.bottom,
                 r.right - r.left, r.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
  }
  auto label = units(w, 8, 0, 0, 12);
  SetWindowPos(GetDlgItem(w, 204), nullptr, label.left,
               client.bottom - label.bottom - 2, client.right - 2 * label.left,
               label.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
}
void enable(Context &c) {
  for (int i : {2, 12, 13, 17}) {
    const bool on = i == 2    ? c.p.uniform_scale
                    : i == 17 ? c.p.cycle_evolution
                              : !c.p.uniform_scale;
    EnableWindow(GetDlgItem(c.panel, 300 + i), on);
    EnableWindow(GetDlgItem(c.panel, 400 + i), on);
  }
}
void sliders(Context &c) {
  for (int i = 0; i < number_count; ++i)
    psui::slider_set(
        GetDlgItem(c.panel, 400 + i),
        static_cast<int>(std::clamp((number(c.p, i) - nums[i].slider_lo) /
                                        (nums[i].slider_hi - nums[i].slider_lo),
                                    0.0, 1.0) *
                             1000 +
                         .5));
}
void fields(HWND w, Context &c) {
  c.updating = true;
  for (int i = 0; i < number_count; ++i)
    psui::write_number(c.panel, 300 + i, number(c.p, i));
  for (int i = 0; i < 4; ++i)
    SendDlgItemMessageW(c.panel, 1000 + i, CB_SETCURSEL, combo_value(c.p, i),
                        0);
  for (int i = 0; i < 5; ++i)
    InvalidateRect(GetDlgItem(c.panel, 2000 + i), nullptr, FALSE);
  sliders(c);
  enable(c);
  c.updating = false;
  queue(w, c);
}
bool read(HWND w, Context &c, bool focus = false) {
  auto next = c.p;
  for (int i = 0; i < number_count; ++i) {
    double n;
    if (!psui::read_number(c.panel, 300 + i, nums[i].lo, nums[i].hi, n) ||
        ((i == 5 || i == 17) && n != std::floor(n))) {
      if (focus) {
        for (const auto &row : rows)
          if (row.kind == Kind::number && row.index == i)
            c.expanded[row.group] = true;
        reflow(w, c);
        const auto field = GetDlgItem(c.panel, 300 + i);
        psui::scroll_ensure_visible(c.panel, field);
        SetFocus(field);
        status(w, L"请检查数值范围；种子和圈数须为整数 / Check value range.");
      }
      return false;
    }
    assign(next, i, n);
  }
  c.p = next;
  return true;
}
void preview(HWND w, Context &c) {
  if (c.source.pixels.empty())
    return;
  auto &s = c.source;
  if (c.preview &&
      !process(s.pixels.data(), s.pixels.size(), s.width * s.channels,
               c.output.data(), c.output.size(), s.width * s.channels, s.width,
               s.height, s.channels, c.p, c.region))
    return;
  for (int y = 0; y < s.height; ++y)
    for (int x = 0; x < s.width; ++x) {
      const auto i = static_cast<std::size_t>(y * s.width + x),
                 p = i * s.channels;
      const int a = s.channels == 4 ? s.pixels[p + 3] : 255,
                m = s.mask.empty() ? 255 : s.mask[i],
                bg = ((x / 8 + y / 8) & 1) ? 156 : 200;
      for (int k = 0; k < 3; ++k) {
        int v =
            c.preview
                ? (s.pixels[p + k] * (255 - m) + c.output[p + k] * m + 127) /
                      255
                : s.pixels[p + k];
        c.display[i * 3 + k] =
            static_cast<std::uint8_t>((v * a + bg * (255 - a) + 127) / 255);
      }
    }
  InvalidateRect(GetDlgItem(w, 210), nullptr, FALSE);
}
INT_PTR handle(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  auto *c = reinterpret_cast<Context *>(GetWindowLongPtrW(w, DWLP_USER));
  if (msg == WM_INITDIALOG) {
    c = reinterpret_cast<Context *>(lp);
    SetWindowLongPtrW(w, DWLP_USER, lp);
    c->panel = GetDlgItem(w, 220);
    psui::scroll_bind(c->panel, &c->theme);
    create(*c);
    c->theme.apply(w, psui::host_brightness(c->f), true);
    fields(w, *c);
    reflow(w, *c);
    RECT box;
    GetClientRect(GetDlgItem(w, 210), &box);
    const int side = std::clamp(
        static_cast<int>(std::min(box.right, box.bottom)) - 6, 16, 512);
    const auto r = c->f->bigDocumentData
                       ? c->f->bigDocumentData->filterRect32
                       : VRect{c->f->filterRect.top, c->f->filterRect.left,
                               c->f->filterRect.bottom, c->f->filterRect.right};
    const int scale = std::max(
        1, (std::max(r.right - r.left, r.bottom - r.top) + side - 1) / side);
    c->region = {std::floor(static_cast<double>(r.left) / scale) * scale,
                 std::floor(static_cast<double>(r.top) / scale) * scale,
                 static_cast<double>(scale)};
    c->error = psui::host_read_proxy(c->f, side, c->source);
    if (c->error != noErr) {
      EndDialog(w, IDCANCEL);
      return TRUE;
    }
    c->output.resize(c->source.pixels.size());
    c->display.resize(static_cast<std::size_t>(c->source.width) *
                      c->source.height * 3);
    EnableWindow(GetDlgItem(w, 217), !c->source.pixels.empty());
    preview(w, *c);
    psui::center_on_owner(w, GetWindow(w, GW_OWNER));
    SetTimer(w, 1, 750, nullptr);
    return TRUE;
  }
  if (!c)
    return FALSE;
  if (msg == WM_SIZE && c->panel) {
    reflow(w, *c);
    return TRUE;
  }
  if (msg == WM_GETMINMAXINFO) {
    auto *info = reinterpret_cast<MINMAXINFO *>(lp);
    const auto r = units(w, 0, 0, 432, 268);
    RECT outer = r;
    AdjustWindowRectEx(
        &outer, static_cast<DWORD>(GetWindowLongPtrW(w, GWL_STYLE)), FALSE,
        static_cast<DWORD>(GetWindowLongPtrW(w, GWL_EXSTYLE)));
    info->ptMinTrackSize = {outer.right - outer.left, outer.bottom - outer.top};
    return TRUE;
  }
  if (msg == WM_MOUSEWHEEL) {
    SendMessageW(c->panel, msg, wp, lp);
    return TRUE;
  }
  if (msg == WM_TIMER) {
    if (wp == 1)
      c->theme.apply(w, psui::host_brightness(c->f));
    if (wp == 2) {
      KillTimer(w, 2);
      c->pending = false;
      preview(w, *c);
    }
    return TRUE;
  }
  if (msg == WM_DESTROY) {
    KillTimer(w, 1);
    KillTimer(w, 2);
    return TRUE;
  }
  if (msg == WM_ERASEBKGND) {
    RECT r;
    GetClientRect(w, &r);
    FillRect(reinterpret_cast<HDC>(wp), &r, c->theme.background());
    return TRUE;
  }
  if (msg == WM_CTLCOLORDLG || msg == WM_CTLCOLORSTATIC ||
      msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX || msg == WM_CTLCOLORBTN)
    return c->theme.control_color(
        msg, wp, GetDlgCtrlID(reinterpret_cast<HWND>(lp)) == 204);
  if (msg == WM_MEASUREITEM) {
    auto *d = reinterpret_cast<MEASUREITEMSTRUCT *>(lp);
    d->itemHeight = units(w, 0, 0, 0, 14).bottom;
    return TRUE;
  }
  if (msg == WM_DRAWITEM) {
    psui::buffered_owner_draw(*reinterpret_cast<DRAWITEMSTRUCT *>(lp),c->theme,[&](const DRAWITEMSTRUCT& d){
    if (d.CtlID >= 500 && d.CtlID < 500 + number_count)
      psui::draw_numeric_frame(d, c->theme);
    else if (d.CtlID >= 6100 && d.CtlID < 6105)
      psui::draw_section(d, c->theme, c->expanded[d.CtlID - 6100]);
    else if (d.CtlType == ODT_COMBOBOX) {
      if (d.itemState & ODS_SELECTED) {
        auto brush = CreateSolidBrush(c->theme.colors().button);
        FillRect(d.hDC, &d.rcItem, brush);
        DeleteObject(brush);
      } else
        FillRect(d.hDC, &d.rcItem, c->theme.field());
      if (d.itemID != static_cast<UINT>(-1)) {
        wchar_t label[128];
        SendMessageW(d.hwndItem, CB_GETLBTEXT, d.itemID,
                     reinterpret_cast<LPARAM>(label));
        auto text = d.rcItem;
        text.left += 4;
        SetTextColor(d.hDC, c->theme.colors().text);
        SetBkMode(d.hDC, TRANSPARENT);
        DrawTextW(d.hDC, label, -1, &text,
                  DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
      }
      return;
    } else if (d.CtlID == 210) {
      if (c->display.empty())
        psui::draw_rgb(d.hDC, d.rcItem, c->theme, nullptr, 0, 0);
      else {
        FillRect(d.hDC, &d.rcItem, c->theme.background());
        if (!psui::host_display_rgb(c->f, d.hDC, d.rcItem, c->display.data(),
                                    c->source.width, c->source.height))
          psui::draw_rgb(d.hDC, d.rcItem, c->theme, c->display.data(),
                         c->source.width, c->source.height);
      }
    } else if (d.CtlID == 217 || (d.CtlID >= 2000 && d.CtlID < 2005))
      psui::draw_checkbox(d, c->theme,
                          d.CtlID == 217 ? c->preview
                                         : c->p.*bools[d.CtlID - 2000]);
    else
      psui::draw_button(d, c->theme);
    });
    return TRUE;
  }
  if (msg == WM_HSCROLL) {
    const int i = GetDlgCtrlID(reinterpret_cast<HWND>(lp)) - 400;
    if (i >= 0 && i < number_count) {
      double value = nums[i].slider_lo +
                     psui::slider_get(reinterpret_cast<HWND>(lp)) *
                         (nums[i].slider_hi - nums[i].slider_lo) / 1000;
      if (i == 5 || i == 17)
        value = std::round(value);
      assign(c->p, i, value);
      c->updating = true;
      psui::write_number(c->panel, 300 + i, value);
      c->updating = false;
      status(w, L"");
      queue(w, *c);
    }
    return TRUE;
  }
  if (msg == WM_COMMAND) {
    const int id = LOWORD(wp), code = HIWORD(wp);
    if (id == IDCANCEL) {
      EndDialog(w, IDCANCEL);
      return TRUE;
    }
    if (code == EN_SETFOCUS || code == CBN_SETFOCUS) {
      psui::scroll_ensure_visible(c->panel, reinterpret_cast<HWND>(lp));
      return TRUE;
    }
    if (id >= 6100 && id < 6105) {
      c->expanded[id - 6100] = !c->expanded[id - 6100];
      reflow(w, *c);
      psui::scroll_ensure_visible(c->panel, GetDlgItem(c->panel, id));
      return TRUE;
    }
    if (id == 217) {
      c->preview = !c->preview;
      InvalidateRect(GetDlgItem(w, id), nullptr, FALSE);
      queue(w, *c);
      return TRUE;
    }
    if (id >= 2000 && id < 2005) {
      c->p.*bools[id - 2000] = !(c->p.*bools[id - 2000]);
      enable(*c);
      InvalidateRect(GetDlgItem(c->panel, id), nullptr, FALSE);
      queue(w, *c);
      return TRUE;
    }
    if (id >= 1000 && id < 1004 && code == CBN_SELCHANGE) {
      InvalidateRect(GetDlgItem(c->panel, id), nullptr, FALSE);
      combo_set(c->p, id - 1000,
                static_cast<int>(
                    SendDlgItemMessageW(c->panel, id, CB_GETCURSEL, 0, 0)));
      queue(w, *c);
      return TRUE;
    }
    if (id == 209) {
      c->p = c->defaults;
      fields(w, *c);
      status(w, L"");
      return TRUE;
    }
    if (code == EN_CHANGE && !c->updating && id >= 300 &&
        id < 300 + number_count) {
      if (read(w, *c)) {
        sliders(*c);
        status(w, L"");
        queue(w, *c);
      }
      return TRUE;
    }
    if (id == IDOK) {
      if (read(w, *c, true) && valid(c->p))
        EndDialog(w, IDOK);
      return TRUE;
    }
  }
  return FALSE;
}
INT_PTR CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  try {
    return handle(w, msg, wp, lp);
  } catch (...) {
    auto *c = reinterpret_cast<Context *>(GetWindowLongPtrW(w, DWLP_USER));
    if (c)
      c->error = memFullErr;
    EndDialog(w, IDCANCEL);
    return TRUE;
  }
}
} // namespace
INT_PTR show_dialog(HINSTANCE m, HWND owner, FilterRecord *f, Parameters &p) {
  if (!psui::register_widgets(m))
    return -1;
  Context c(p, f);
  c.module = m;
  const auto result = DialogBoxParamW(m, MAKEINTRESOURCEW(101), owner, proc,
                                      reinterpret_cast<LPARAM>(&c));
  psui::release_widgets(m);
  if (c.error != noErr)
    return c.error;
  if (result == IDOK)
    p = c.p;
  return result;
}
} // namespace fractal
