#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "tint_dialog.hpp"
#include "tint_info.hpp"
#include <algorithm>
#include <vector>
namespace tint {
namespace {
struct Context {
  Parameters values;
  FilterRecord *host;
  psui::ProxyImage source;
  psui::PreviewView view;
  psui::PreviewBackground background;
  std::vector<std::uint8_t> processed, display;
  bool dirty = true, preview = true;
  ULONGLONG update_at = 0;
  OSErr error = noErr;
  Context(const Parameters &p, FilterRecord *h) : values(p), host(h) {}
};
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
    c.processed.resize(c.source.pixels.size());
    c.display.resize(static_cast<std::size_t>(c.source.width) *
                     c.source.height * 3);
    c.view.dirty = false;
  }
  if (c.source.pixels.empty()) {
    c.dirty = false;
    return;
  }
  const auto &src = c.source;
  c.background.prepare(c.host,src);
  ConstImage input{src.pixels.data(),
                   src.pixels.size(),
                   static_cast<std::size_t>(src.width),
                   static_cast<std::size_t>(src.height),
                   static_cast<std::size_t>(src.width * src.channels),
                   static_cast<unsigned>(src.channels),
                   SampleType::u8};
  Image output{c.processed.data(), c.processed.size(), input.width,
               input.height,       input.row_bytes,    input.components,
               SampleType::u8};
  if (c.preview)
    process(input, output, c.values);
  for (int y = 0; y < src.height; ++y)
    for (int x = 0; x < src.width; ++x) {
      const auto i = static_cast<std::size_t>(y * src.width + x),
                 p = i * src.channels;
      const int alpha = src.channels == 4 ? src.pixels[p + 3] : 255,
                mask = src.mask.empty() ? 255 : src.mask[i],
                bg = ((x / 8 + y / 8) & 1) ? 156 : 200;
      if(c.background.enabled){
        const std::array<float,4> original{src.pixels[p]/255.f,src.pixels[p+1]/255.f,src.pixels[p+2]/255.f,alpha/255.f};
        auto processed=original;
        if(c.preview)for(int k=0;k<3;++k)processed[k]=c.processed[p+k]/255.f;
        const auto pixel=c.background.pixel(x,y,original,processed,mask/255.f);
        for(int k=0;k<3;++k)c.display[i*3+k]=pixel[k];
        continue;
      }
      for (int k = 0; k < 3; ++k) {
        const int original = src.pixels[p + k],
                  v = c.preview ? (original * (255 - mask) +
                                   c.processed[p + k] * mask + 127) /
                                      255
                                : original;
        c.display[i * 3 + k] = static_cast<std::uint8_t>(
            (v * alpha + bg * (255 - alpha) + 127) / 255);
      }
    }
  if (!ui.preview(c.host, c.display.data(), src.width, src.height)) {
    c.error = memFullErr;
    ui.close(false);
  }
  c.dirty = false;
  c.update_at = GetTickCount64() + 25;
}
void draw(psui::LibraryPanel &ui, Context &c) {
 ui.parameter_defaults(c.values, Parameters{});
  const auto s = ui.scale();
  bool changed = false;
  ImGui::BeginChild("preview-side", {190 * s, 0}, 0,
                    ImGuiWindowFlags_NoScrollbar);
  const float preview_height = std::clamp(
      ImGui::GetContentRegionAvail().y - 112 * s, 100 * s, 190 * s);
  c.view.configure(psui::filter_bounds(c.host), 190 * s, preview_height);
  update(ui, c);
  ui.image_view(c.view, c.source);
  changed |= ui.check("预览 / Preview", c.preview);
  if(psui::preview_background_control(ui,c.background))c.dirty=true;
  ui.preview_zoom(c.view);
  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("parameters", {0, 0}, 0,
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse);
  const float footer = 3 * (ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y);
  ImGui::BeginChild("tint-controls", {0, -footer}, 0,
                    ImGuiWindowFlags_NoScrollWithMouse);
  for (int i = 0; i < 2; ++i) {
    auto *color = i == 0 ? c.values.black : c.values.white;
    changed |= ui.parameter_label(i == 0 ? "将黑色映射到 / Map Black To" : "将白色映射到 / Map White To",color,3*sizeof(double));
    ImGui::PushID(i);
    changed |= ui.color_swatch(i == 0 ? "black" : "white",c.host,color,{64*s,22*s});
    ImGui::PopID();
  }
  ui.number_default("着色量 / Amount", Parameters{}.amount * 100);
  double amount = c.values.amount * 100;
  if (ui.number("着色量 / Amount", amount, 0, 100, 0, 100)) {
    c.values.amount = amount / 100;
    changed = true;
  }
  if (ui.button("swap", "交换颜色 / Swap Colors", -1)) {
    for (int k = 0; k < 3; ++k)
      std::swap(c.values.black[k], c.values.white[k]);
    changed = true;
  }
  ImGui::EndChild();
  if (ui.button("ok", "确定 / OK", -1))
    ui.close(true);
  if (ui.button("cancel", "取消 / Cancel", -1))
    ui.close(false);
  if (ui.button("reset", "全部重置 / Reset All", -1)) {
    c.values = Parameters{};
    changed = true;
  }
  ImGui::EndChild();
  if (changed)
    c.dirty = true;
}
} // namespace
INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord *host,
                    Parameters &p) {
  try {
    Context c{p, host};
    const auto result = psui::run_library_panel(
        module, owner, host, L"Tint 色调", 640, 350,
        [&](psui::LibraryPanel &ui) { draw(ui, c); }, nullptr, &plugin_info);
    if (c.error != noErr)
      return c.error;
    if (result == IDOK)
      p = c.values;
    return result;
  } catch (...) {
    return memFullErr;
  }
}
} // namespace tint
