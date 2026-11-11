#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "fractal_core.hpp"
#include "fractal_info.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_composite.hpp"
#include "psui/preview_background.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>
namespace fractal {
namespace {
struct Number {
  const char *text;
  double Parameters::*member;
  double lo, hi, slider_lo, slider_hi;
};
constexpr Number nums[] = {
    {"对比度 / Contrast", &Parameters::contrast, 0, 1000, 0, 1000},
    {"亮度 / Brightness", &Parameters::brightness, -1000, 1000, -100, 100},
    {"缩放 / Scale", &Parameters::scale, 1, 10000, 1, 600},
    {"复杂度 / Complexity", &Parameters::complexity, 1, 20, 1, 20},
    {"演化(°) / Evolution", &Parameters::evolution, -1000000, 1000000, 0, 3600},
    {"随机值 / Seed", nullptr, 0, 4294967295.0, 0, 100000},
    {"不透明度 / Opacity", &Parameters::opacity, 0, 100, 0, 100},
    {"旋转(°) / Rotation", &Parameters::rotation, -100000, 100000, -360, 360},
    {"偏移 X / Offset X", &Parameters::offset_x, -2000000, 2000000, -2000,
     2000},
    {"偏移 Y / Offset Y", &Parameters::offset_y, -2000000, 2000000, -2000,
     2000},
    {"子影响(%) / Influence", &Parameters::sub_influence, 0, 100, 0, 100},
    {"子缩放(%) / Subscale", &Parameters::sub_scale, 10, 100, 10, 100},
    {"缩放宽度 / Width", &Parameters::scale_width, 1, 10000, 1, 600},
    {"缩放高度 / Height", &Parameters::scale_height, 1, 10000, 1, 600},
    {"子旋转(°) / Rotation", &Parameters::sub_rotation, -100000, 100000, -360,
     360},
    {"子位移 X / Offset X", &Parameters::sub_offset_x, -2000000, 2000000, -1000,
     1000},
    {"子位移 Y / Offset Y", &Parameters::sub_offset_y, -2000000, 2000000, -1000,
     1000},
    {"循环圈数 / Cycles", &Parameters::cycles, 1, 10000, 1, 100}};

constexpr const char *groups[] = {"基本 / Basic", "变换 / Transform",
                                  "子设置 / Sub Settings",
                                  "演化选项 / Evolution", "合成 / Compositing"};
constexpr const char *checks[] = {"反转 / Invert", "统一缩放 / Uniform Scale",
                                  "透视位移 / Perspective Offset",
                                  "中心辅助比例 / Center Subscale",
                                  "循环演化 / Cycle Evolution"};
constexpr bool Parameters::*bools[] = {
    &Parameters::invert, &Parameters::uniform_scale,
    &Parameters::perspective_offset, &Parameters::center_subscale,
    &Parameters::cycle_evolution};
constexpr const char *fractals[] = {"基本 / Basic",
                                    "湍流平滑 / Turbulent Smooth",
                                    "湍流基本 / Turbulent Basic",
                                    "湍流锐化 / Turbulent Sharp",
                                    "动态 / Dynamic",
                                    "动态渐进 / Dynamic Progressive",
                                    "动态扭曲 / Dynamic Twist",
                                    "最大 / Max",
                                    "涂抹 / Smeary",
                                    "旋涡 / Swirly",
                                    "岩石 / Rocky",
                                    "多云 / Cloudy",
                                    "地形 / Terrain",
                                    "子缩放 / Subscale",
                                    "小凸起 / Small Bumps",
                                    "绳状 / Strings",
                                    "线状 / Threads"};
constexpr const char *noises[] = {"块 / Block", "线性 / Linear",
                                  "柔和线性 / Soft Linear", "样条 / Spline"};
constexpr const char *overflows[] = {"剪切 / Clip", "柔和限制 / Soft Clamp",
                                     "折回 / Wrap Back",
                                     "允许 HDR / Allow HDR Results"};
constexpr const char *blends[] = {"正常 / Normal",
                                  "无 / None",
                                  "溶解 / Dissolve",
                                  "动态溶解 / Dancing Dissolve",
                                  "变暗 / Darken",
                                  "正片叠底 / Multiply",
                                  "颜色加深 / Color Burn",
                                  "线性加深 / Linear Burn",
                                  "深色 / Darker Color",
                                  "变亮 / Lighten",
                                  "滤色 / Screen",
                                  "颜色减淡 / Color Dodge",
                                  "相加 / Add",
                                  "浅色 / Lighter Color",
                                  "叠加 / Overlay",
                                  "柔光 / Soft Light",
                                  "强光 / Hard Light",
                                  "亮光 / Vivid Light",
                                  "线性光 / Linear Light",
                                  "点光 / Pin Light",
                                  "实色混合 / Hard Mix",
                                  "差值 / Difference",
                                  "排除 / Exclusion",
                                  "相减 / Subtract",
                                  "相除 / Divide",
                                  "色相 / Hue",
                                  "饱和度 / Saturation",
                                  "颜色 / Color",
                                  "明度 / Luminosity"};
constexpr const char *combo_labels[] = {"分形类型 / Fractal",
                                        "杂色类型 / Noise", "溢出 / Overflow",
                                        "混合模式 / Blend"};

struct Context {
  Parameters p, defaults;
  FilterRecord *host;
  psui::ProxyImage source;
  psui::PreviewView view;
  psui::PreviewBackground background;
  std::vector<std::uint8_t> output, display;
  Region region;
  bool dirty = true, preview = true, fill_opacity = false;
  ULONGLONG update_at = 0;
  OSErr error = noErr;
  Context(Parameters v, FilterRecord *f) : p(v), host(f) {
    defaults.offset_x = f->bigDocumentData
                            ? f->bigDocumentData->imageSize32.h / 2.0
                            : f->imageSize.h / 2.0;
    defaults.offset_y = f->bigDocumentData
                            ? f->bigDocumentData->imageSize32.v / 2.0
                            : f->imageSize.v / 2.0;
  }
};
bool initialize(Context &c) {
  auto *f = c.host;
  // Matches editable-transparency PiPL cases: InWhiteZap + OutFillMask.
  // Photoshop preprocesses invisible RGB to white and creates output opacity.
  c.fill_opacity = f->filterCase == filterCaseEditableTransparencyNoSelection ||
                   f->filterCase == filterCaseEditableTransparencyWithSelection;
  c.region.center_x = (f->bigDocumentData ? f->bigDocumentData->imageSize32.h
                                          : f->imageSize.h) /
                      2.0;
  c.region.center_y = (f->bigDocumentData ? f->bigDocumentData->imageSize32.v
                                          : f->imageSize.v) /
                      2.0;
  return true;
}
void update(psui::LibraryPanel &ui, Context &c) {
  if ((!c.dirty && !c.view.dirty) || GetTickCount64() < c.update_at)
    return;
  if (c.view.dirty) {
    c.error = psui::host_read_proxy_area(c.host, c.view.proxy_side(),
                                         c.view.region(), c.source);
    if (c.error != noErr) {
      ui.close(false);
      return;
    }
    c.region.x = c.source.origin_x;
    c.region.y = c.source.origin_y;
    c.region.step = c.source.step;
    c.output.resize(c.source.pixels.size());
    c.display.resize(static_cast<std::size_t>(c.source.width) *
                     c.source.height * 3);
    c.view.dirty = false;
  }
  auto &s = c.source;
  if (s.pixels.empty()) {
    c.dirty = false;
    return;
  }
  c.background.prepare(c.host,s);
  if (c.preview &&
      !process(s.pixels.data(), s.pixels.size(), s.width * s.channels,
               c.output.data(), c.output.size(), s.width * s.channels, s.width,
               s.height, s.channels, c.p, c.region))
    return;
  for (int y = 0; y < s.height; ++y)
    for (int x = 0; x < s.width; ++x) {
      const auto i = static_cast<std::size_t>(y * s.width + x),
                 p = i * s.channels;
      const int alpha = s.channels == 4 ? s.pixels[p + 3] : 255,
                mask = s.mask.empty() ? 255 : s.mask[i],
                bg = ((x / 8 + y / 8) & 1) ? 156 : 200;
      if(c.background.enabled){
        const std::array<float,4> original{s.pixels[p]/255.f,s.pixels[p+1]/255.f,s.pixels[p+2]/255.f,alpha/255.f};
        auto processed=original;
        if(c.preview){for(int k=0;k<3;++k)processed[k]=c.output[p+k]/255.f;if(c.fill_opacity)processed[3]=1;}
        const auto pixel=c.background.pixel(x,y,original,processed,mask/255.f);
        for(int k=0;k<3;++k)c.display[i*3+k]=pixel[k];
        continue;
      }
      for (int k = 0; k < 3; ++k)
        c.display[i * 3 + k] =
            psui::preview_channel(s.pixels[p + k], c.output[p + k], alpha, mask,
                                  bg, c.preview, c.fill_opacity);
    }
  if (!ui.preview(c.host, c.display.data(), s.width, s.height)) {
    c.error = memFullErr;
    ui.close(false);
  }
  c.dirty = false;
  c.update_at = GetTickCount64() + 35;
}
void draw(psui::LibraryPanel &ui, Context &c) {
 ui.parameter_defaults(c.p, c.defaults);
  const auto scale = ui.scale();
  bool changed = false;
  ImGui::BeginChild("preview-side", {190 * scale, 0}, 0,
                    ImGuiWindowFlags_NoScrollbar);
  const float view_height = std::clamp(
      ImGui::GetContentRegionAvail().y - 217 * scale, 100 * scale, 190 * scale);
  c.view.configure(psui::filter_bounds(c.host), 190 * scale, view_height);
  update(ui, c);
  ui.image_view(c.view, c.source);
  ImGui::Spacing();
  changed |= ui.check("预览 / Preview", c.preview);
  if(psui::preview_background_control(ui,c.background))c.dirty=true;
  ui.preview_zoom(c.view);
  ImGui::SetCursorPosY(
      std::max(ImGui::GetCursorPosY(), ImGui::GetWindowHeight() - 110 * scale));
  if (ui.button("ok", "确定 / OK", -1) && valid(c.p))
    ui.close(true);
  if (ui.button("cancel", "取消 / Cancel", -1))
    ui.close(false);
  if (ui.button("reset", "全部重置 / Reset All", -1)) {
    c.p = c.defaults;
    changed = true;
  }
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("parameters", {0, 0}, 0,
                    ImGuiWindowFlags_AlwaysVerticalScrollbar);
  auto n = [&](int i, bool enabled = true) {
    ui.number_default(nums[i].text, nums[i].member ? c.defaults.*nums[i].member : double(c.defaults.seed));
    double value =
        nums[i].member ? c.p.*nums[i].member : static_cast<double>(c.p.seed);
    const bool ch = ui.number(nums[i].text, value, nums[i].lo, nums[i].hi,
                              nums[i].slider_lo, nums[i].slider_hi, enabled,
                              i == 5 || i == 17);
    if (ch) {
      if (nums[i].member)
        c.p.*nums[i].member = value;
      else
        c.p.seed = static_cast<std::uint32_t>(value);
    }
    changed |= ch;
  };
  auto b = [&](int i) { changed |= ui.check(checks[i], c.p.*bools[i]); };
  auto combo = [&](int i, const auto &values) {
    ui.choice_default(combo_labels[i], i==0 ? int(c.defaults.fractal) : i==1 ? int(c.defaults.noise) : i==2 ? int(c.defaults.overflow) : int(c.defaults.blend));
    int value = i == 0   ? static_cast<int>(c.p.fractal)
                : i == 1 ? static_cast<int>(c.p.noise)
                : i == 2 ? static_cast<int>(c.p.overflow)
                         : static_cast<int>(c.p.blend);
    if (ui.choice(combo_labels[i], value, values,
                  static_cast<int>(std::size(values)))) {
      if (i == 0)
        c.p.fractal = static_cast<FractalType>(value);
      else if (i == 1)
        c.p.noise = static_cast<NoiseType>(value);
      else if (i == 2)
        c.p.overflow = static_cast<Overflow>(value);
      else
        c.p.blend = static_cast<Blend>(value);
      changed = true;
    }
  };
  if (ui.section(groups[0], true)) {
    combo(0, fractals);
    combo(1, noises);
    b(0);
    n(0);
    n(1);
    combo(2, overflows);
    n(3);
  }
  if (ui.section(groups[1])) {
    n(7);
    b(1);
    n(2, c.p.uniform_scale);
    n(12, !c.p.uniform_scale);
    n(13, !c.p.uniform_scale);
    n(8);
    n(9);
    b(2);
  }
  if (ui.section(groups[2])) {
    n(10);
    n(11);
    n(14);
    n(15);
    n(16);
    b(3);
  }
  if (ui.section(groups[3])) {
    n(4);
    b(4);
    n(17, c.p.cycle_evolution);
    n(5);
  }
  if (ui.section(groups[4], true)) {
    n(6);
    combo(3, blends);
  }
  ImGui::EndChild();
  if (changed)
    c.dirty = true;
}
} // namespace
INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord *host,
                    Parameters &p) {
  try {
    Context c(p, host);
    if (!initialize(c))
      return c.error;
    std::string glyphs;
    for (const auto &n : nums) {
      glyphs += n.text;
      glyphs += ' ';
    }
    auto add = [&](const auto &labels) {
      for (const auto *text : labels) {
        glyphs += text;
        glyphs += ' ';
      }
    };
    add(groups);
    add(checks);
    add(fractals);
    add(noises);
    add(overflows);
    add(blends);
    add(combo_labels);
    const auto result = psui::run_library_panel(
        module, owner, host, L"Fractal Noise 分形杂色", 680, 480,
        [&](psui::LibraryPanel &ui) { draw(ui, c); }, glyphs.c_str(),
        &plugin_info);
    if (c.error != noErr)
      return c.error;
    if (result == IDOK)
      p = c.p;
    return result;
  } catch (...) {
    return memFullErr;
  }
}
} // namespace fractal
