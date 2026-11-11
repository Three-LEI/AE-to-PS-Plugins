#pragma once
#include "imgui.h"
#include "localization.hpp"
#include "photoshop_host.hpp"
#include "plugin_info.hpp"
#include "preview_view.hpp"
#include <functional>
#include <map>
#include <string>
#include <type_traits>
namespace psui {
// Dear ImGui controls, popup lists, scrolling and preview share one swap chain.
// No child HWND controls or direct on-screen GDI drawing are used.
class LibraryPanel {
public:
  struct Impl;
  explicit LibraryPanel(Impl *p) : impl(p) {}
  Impl *impl;
  HWND window() const;
  float scale() const;
  int frame() const;
  float font_pixels() const;
  int font_generation() const;
  Language language() const;
  LanguageMode language_mode() const;
  std::string text(const char *bilingual) const;
  // Harness-only scaling, ignored unless an explicit test hook is installed.
  void test_scale(float scale);
  void close(bool accept);
  bool preview(FilterRecord *, const std::uint8_t *rgb, int width, int height);
  void image(float width, float height);
  void image_view(PreviewView &, const ProxyImage &);
  bool preview_zoom(PreviewView &);
  bool numeric_value(const char *id, double &, double minimum, double maximum,
                     double units_per_pixel, bool integer = false,
                     const char *format = "%.8g");
  // Swatch opens the host picker; adjacent Pick button samples the preview or
  // asks Photoshop to sample its canvas. No parameter pointers outlive a frame.
  bool color_swatch(const char *key, FilterRecord *, double rgb[3],
                    ImVec2 size = ImVec2(0,0));
  bool picking_color() const;
  // Bind the effect's factory defaults each frame. Copies defaults immediately;
  // addresses are used only to locate fields in the current parameter block.
  void parameter_defaults(const void *current, const void *defaults, size_t bytes);
  template<class T> void parameter_defaults(T &current, const T &defaults) {
    static_assert(std::is_trivially_copyable_v<T>);
    parameter_defaults(&current, &defaults, sizeof(T));
  }
  bool parameter_default(const void *field, void *out, size_t bytes) const;
  void number_default(const char *label, double value);
  void choice_default(const char *label, int value);
  template<class T> void inherit_number_default(const char *label, const T &field) {
    T value{}; if(parameter_default(&field, &value, sizeof(T))) number_default(label, double(value));
  }
  template<class T> void inherit_choice_default(const char *label, const T &field) {
    T value{}; if(parameter_default(&field, &value, sizeof(T))) choice_default(label, int(value));
  }
  bool parameter_label(const char *label, void *value, size_t bytes);
  template<class T> bool parameter_label(const char *label, T &value) {
    static_assert(std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>);
    return parameter_label(label, &value, sizeof(T));
  }
  bool point_label(const char *label, double &x, double &y);
  bool number(const char *label, double &, double minimum, double maximum,
              double slide_min, double slide_max, bool enabled = true,
              bool integer = false);
  bool choice(const char *label, int &, const char *const *labels, int count);
  bool check(const char *label, bool &);
  bool section(const char *label, bool open = false);
  bool button(const char *key, const char *label, float width = 0);
  void mark(const char *key);
  std::map<std::string, ImVec4> items;
  bool capture(const wchar_t *path);
};
struct PanelTestHook {
  std::function<void(LibraryPanel &)> tick, before_frame;
  bool software = false;
  float initial_scale = 0;
  bool native_wheel_input = false;
  bool force_legacy_dpi = false;
  // Deterministic sampling providers for the standalone panel harness only.
  std::function<bool(POINT,double*)> screen_sample;
  // Standalone UI tests capture the requested public URL without launching a browser.
  std::function<bool(const char*)> open_url;
  // Tests never read or alter the host window's session preference.
  std::string host_locale = "en_US";
  std::string error;
};
void set_panel_test_hook(PanelTestHook *);
INT_PTR run_library_panel(HINSTANCE, HWND, FilterRecord *, const wchar_t *title,
                          int width, int height,
                          const std::function<void(LibraryPanel &)> &draw,
                          const char *localized_text = nullptr,
                          const PluginInfo *info = nullptr);
} // namespace psui
