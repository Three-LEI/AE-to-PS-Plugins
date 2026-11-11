#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/library_panel.hpp"
#include "psui/scrub_control.hpp"
#include "psui/color_sampling.hpp"
#include "psui/icons.hpp"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "psui/ui_metrics.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <d3d11.h>
#include <dxgi.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <shellapi.h>
#include <vector>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM,
                                                             LPARAM);
namespace psui {
namespace {
thread_local PanelTestHook *test_hook = nullptr;
bool rail_slider(double &value, double lo, double hi, float scale) {
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
  ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12 * scale);
  for (auto c :
       {ImGuiCol_FrameBg, ImGuiCol_FrameBgHovered, ImGuiCol_FrameBgActive,
        ImGuiCol_SliderGrab, ImGuiCol_SliderGrabActive})
    ImGui::PushStyleColor(c, {0, 0, 0, 0});
  const bool changed = ImGui::SliderScalar(
      "##slider", ImGuiDataType_Double, &value, &lo, &hi, "",
      ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoInput);
  ImGui::PopStyleColor(5);
  ImGui::PopStyleVar(2);
  const auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
  const auto rail =
      rail_geometry(a.x, a.y, b.x - a.x, b.y - a.y, scale, value, lo, hi);
  const bool hot = ImGui::IsItemHovered() || ImGui::IsItemActive();
  auto *list = ImGui::GetWindowDrawList();
  list->AddLine({rail.left, rail.y}, {rail.right, rail.y},
                ImGui::GetColorU32(ImGuiCol_Border), rail.thickness);
  list->AddCircleFilled(
      {rail.center, rail.y}, rail.radius,
      ImGui::GetColorU32(hot ? ImGuiCol_ButtonHovered : ImGuiCol_Button), 24);
  list->AddCircle({rail.center, rail.y}, rail.radius,
                  ImGui::GetColorU32(ImGuiCol_Text), 24, std::max(1.f, scale));
  return changed;
}
bool open_source_repository(HWND owner) {
  if (test_hook && test_hook->open_url)
    return test_hook->open_url(source_repository);
  const std::string url(source_repository);
  const std::wstring wide(url.begin(), url.end());
  return reinterpret_cast<INT_PTR>(ShellExecuteW(owner, L"open", wide.c_str(),
                                                nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}
void about_content(LibraryPanel &ui, const PluginInfo &info) {
  const auto content = about_text(info, ui.language(), false);
  ImGui::TextWrapped("%s", content.c_str());
  ui.mark("about.content");
  ImGui::Spacing();
  const auto blue = numeric_color(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
  ImGui::PushStyleColor(ImGuiCol_Text, blue);
  ImGui::PushStyleColor(ImGuiCol_Button, {0, 0, 0, 0});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0, 0, 0, 0});
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0, 0, 0, 0});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
  const auto label = ui.text(about_source) + "###about-source";
  const bool clicked = ImGui::Button(label.c_str());
  ImGui::PopStyleVar(2);
  ImGui::PopStyleColor(4);
  ui.mark("about.source");
  ui.items["about.source.color"] = blue;
  if (ImGui::IsItemHovered()) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    const auto a=ImGui::GetItemRectMin(), b=ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddLine({a.x,b.y},{b.x,b.y},ImGui::GetColorU32(blue));
    ImGui::SetTooltip("%s", source_repository);
  }
  auto *storage=ImGui::GetStateStorage();
  const auto error=ImGui::GetID("about-source-error");
  if(clicked)storage->SetBool(error,!open_source_repository(ui.window()));
  if(storage->GetBool(error,false)){
    ImGui::TextWrapped("%s",ui.text("无法打开浏览器。 / Could not open the browser.").c_str());
    ui.mark("about.source.error");
    if(ui.button("about.source.copy","复制地址 / Copy URL"))ImGui::SetClipboardText(source_repository);
  }
}
void about_popup(LibraryPanel &ui, const PluginInfo &info) {
  const auto id = ui.text("关于 / About") + "###plugin-about";
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                          ImGuiCond_Appearing, {.5f, .5f});
  ImGui::SetNextWindowSize(
      {std::min(390 * ui.scale(),
                ImGui::GetIO().DisplaySize.x - 24 * ui.scale()),
       0},
      ImGuiCond_Always);
  bool open = true;
  if (ImGui::BeginPopupModal(id.c_str(), &open,
                             ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoSavedSettings)) {
    about_content(ui,info);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ui.button("about.close", "关闭 / Close", -1) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape, false))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
}
struct ThreadDpiScope {
  DPI_AWARENESS_CONTEXT previous =
      SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  ThreadDpiScope() {
    if (!previous)
      previous =
          SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
  }
  ~ThreadDpiScope() {
    if (previous)
      SetThreadDpiAwarenessContext(previous);
  }
};
template <class T> void release(T *&p) {
  if (p) {
    p->Release();
    p = nullptr;
  }
}
ImVec4 color(COLORREF c) {
  return {GetRValue(c) / 255.f, GetGValue(c) / 255.f, GetBValue(c) / 255.f, 1};
}
void style(const Palette &c) {
  auto &s = ImGui::GetStyle();
  auto *p = s.Colors;
  p[ImGuiCol_WindowBg] = p[ImGuiCol_ChildBg] = color(c.bg);
  p[ImGuiCol_PopupBg] = color(c.field);
  p[ImGuiCol_TitleBg] = p[ImGuiCol_TitleBgActive] =
      p[ImGuiCol_TitleBgCollapsed] = color(c.button);
  p[ImGuiCol_Text] = color(c.text);
  p[ImGuiCol_TextDisabled] = color(c.border);
  p[ImGuiCol_Border] = color(c.border);
  p[ImGuiCol_FrameBg] = color(c.field);
  p[ImGuiCol_FrameBgHovered] = color(c.button);
  p[ImGuiCol_FrameBgActive] = color(c.pressed);
  p[ImGuiCol_Button] = color(c.button);
  p[ImGuiCol_ButtonHovered] = color(c.pressed);
  p[ImGuiCol_ButtonActive] = color(c.field);
  p[ImGuiCol_Header] = color(c.bg);
  p[ImGuiCol_HeaderHovered] = color(c.button);
  p[ImGuiCol_HeaderActive] = color(c.pressed);
  p[ImGuiCol_CheckMark] = p[ImGuiCol_SliderGrab] = color(c.text);
  p[ImGuiCol_SliderGrabActive] = color(c.border);
  p[ImGuiCol_ScrollbarBg] = color(c.bg);
  p[ImGuiCol_ScrollbarGrab] = color(c.border);
  p[ImGuiCol_ScrollbarGrabHovered] = p[ImGuiCol_ScrollbarGrabActive] =
      color(c.text);
  p[ImGuiCol_Separator] = color(c.track);
}
} // namespace
struct LibraryPanel::Impl {
  HWND window = nullptr;
  HWND preference_window = nullptr;
  LanguageMode language_mode = LanguageMode::follow_host;
  std::string locale;
  bool running = true, accept = false;
  bool numeric_escape = false;
  int frames = 0;
  std::uintptr_t parameter_base = 0;
  std::vector<unsigned char> parameter_values;
  std::map<std::string, std::vector<unsigned char>> parameter_overrides;
  float dpi = 1, pending_dpi = 0;
  int min_width = 600, min_height = 400;
  float font_size = 0;
  int font_builds = 0;
  ImGuiStyle base_style;
  ImVector<ImWchar> glyphs;
  std::string font_path;
  UINT resize_w = 0, resize_h = 0;
  ID3D11Device *device = nullptr;
  ID3D11DeviceContext *context = nullptr;
  IDXGISwapChain *swap = nullptr;
  ID3D11RenderTargetView *target = nullptr;
  ID3D11Texture2D *texture = nullptr;
  ID3D11ShaderResourceView *view = nullptr;
  int image_w = 0, image_h = 0;
  PreviewView displayed_preview;
  PreviewColorSampler color_sampler;
  struct {
    ImGuiID active=0, ready=0;
    double rgb[3]{};
    POINT click{};
    POINT start{};
    HCURSOR cursor=nullptr;
    bool clicked=false, cancel=false, error=false, initial_press=false;
    bool ignore_capture_loss=false;
  } color_pick;
  struct WheelEvent {float x,y,delta;};
  std::vector<WheelEvent> preview_wheels;
  RECT preview_rect{};
  bool render_target() {
    ID3D11Texture2D *buffer = nullptr;
    if (FAILED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D),
                               reinterpret_cast<void **>(&buffer))))
      return false;
    const auto hr = device->CreateRenderTargetView(buffer, nullptr, &target);
    buffer->Release();
    return SUCCEEDED(hr);
  }
  bool create(bool software) {
    DXGI_SWAP_CHAIN_DESC d{};
    d.BufferCount = 2;
    d.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d.OutputWindow = window;
    d.SampleDesc.Count = 1;
    d.Windowed = TRUE;
    d.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0,
                                        D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL level;
    auto hr = D3D11CreateDeviceAndSwapChain(
        nullptr, software ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, levels, 2, D3D11_SDK_VERSION, &d, &swap, &device, &level,
        &context);
    if (FAILED(hr) && !software) {
      release(swap);
      release(context);
      release(device);
      hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                                         0, levels, 2, D3D11_SDK_VERSION, &d,
                                         &swap, &device, &level, &context);
    }
    return SUCCEEDED(hr) && render_target();
  }
  ~Impl() {
    release(view);
    release(texture);
    release(target);
    if (context) {
      context->ClearState();
      context->Flush();
    }
    release(swap);
    release(context);
    release(device);
  }
};
namespace {
bool rebuild_scale(LibraryPanel::Impl &state, float scale,
                   const Palette &palette) {
  auto &io = ImGui::GetIO();
  // Re-rasterize at actual device pixels. Never magnify an old font atlas.
  ImGui_ImplDX11_InvalidateDeviceObjects();
  io.FontDefault = nullptr;
  io.Fonts->Clear();
  ImFontConfig cfg{};
  cfg.OversampleH = 2;
  cfg.OversampleV = 1;
  cfg.PixelSnapH = true;
  state.font_size = raster_font_pixels(scale);
  io.FontDefault = io.Fonts->AddFontFromFileTTF(
      state.font_path.c_str(), state.font_size, &cfg, state.glyphs.Data);
  io.FontGlobalScale = 1;
  io.FontAllowUserScaling = false;
  io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
  io.Fonts->TexDesiredWidth = scale <= 2.f ? 2048 : 4096;
  if (!io.FontDefault || !io.Fonts->Build() ||
      !ImGui_ImplDX11_CreateDeviceObjects())
    return false;
  ImGui::GetStyle() = state.base_style;
  ImGui::GetStyle().ScaleAllSizes(scale);
  style(palette);
  state.dpi = scale;
  ++state.font_builds;
  return true;
}
LRESULT CALLBACK proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
  auto *p = reinterpret_cast<LibraryPanel::Impl *>(
      GetWindowLongPtrW(w, GWLP_USERDATA));
  if (msg == WM_NCCREATE) {
    p = static_cast<LibraryPanel::Impl *>(
        reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
    SetWindowLongPtrW(w, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
  }
  // Own mouse capture makes the disabled host canvas a read-only sample target.
  // Do not feed these clicks to ImGui or Photoshop's normal edit/tool handlers.
  if(p && p->color_pick.active) {
    if(msg==WM_SETCURSOR){SetCursor(p->color_pick.cursor);return TRUE;}
    if(msg==WM_LBUTTONDOWN || msg==WM_LBUTTONDBLCLK) {
      POINT point{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};
      ClientToScreen(w,&point);p->color_pick.click=point;p->color_pick.clicked=true;
      p->color_pick.initial_press=false;
      return 0;
    }
    if(msg==WM_LBUTTONUP && p->color_pick.initial_press) {
      p->color_pick.initial_press=false;
      POINT point{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};
      ClientToScreen(w,&point);
      if(std::hypot(double(point.x-p->color_pick.start.x),double(point.y-p->color_pick.start.y))>=3*p->dpi) {
        p->color_pick.click=point;p->color_pick.clicked=true;
      }
      // Finish ImGui's initiating button gesture, while retaining capture for a
      // click-to-arm or a drag-to-pick. The backend owns its mouse-button bits.
      p->color_pick.ignore_capture_loss=true;
      ImGui_ImplWin32_WndProcHandler(w,msg,wp,lp);
      SetCapture(w);
      p->color_pick.ignore_capture_loss=false;
      return 0;
    }
    if(msg==WM_RBUTTONDOWN || (msg==WM_KEYDOWN && wp==VK_ESCAPE)) {
      p->color_pick.cancel=true;return 0;
    }
    if(msg==WM_CAPTURECHANGED && reinterpret_cast<HWND>(lp)!=w && !p->color_pick.ignore_capture_loss)
      p->color_pick.cancel=true;
    if(msg==WM_KILLFOCUS)p->color_pick.cancel=true;
    if(msg==WM_LBUTTONUP || msg==WM_RBUTTONUP || msg==WM_MBUTTONDOWN ||
       msg==WM_MBUTTONUP || msg==WM_MOUSEWHEEL || msg==WM_MOUSEHWHEEL)
      return 0;
  }
  if(p && msg==WM_MOUSEWHEEL && ImGui::GetCurrentContext()) {
    POINT point{static_cast<short>(LOWORD(lp)),static_cast<short>(HIWORD(lp))};
    ScreenToClient(w,&point);
    if(PtInRect(&p->preview_rect,point) &&
       !ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopupId)) {
      if(p->preview_wheels.size()<64)
        p->preview_wheels.push_back({float(point.x),float(point.y),float(static_cast<short>(HIWORD(wp)))/WHEEL_DELTA});
      return 0;
    }
  }
  if (ImGui::GetCurrentContext() &&
      ImGui_ImplWin32_WndProcHandler(w, msg, wp, lp))
    return TRUE;
  switch (msg) {
  case WM_CLOSE:
    if (p)
      p->running = false;
    return 0;
  case WM_ERASEBKGND:
    return 1;
  case WM_PAINT:
    ValidateRect(w, nullptr);
    return 0;
  case WM_SIZE:
    if (p && wp != SIZE_MINIMIZED) {
      p->resize_w = LOWORD(lp);
      p->resize_h = HIWORD(lp);
    }
    return 0;
  case WM_GETMINMAXINFO: {
    // Windows can query tracking limits before WM_NCCREATE attaches Impl.
    // Applying the generic 400px minimum here would enlarge compact filters.
    if (!p)
      return DefWindowProcW(w, msg, wp, lp);
    auto *m = reinterpret_cast<MINMAXINFO *>(lp);
    const float s = p ? p->dpi : 1;
    MONITORINFO info{sizeof(info), {}, {}, 0};
    GetMonitorInfoW(MonitorFromWindow(w, MONITOR_DEFAULTTONEAREST), &info);
    RECT frame{0, 0, LONG((p ? p->min_width : 600) * s),
               LONG((p ? p->min_height : 400) * s)};
    AdjustWindowRectExForDpi(&frame,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        FALSE, WS_EX_DLGMODALFRAME, UINT(std::round(96 * s)));
    m->ptMinTrackSize = {std::min(frame.right - frame.left,
                                  info.rcWork.right - info.rcWork.left),
                         std::min(frame.bottom - frame.top,
                                  info.rcWork.bottom - info.rcWork.top)};
    return 0;
  }
  case WM_SYSCOMMAND:
    if ((wp & 0xfff0) == SC_KEYMENU)
      return 0;
    break;
  case WM_DPICHANGED: {
    if (p)
      p->pending_dpi = HIWORD(wp) / 96.f;
    auto *r = reinterpret_cast<RECT *>(lp);
    SetWindowPos(w, nullptr, r->left, r->top, r->right - r->left,
                 r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
    return 0;
  }
  case WM_NCDESTROY:
    SetWindowLongPtrW(w, GWLP_USERDATA, 0);
    break;
  }
  return DefWindowProcW(w, msg, wp, lp);
}
} // namespace
HWND LibraryPanel::window() const { return impl->window; }
float LibraryPanel::scale() const { return impl->dpi; }
int LibraryPanel::frame() const { return impl->frames; }
float LibraryPanel::font_pixels() const { return impl->font_size; }
int LibraryPanel::font_generation() const { return impl->font_builds; }
Language LibraryPanel::language() const {
  return resolve_language(impl->language_mode, impl->locale);
}
LanguageMode LibraryPanel::language_mode() const { return impl->language_mode; }
std::string LibraryPanel::text(const char *bilingual) const {
  return std::string(localized_text(bilingual, language()));
}
void LibraryPanel::test_scale(float value) {
  if (!test_hook || !std::isfinite(value) || value < .75f || value > 4.f)
    return;
  RECT r;
  GetWindowRect(window(), &r);
  const float factor = value / scale();
  impl->pending_dpi = value;
  SetWindowPos(window(), nullptr, 0, 0,
               static_cast<int>(std::lround((r.right - r.left) * factor)),
               static_cast<int>(std::lround((r.bottom - r.top) * factor)),
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}
void LibraryPanel::close(bool accept) {
  impl->accept = accept;
  impl->running = false;
}
bool LibraryPanel::picking_color() const {return impl->color_pick.active!=0;}
bool LibraryPanel::color_swatch(const char *key, FilterRecord *host,
                               double rgb[3], ImVec2 size) {
  ImGui::PushID(key);
  const auto id=ImGui::GetID("##pick-target");
  bool changed=false;
  if(impl->color_pick.ready==id) {
    for(int k=0;k<3;++k)rgb[k]=impl->color_pick.rgb[k];
    impl->color_pick.ready=0;changed=true;
  }
  if(size.x<=0)size={54*scale(),20*scale()};
  if(ImGui::ColorButton("##color",{float(rgb[0]),float(rgb[1]),float(rgb[2]),1},
      ImGuiColorEditFlags_NoTooltip|ImGuiColorEditFlags_NoDragDrop,size))
    changed|=host_pick_rgb(host,rgb)==noErr;
  mark(key);
  if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    ImGui::SetTooltip("%s",text("打开 PS 拾色器。 / Open Photoshop's color picker.").c_str());
  ImGui::SameLine();
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
  for(auto c:{ImGuiCol_Button,ImGuiCol_ButtonHovered,ImGuiCol_ButtonActive})
    ImGui::PushStyleColor(c,{0,0,0,0});
  const bool pressed=ImGui::Button("##eyedropper",{28*scale(),std::max(22*scale(),size.y)});
  const bool activated=ImGui::IsItemActivated();
  const bool highlighted=ImGui::IsItemHovered()||ImGui::IsItemActive();
  ImGui::PopStyleColor(3);
  ImGui::PopStyleVar();
  const auto a=ImGui::GetItemRectMin(),b=ImGui::GetItemRectMax();
  draw_eyedropper_icon(ImGui::GetWindowDrawList(),{(a.x+b.x)*.5f,(a.y+b.y)*.5f},
                       20*scale(),highlighted?
                         ImGui::GetColorU32(numeric_color(ImGui::GetStyle().Colors[ImGuiCol_WindowBg])):
                         ImGui::GetColorU32(ImGuiCol_Text));
  if(pressed||activated) {
    impl->color_pick.active=id;impl->color_pick.ready=0;
    impl->color_pick.clicked=impl->color_pick.cancel=impl->color_pick.error=false;
    impl->color_pick.initial_press=ImGui::IsMouseDown(0);
    const auto mouse=ImGui::GetIO().MousePos;
    impl->color_pick.start={LONG(mouse.x),LONG(mouse.y)};ClientToScreen(window(),&impl->color_pick.start);
    impl->color_pick.cursor=host_eyedropper_cursor(host);
    SetCapture(window());
  }
  mark((std::string(key)+".pick").c_str());
  if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    ImGui::SetTooltip("%s",text("吸管：点击后取色，或按住拖到目标后松开。支持缩览图、PS 画布和全屏幕；Esc 或右键取消。 / Eyedropper: click then pick, or drag and release. Preview, Photoshop canvas and full screen; Esc or right-click cancels.").c_str());
  ImGui::PopID();return changed;
}
void LibraryPanel::mark(const char *key) {
  auto a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
  items[key] = {a.x, a.y, b.x, b.y};
}
bool LibraryPanel::button(const char *key, const char *label, float width) {
  ImGui::PushID(key);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10 * scale());
  const bool result =
      ImGui::Button((text(label) + "###button").c_str(), {width, 0});
  mark(key);
  ImGui::PopStyleVar();
  ImGui::PopID();
  return result;
}
void LibraryPanel::parameter_defaults(const void *current, const void *defaults, size_t bytes) {
  impl->parameter_base = reinterpret_cast<std::uintptr_t>(current);
  const auto *first = static_cast<const unsigned char *>(defaults);
  impl->parameter_values.assign(first, first + bytes);
  impl->parameter_overrides.clear();
}
bool LibraryPanel::parameter_default(const void *field, void *out, size_t bytes) const {
  const auto address = reinterpret_cast<std::uintptr_t>(field);
  if(address < impl->parameter_base) return false;
  const auto offset = address - impl->parameter_base;
  if(offset > impl->parameter_values.size() || bytes > impl->parameter_values.size() - offset) return false;
  std::memcpy(out, impl->parameter_values.data() + offset, bytes);
  return true;
}
void LibraryPanel::number_default(const char *label, double value) {
  auto &bytes = impl->parameter_overrides[label]; bytes.resize(sizeof(value));
  std::memcpy(bytes.data(), &value, sizeof(value));
}
void LibraryPanel::choice_default(const char *label, int value) {
  auto &bytes = impl->parameter_overrides[label]; bytes.resize(sizeof(value));
  std::memcpy(bytes.data(), &value, sizeof(value));
}
namespace {
bool parameter_reset_icon(LibraryPanel &ui, const char *label) {
  ImGui::PushID(label);
  const float s=ui.scale(), h=ImGui::GetFrameHeight();
  const bool pressed=ImGui::InvisibleButton("##parameter-reset", {18*s,h});
  ui.mark((std::string(label)+".reset").c_str());
  const auto a=ImGui::GetItemRectMin();
  const ImVec2 center{a.x+9*s,a.y+h*.5f};
  auto *draw=ImGui::GetWindowDrawList();
  const auto blue=ImGui::GetColorU32(numeric_color(ImGui::GetStyle().Colors[ImGuiCol_WindowBg]));
  draw->PathArcTo(center,5*s,-.25f,4.65f,24);
  draw->PathStroke(blue,0,1.4f*s);
  draw->AddTriangleFilled({center.x-3.7f*s,center.y-5.9f*s},
                          {center.x+1.1f*s,center.y-8*s},
                          {center.x+.1f*s,center.y-2.8f*s},blue);
  if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    ImGui::SetTooltip("%s",ui.text("重置此参数 / Reset this parameter").c_str());
  ImGui::PopID();ImGui::SameLine(0,3*s);
  return pressed;
}
}
bool LibraryPanel::parameter_label(const char *label, void *value, size_t bytes) {
  std::vector<unsigned char> defaults(bytes);
  bool found=parameter_default(value,defaults.data(),bytes), changed=false;
  if(!found) {
    const auto it=impl->parameter_overrides.find(label);
    if(it!=impl->parameter_overrides.end() && it->second.size()==bytes) {defaults=it->second;found=true;}
  }
  if(found && parameter_reset_icon(*this,label) && std::memcmp(value,defaults.data(),bytes)!=0) {
    std::memcpy(value,defaults.data(),bytes);changed=true;
  }
  ImGui::AlignTextToFramePadding();ImGui::TextUnformatted(text(label).c_str());
  return changed;
}
bool LibraryPanel::point_label(const char *label, double &x, double &y) {
  double dx{},dy{};bool changed=false;
  if(parameter_default(&x,&dx,sizeof(dx)) && parameter_default(&y,&dy,sizeof(dy)) &&
     parameter_reset_icon(*this,label) && (x!=dx || y!=dy)) {x=dx;y=dy;changed=true;}
  ImGui::AlignTextToFramePadding();ImGui::TextUnformatted(text(label).c_str());
  return changed;
}
bool LibraryPanel::check(const char *label, bool &value) {
  ImGui::PushID(label);
  bool result=false, defaults=false;
  if(parameter_default(&value,&defaults,sizeof(defaults)) && parameter_reset_icon(*this,label) && value!=defaults) {value=defaults;result=true;}
  result |=
      ImGui::Checkbox((text(label) + "###check").c_str(), &value);
  mark(label);
  ImGui::PopID();
  return result;
}
bool LibraryPanel::section(const char *label, bool open) {
  ImGui::PushID(label);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
  const bool result =
      ImGui::CollapsingHeader((text(label) + "###section").c_str(),
                              open ? ImGuiTreeNodeFlags_DefaultOpen : 0);
  mark(label);
  ImGui::PopStyleVar();
  ImGui::PopID();
  ImGui::Separator();
  return result;
}
bool LibraryPanel::numeric_value(const char *id, double &value, double minimum,
                                 double maximum, double units_per_pixel,
                                 bool integer, const char *format) {
  const auto result = scrub_number(id, value, minimum, maximum, units_per_pixel,
                                    scale(), integer, format);
  impl->numeric_escape |= result.consumed_escape;
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
    ImGui::SetTooltip("%s", text("左右拖动；Shift 加快 10 倍；Ctrl 放慢 10 倍；点击输入。 / Drag horizontally; Shift: 10x faster; Ctrl: 10x finer; click to type.").c_str());
  return result.changed;
}
bool LibraryPanel::number(const char *label, double &value, double minimum,
                          double maximum, double slide_min, double slide_max,
                          bool enabled, bool integer) {
  ImGui::PushID(label);
  ImGui::BeginDisabled(!enabled);
  bool changed = false;
  if (ImGui::BeginTable("row", 3, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch, 0.43f);
    ImGui::TableSetupColumn("number", ImGuiTableColumnFlags_WidthStretch,
                            0.22f);
    ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthStretch,
                            0.35f);
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    changed |= parameter_label(label,value);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    const double speed = integer ? 1.0 : std::clamp(
        std::abs(slide_max - slide_min) / 200.0, .005, 1.0);
    changed |= numeric_value("##value", value, minimum, maximum, speed, integer);
    mark(label);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    changed |= rail_slider(value, slide_min, slide_max, scale());
    mark((std::string(label) + ".slider").c_str());
    ImGui::EndTable();
  }
  if (changed) {
    if (!std::isfinite(value))
      value = minimum;
    value = std::clamp(value, minimum, maximum);
    if (integer)
      value = std::round(value);
  }
  ImGui::EndDisabled();
  ImGui::PopID();
  return changed;
}
bool LibraryPanel::choice(const char *label, int &value,
                          const char *const *labels, int count) {
  ImGui::PushID(label);
  bool changed = false;
  if (ImGui::BeginTable("row", 2, ImGuiTableFlags_SizingStretchProp)) {
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch, 0.43f);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, 0.57f);
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    changed |= parameter_label(label,value);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1);
    const bool open = ImGui::BeginCombo(
        "##choice", text(labels[std::clamp(value, 0, count - 1)]).c_str());
    mark(label);
    if (open) {
      for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        if (ImGui::Selectable((text(labels[i]) + "###item").c_str(),
                              value == i)) {
          value = i;
          changed = true;
        }
        mark((std::string(label) + "/item/" + std::to_string(i)).c_str());
        if (value == i)
          ImGui::SetItemDefaultFocus();
        ImGui::PopID();
      }
      ImGui::EndCombo();
    }
    ImGui::EndTable();
  }
  ImGui::PopID();
  return changed;
}
bool LibraryPanel::preview(FilterRecord *host, const std::uint8_t *rgb, int w,
                           int h) {
  if (!rgb || w <= 0 || h <= 0)
    return false;
  std::vector<std::uint32_t> pixels(static_cast<std::size_t>(w) * h);
  HDC dc = CreateCompatibleDC(nullptr);
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  info.bmiHeader.biWidth = w;
  info.bmiHeader.biHeight = -h;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  void *bits = nullptr;
  HBITMAP bitmap =
      dc ? CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0)
         : nullptr;
  if (bitmap) {
    auto old = SelectObject(dc, bitmap);
    if (host_display_rgb(host, dc, {0, 0, w, h}, rgb, w, h)) {
      GdiFlush();
      auto *source = static_cast<std::uint8_t *>(bits);
      for (std::size_t i = 0; i < pixels.size(); ++i)
        pixels[i] = source[i * 4 + 2] | (source[i * 4 + 1] << 8) |
                    (source[i * 4] << 16) | 0xff000000U;
    } else
      for (std::size_t i = 0; i < pixels.size(); ++i)
        pixels[i] = rgb[i * 3] | (rgb[i * 3 + 1] << 8) |
                    (rgb[i * 3 + 2] << 16) | 0xff000000U;
    SelectObject(dc, old);
    DeleteObject(bitmap);
    DeleteDC(dc);
  } else {
    if (dc)
      DeleteDC(dc);
    return false;
  }
  if (w != impl->image_w || h != impl->image_h) {
    D3D11_TEXTURE2D_DESC d{};
    d.Width = w;
    d.Height = h;
    d.MipLevels = 1;
    d.ArraySize = 1;
    d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count = 1;
    d.Usage = D3D11_USAGE_DYNAMIC;
    d.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Texture2D *texture = nullptr;
    ID3D11ShaderResourceView *view = nullptr;
    if (FAILED(impl->device->CreateTexture2D(&d, nullptr, &texture)))
      return false;
    if (FAILED(
            impl->device->CreateShaderResourceView(texture, nullptr, &view))) {
      release(texture);
      return false;
    }
    release(impl->view);
    release(impl->texture);
    impl->texture = texture;
    impl->view = view;
    impl->image_w = w;
    impl->image_h = h;
  }
  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (FAILED(impl->context->Map(impl->texture, 0, D3D11_MAP_WRITE_DISCARD, 0,
                                &mapped)))
    return false;
  for (int y = 0; y < h; ++y)
    std::memcpy(static_cast<std::uint8_t *>(mapped.pData) + y * mapped.RowPitch,
                pixels.data() + y * w, w * 4);
  impl->context->Unmap(impl->texture, 0);
  impl->color_sampler.pixels(rgb,w,h);
  return true;
}
void LibraryPanel::image(float width, float height) {
  impl->color_sampler.visible=false;
  if (!impl->view) {
    ImGui::Dummy({width, height});
    return;
  }
  const float s = std::min(width / impl->image_w, height / impl->image_h);
  ImGui::Image(reinterpret_cast<ImTextureID>(impl->view),
               {impl->image_w * s, impl->image_h * s});
  mark("image");
  const auto lo=ImGui::GetItemRectMin(),hi=ImGui::GetItemRectMax();
  impl->color_sampler.geometry(lo,hi,lo,hi);
}
void LibraryPanel::image_view(PreviewView &v, const ProxyImage &source) {
  impl->color_sampler.visible=false;
  const auto a = ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("##preview-pan", {v.width, v.height}, ImGuiButtonFlags_MouseButtonLeft);
  mark("image");
  // Keep the last complete image and transform together until a new ROI is ready.
  items["preview.source"]={float(source.origin_x),float(source.origin_y),float(source.origin_x+source.width*source.step),float(source.origin_y+source.height*source.step)};
  if (!v.dirty) impl->displayed_preview=v;
  const auto shown=impl->displayed_preview;
  impl->preview_rect={LONG(std::floor(a.x)),LONG(std::floor(a.y)),LONG(std::ceil(a.x+v.width)),LONG(std::ceil(a.y+v.height))};
  // Native wheel messages carry the event's actual screen position. Do not
  // use a possibly delayed ImGui hover position to route Windows scrolling.
  for(const auto& event:impl->preview_wheels)
    if(!ImGui::IsMouseDown(0)&&event.x>=a.x&&event.x<a.x+v.width&&event.y>=a.y&&event.y<a.y+v.height)
      v.wheel(event.delta,event.x-a.x,event.y-a.y);
  impl->preview_wheels.clear();
  if(ImGui::IsItemHovered()) {
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    const auto& io=ImGui::GetIO();
    if(test_hook&&!test_hook->native_wheel_input&&!ImGui::IsMouseDown(0)&&io.MouseWheel!=0)
      v.wheel(io.MouseWheel,io.MousePos.x-a.x,io.MousePos.y-a.y);
  }
  if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
    if(v.limit_x()>0 || v.limit_y()>0) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))v.recenter();
    else if(ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
      const auto delta=ImGui::GetIO().MouseDelta;v.pan(delta.x,delta.y);
    }
    if(ImGui::IsItemHovered() && !ImGui::IsMouseDown(0))
      ImGui::SetTooltip("%s",text("滚轮缩放；拖动平移；双击居中。 / Wheel to zoom; drag to pan; double-click to center.").c_str());
  }
  auto *list = ImGui::GetWindowDrawList();
  const ImVec2 end{a.x+v.width,a.y+v.height};
  items["preview.transform"]={float(v.percent),float(v.pan_x),float(v.pan_y),v.fitted?1.f:0.f};
  list->AddRectFilled(a,end,ImGui::GetColorU32(ImGuiCol_FrameBg));
  const auto border=[&]{list->AddRect(a,{end.x-1,end.y-1},ImGui::GetColorU32(ImGuiCol_Border),0,0,std::max(1.f,scale()));};
  if (!impl->view || source.width <= 0 || source.height <= 0)
    {border();return;}
  const double z = shown.percent / 100;
  const ImVec2 mid{a.x + v.width * .5f, a.y + v.height * .5f};
  const ImVec2 lo{float(mid.x + (source.origin_x - shown.center_x()) * z),
                  float(mid.y + (source.origin_y - shown.center_y()) * z)};
  const ImVec2 hi{float(lo.x + source.width * source.step * z),
                  float(lo.y + source.height * source.step * z)};
  const ImVec2 clip_lo{
      std::max(a.x, float(mid.x + (v.bounds.left - shown.center_x()) * z)),
      std::max(a.y, float(mid.y + (v.bounds.top - shown.center_y()) * z))};
  const ImVec2 clip_hi{
      std::min(a.x + v.width,
               float(mid.x + (v.bounds.right - shown.center_x()) * z)),
      std::min(a.y + v.height,
               float(mid.y + (v.bounds.bottom - shown.center_y()) * z))};
  list->PushClipRect(clip_lo, clip_hi, true);
  list->AddImage(reinterpret_cast<ImTextureID>(impl->view), lo, hi);
  list->PopClipRect();
  impl->color_sampler.geometry(lo,hi,clip_lo,clip_hi);
  border();
}
bool LibraryPanel::preview_zoom(PreviewView &v) {
  ImGui::PushID("preview-zoom");
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(text("缩放 / Zoom").c_str());
  ImGui::SameLine();
  ImGui::SetNextItemWidth(70 * scale());
  double percent = v.percent;
  bool changed = numeric_value("##percent", percent,
      std::min(.01, v.fit_percent()), std::max(800.0, v.fit_percent()), 1, false,
      percent < 10 ? "%.2f%%" : "%.1f%%");
  mark("preview.zoom");
  if (changed)
    v.zoom(percent);
  ImGui::SameLine();
  if (button("preview.fit", "适合 / Fit", -1)) {
    v.fit();
    changed = true;
  }
  ImGui::SetNextItemWidth(-1);
  double position = std::log2(std::max(.000001, v.percent));
  if (rail_slider(position, std::log2(std::min(.01, v.fit_percent())),
                  std::log2(std::max(800.0, v.fit_percent())), scale())) {
    v.zoom(std::exp2(position));
    changed = true;
  }
  mark("preview.zoom.slider");
  if (ImGui::IsItemHovered())
    ImGui::SetTooltip(
        "%s", text("仅缩放预览；100% 为一个图像像素对应一个屏幕像素。 / View "
                   "only; 100% is one image pixel per screen pixel.")
                  .c_str());
  ImGui::PopID();
  return changed;
}
bool LibraryPanel::capture(const wchar_t *path) {
  ID3D11Texture2D *back = nullptr;
  if (FAILED(impl->swap->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                   reinterpret_cast<void **>(&back))))
    return false;
  D3D11_TEXTURE2D_DESC d;
  back->GetDesc(&d);
  d.Usage = D3D11_USAGE_STAGING;
  d.BindFlags = 0;
  d.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  ID3D11Texture2D *staging = nullptr;
  if (FAILED(impl->device->CreateTexture2D(&d, nullptr, &staging))) {
    release(back);
    return false;
  }
  impl->context->CopyResource(staging, back);
  release(back);
  D3D11_MAPPED_SUBRESOURCE map{};
  if (FAILED(impl->context->Map(staging, 0, D3D11_MAP_READ, 0, &map))) {
    release(staging);
    return false;
  }
  BITMAPFILEHEADER fh{};
  BITMAPINFOHEADER bi{};
  bi.biSize = sizeof(bi);
  bi.biWidth = d.Width;
  bi.biHeight = -static_cast<LONG>(d.Height);
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  fh.bfType = 0x4d42;
  fh.bfOffBits = sizeof(fh) + sizeof(bi);
  fh.bfSize = fh.bfOffBits + d.Width * d.Height * 4;
  std::ofstream file(std::filesystem::path(path), std::ios::binary);
  file.write(reinterpret_cast<const char *>(&fh), sizeof(fh));
  file.write(reinterpret_cast<const char *>(&bi), sizeof(bi));
  std::vector<std::uint32_t> row(d.Width);
  for (UINT y = 0; y < d.Height; ++y) {
    auto *source =
        static_cast<const std::uint8_t *>(map.pData) + map.RowPitch * y;
    for (UINT x = 0; x < d.Width; ++x)
      row[x] =
          (source[x * 4] << 16) | (source[x * 4 + 1] << 8) | source[x * 4 + 2];
    file.write(reinterpret_cast<const char *>(row.data()), row.size() * 4);
  }
  impl->context->Unmap(staging, 0);
  release(staging);
  return file.good();
}
void set_panel_test_hook(PanelTestHook *hook) { test_hook = hook; }
void show_host_about(const PluginInfo &info, const wchar_t *title) {
  HMODULE module=nullptr;
  GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(&show_host_about),&module);
  run_library_panel(module,GetActiveWindow(),nullptr,title,440,320,
    [&](LibraryPanel& ui){
      about_content(ui,info);
      ImGui::Spacing();ImGui::Separator();ImGui::Spacing();
      if(ui.button("about.close","关闭 / Close",-1))ui.close(false);
    },info.name);
}
INT_PTR run_library_panel(HINSTANCE module, HWND owner, FilterRecord *host,
                          const wchar_t *title, int width, int height,
                          const std::function<void(LibraryPanel &)> &draw,
                          const char *localized_text, const PluginInfo *info) {
  ThreadDpiScope dpi_scope;
  const wchar_t *cls = L"ZaYu.LibraryPanel.ImGui.1";
  LibraryPanel::Impl state;
  state.min_width = std::min(width, 600);
  state.min_height = std::min(height, 400);
  state.locale = test_hook ? test_hook->host_locale : host_locale(host);
  constexpr auto language_property = L"ZaYu.AEPS.UILanguage.v1";
  if (!test_hook && owner) {
    state.preference_window = GetAncestor(owner, GA_ROOTOWNER);
    DWORD process_id = 0;
    GetWindowThreadProcessId(state.preference_window, &process_id);
    if (process_id != GetCurrentProcessId())
      state.preference_window = nullptr;
    const auto saved = reinterpret_cast<UINT_PTR>(
        GetPropW(state.preference_window, language_property));
    if (saved >= 1 && saved <= 3)
      state.language_mode = static_cast<LanguageMode>(saved - 1);
  }
  LibraryPanel panel(&state);
  Theme theme;
  WNDCLASSEXW wc{sizeof(wc)};
  wc.lpfnWndProc = proc;
  wc.hInstance = module;
  wc.lpszClassName = cls;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  if (!RegisterClassExW(&wc))
    return -1;
  const auto monitor = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
  MONITORINFO monitor_info{sizeof(monitor_info), {}, {}, 0};
  GetMonitorInfoW(monitor, &monitor_info);
  state.dpi = ImGui_ImplWin32_GetDpiScaleForMonitor(monitor);
  if (test_hook && test_hook->initial_scale > 0)
    state.dpi = test_hook->initial_scale;
  if (state.dpi <= 0)
    state.dpi = 1;
  auto outer_size = [&](float dpi) {
    RECT r{0, 0, LONG(std::round(width * dpi)),
            LONG(std::round(height * dpi))};
    AdjustWindowRectExForDpi(&r,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
        FALSE, WS_EX_DLGMODALFRAME, UINT(std::round(96 * dpi)));
    return SIZE{std::min(r.right - r.left,
                         monitor_info.rcWork.right - monitor_info.rcWork.left),
                 std::min(r.bottom - r.top,
                         monitor_info.rcWork.bottom - monitor_info.rcWork.top)};
  };
  const auto initial = outer_size(state.dpi);
  state.window =
      CreateWindowExW(WS_EX_DLGMODALFRAME, cls, title,
                      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
                      monitor_info.rcWork.left, monitor_info.rcWork.top,
                      initial.cx, initial.cy,
                      owner, nullptr, module, &state);
  if (!state.window) {
    UnregisterClassW(cls, module);
    return -1;
  }
  if (!(test_hook && test_hook->initial_scale > 0)) {
    const float actual = GetDpiForWindow(state.window) / 96.f;
    if (actual > 0 && actual != state.dpi) {
      state.dpi = actual;
      const auto resized = outer_size(actual);
      SetWindowPos(state.window, nullptr, 0, 0,
                   resized.cx, resized.cy,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
  }
  if (!state.create(test_hook && test_hook->software)) {
    DestroyWindow(state.window);
    UnregisterClassW(cls, module);
    return -1;
  }
  auto *previous = ImGui::GetCurrentContext();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  wchar_t windows[32768];
  GetWindowsDirectoryW(windows, 32768);
  state.font_path =
      (std::filesystem::path(windows) / L"Fonts/msyh.ttc").string();
  ImFontGlyphRangesBuilder glyph_builder;
  glyph_builder.AddText("全部重置 重置此参数");
  glyph_builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
  glyph_builder.AddText("预览 确定 取消 重置 交换颜色 将黑色映射到 "
                        "将白色映射到 着色量 语言 跟随 关于 关闭 适合 缩放 "
                        "版本 仅缩放预览 为一个图像像素对应屏幕像素。");
  glyph_builder.AddText(about_disclaimer);
  glyph_builder.AddText(about_author);
  glyph_builder.AddText(about_source);
  glyph_builder.AddText("无法打开浏览器。复制地址");
  glyph_builder.AddText("放大后拖动平移；双击居中。滚轮缩放；");
  glyph_builder.AddText("左右拖动；Shift 加快 10 倍；Ctrl 放慢 10 倍；点击输入。");
  glyph_builder.AddText("取色 打开 PS 拾色器。点击缩览图或 PS 画布取色；Esc 或右键取消。此处无法取色，请点击缩览图或 PS 画布。");
  glyph_builder.AddText("吸管：点击后取色，或按住拖到目标后松开。支持缩览图、PS 画布和全屏幕；Esc 或右键取消。点击屏幕取色，或拖到目标后松开；Esc 或右键取消。此处无法读取颜色，请重试或按 Esc 取消。");
  glyph_builder.AddText("显示背景 当前图层和下方可见图层，不显示上方图层。背景暂不可用 背景为近似预览 调整图层、图层组及部分高级混合设置无法完整复现。");
  if (info)
    glyph_builder.AddText(info->name);
  if (localized_text)
    glyph_builder.AddText(localized_text);
  glyph_builder.BuildRanges(&state.glyphs);
  bool win_ready = false, dx_ready = false;
  win_ready = ImGui_ImplWin32_Init(state.window);
  if (win_ready)
    dx_ready = ImGui_ImplDX11_Init(state.device, state.context);
  if (!dx_ready) {
    if (win_ready)
      ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ImGui::SetCurrentContext(previous);
    DestroyWindow(state.window);
    UnregisterClassW(cls, module);
    return -1;
  }
  ImGui::StyleColorsDark();
  auto &s = ImGui::GetStyle();
  s.FrameRounding = 2;
  s.GrabRounding = 3;
  s.WindowPadding = {10, 10};
  s.FramePadding = {5, 3};
  s.ItemSpacing = {6, 4};
  s.FrameBorderSize = 1;
  s.WindowBorderSize = 0;
  s.ScrollbarSize = 12;
  state.base_style = s;
  theme.apply(state.window, host_brightness(host), true);
  if (!rebuild_scale(state, state.dpi, theme.colors())) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    ImGui::SetCurrentContext(previous);
    DestroyWindow(state.window);
    UnregisterClassW(cls, module);
    return -1;
  }
  center_on_owner(state.window, owner);
  const bool disabled = owner && IsWindowEnabled(owner);
  if (disabled)
    EnableWindow(owner, FALSE);
  INT_PTR result = IDCANCEL;
  ULONGLONG theme_time = 0;
  try {
    while (state.running) {
      MSG msg;
      while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
          PostQuitMessage(static_cast<int>(msg.wParam));
          state.running = false;
          break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
      }
      if (!state.running)
        break;
      if (state.resize_w && state.resize_h) {
        state.context->OMSetRenderTargets(0, nullptr, nullptr);
        release(state.target);
        if (FAILED(state.swap->ResizeBuffers(0, state.resize_w, state.resize_h,
                                             DXGI_FORMAT_UNKNOWN, 0)) ||
            !state.render_target())
          throw 1;
        state.resize_w = state.resize_h = 0;
      }
      if (state.pending_dpi > 0) {
        if (state.pending_dpi != state.dpi &&
            !rebuild_scale(state, state.pending_dpi, theme.colors()))
          throw 1;
        state.pending_dpi = 0;
      }
      const auto now = GetTickCount64();
      if (now - theme_time >= 500) {
        theme.apply(state.window, host_brightness(host));
        style(theme.colors());
        theme_time = now;
      }
      ImGui_ImplDX11_NewFrame();
      ImGui_ImplWin32_NewFrame();
      if (test_hook && test_hook->before_frame)
        test_hook->before_frame(panel);
      const bool popup_was_open =
          ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
      ImGui::NewFrame();
      const bool picking_was_active=panel.picking_color();
      const auto stop_pick=[&]{
        state.color_pick.active=0;state.color_pick.clicked=false;
        state.color_pick.cancel=false;
        if(GetCapture()==state.window)ReleaseCapture();
        SetCursor(LoadCursorW(nullptr,IDC_ARROW));
      };
      if(picking_was_active) {
        if(state.color_pick.cancel || GetCapture()!=state.window ||
           ImGui::IsKeyPressed(ImGuiKey_Escape,false) || ImGui::IsMouseClicked(1))
          stop_pick();
        else if(state.color_pick.clicked) {
          state.color_pick.clicked=false;
          POINT point=state.color_pick.click;ScreenToClient(state.window,&point);
          double color[3]{};
          bool sampled=state.color_sampler.sample(float(point.x),float(point.y),color);
          if(!sampled) {
            // A full-screen eyedropper must sample what is actually visible,
            // including other apps and PS chrome. The legacy host SamplePoint
            // may enter its own interactive sampling mode outside image areas.
            sampled=test_hook&&test_hook->screen_sample?
              test_hook->screen_sample(state.color_pick.click,color):screen_sample_rgb(state.color_pick.click,color);
          }
          if(sampled && !state.color_pick.cancel) {
            state.color_pick.ready=state.color_pick.active;
            for(int k=0;k<3;++k)state.color_pick.rgb[k]=color[k];
            stop_pick();
          } else state.color_pick.error=true;
        }
      }
      ImGui::SetNextWindowPos({0, 0});
      ImGui::SetNextWindowSize(io.DisplaySize);
      ImGui::Begin("##filter", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoSavedSettings |
                       ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse);
      panel.items.clear();
      ImGui::BeginDisabled(picking_was_active);
      if (info) {
        if (panel.button("about", "关于 / About", 70 * panel.scale()))
          ImGui::OpenPopup(
              (panel.text("关于 / About") + "###plugin-about").c_str());
        ImGui::SameLine();
      }
      // Both languages are already in the font atlas. Switching changes labels
      // in this frame, without resetting controls or rebuilding the preview.
      const float language_width = 155 * panel.scale();
      const auto label = "语言 / Language";
      ImGui::SetCursorPosX(
          std::max(ImGui::GetCursorPosX(),
                   ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x -
                       language_width - ImGui::CalcTextSize(label).x -
                       ImGui::GetStyle().ItemSpacing.x));
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted(label);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(language_width);
      constexpr const char *languages[] = {"跟随 PS / Auto", "中文", "English"};
      const bool language_open = ImGui::BeginCombo(
          "##ui-language", languages[static_cast<int>(state.language_mode)]);
      panel.mark("language");
      if (language_open) {
        for (int i = 0; i < 3; ++i) {
          if (ImGui::Selectable(languages[i],
                                static_cast<int>(state.language_mode) == i)) {
            state.language_mode = static_cast<LanguageMode>(i);
            if (state.preference_window)
              SetPropW(state.preference_window, language_property,
                       reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(i + 1)));
          }
          panel.mark(("language/item/" + std::to_string(i)).c_str());
        }
        ImGui::EndCombo();
      }
      ImGui::Separator();
      state.numeric_escape = false;
      draw(panel);
      if (info)
        about_popup(panel, *info);
      ImGui::EndDisabled();
      ImGui::End();
      panel.items["color.pick.active"]={panel.picking_color()?1.f:0.f,state.color_pick.error?1.f:0.f,0,0};
      if(panel.picking_color()) {
        ImGui::SetNextWindowPos({io.DisplaySize.x*.5f,8*panel.scale()},ImGuiCond_Always,{.5f,0});
        ImGui::SetNextWindowSizeConstraints({0,0},{std::max(100.f,io.DisplaySize.x-24*panel.scale()),FLT_MAX});
        ImGui::Begin("##color-pick-hint",nullptr,ImGuiWindowFlags_NoDecoration|
          ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoInputs|
          ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX()+io.DisplaySize.x-48*panel.scale());
        ImGui::TextUnformatted(panel.text(state.color_pick.error?
          "此处无法读取颜色，请重试或按 Esc 取消。 / Cannot read a color here; retry or press Esc to cancel.":
          "点击屏幕取色，或拖到目标后松开；Esc 或右键取消。 / Click anywhere or drag and release to pick; Esc or right-click cancels.").c_str());
        ImGui::PopTextWrapPos();ImGui::End();
        SetCursor(state.color_pick.cursor);
      }
      if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) && !picking_was_active && !state.numeric_escape && !popup_was_open &&
          !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId))
        panel.close(false);
      ImGui::Render();
      const auto c = theme.colors().bg;
      float clear[] = {GetRValue(c) / 255.f, GetGValue(c) / 255.f,
                       GetBValue(c) / 255.f, 1};
      state.context->OMSetRenderTargets(1, &state.target, nullptr);
      state.context->ClearRenderTargetView(state.target, clear);
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
      ++state.frames;
      if (test_hook && test_hook->tick)
        test_hook->tick(panel);
      if (FAILED(state.swap->Present(1, 0)))
        throw 1;
      if (state.frames == 1) {
        ShowWindow(state.window, SW_SHOW);
        SetForegroundWindow(state.window);
      }
      MsgWaitForMultipleObjectsEx(0, nullptr, 8, QS_ALLINPUT,
                                  MWMO_INPUTAVAILABLE);
    }
    result = state.accept ? IDOK : IDCANCEL;
  } catch (const std::exception &e) {
    if (test_hook)
      test_hook->error = e.what();
    result = -1;
  } catch (...) {
    if (test_hook)
      test_hook->error = "Graphics or UI failure";
    result = -1;
  }
  state.color_pick.active=0;
  if(GetCapture()==state.window)ReleaseCapture();
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
  ImGui::SetCurrentContext(previous);
  if (disabled) {
    EnableWindow(owner, TRUE);
    SetActiveWindow(owner);
  }
  DestroyWindow(state.window);
  UnregisterClassW(cls, module);
  return result;
}
} // namespace psui
