#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "psui/adaptive_preview.hpp"
#include "tint_dialog.hpp"
#include "tint_info.hpp"
#include <algorithm>
#include <vector>
namespace tint {
namespace {
struct PreviewFrame {std::shared_ptr<psui::ProxyImage> input;std::vector<std::uint8_t> output;};
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
  bool display_dirty=true;
  std::shared_ptr<psui::ProxyImage> full_input,draft_input;int draft_side=0;
  psui::AdaptivePreview<PreviewFrame> worker;
  Context(const Parameters &p, FilterRecord *h) : values(p), host(h) {}
};
void update(psui::LibraryPanel &ui, Context &c) {
  if(c.view.dirty||c.draft_side!=c.worker.draft_side()){c.draft_side=c.worker.draft_side();
    auto full=std::make_shared<psui::ProxyImage>(),small=std::make_shared<psui::ProxyImage>();
    c.error=psui::host_read_proxy_area(c.host,c.view.proxy_side(),c.view.region(),*full);
    if(!c.error)c.error=psui::host_read_proxy_area(c.host,std::min(c.draft_side,c.view.proxy_side()),c.view.region(),*small);
    if(c.error){ui.close(false);return;}
    c.full_input=std::move(full);c.draft_input=std::move(small);c.view.dirty=false;c.dirty=true;
  }
  const auto p=c.values;const auto full=c.full_input,small=c.draft_input;
  try{PreviewFrame frame;if(c.worker.update(c.dirty,frame,[p,full,small](bool draft,const auto& cancel){
    PreviewFrame frame;frame.input=draft?small:full;const auto input=frame.input;
    if(!input||cancel())return frame;frame.output.resize(input->pixels.size());
    ConstImage src{input->pixels.data(),input->pixels.size(),std::size_t(input->width),std::size_t(input->height),std::size_t(input->width*input->channels),unsigned(input->channels),SampleType::u8};
      Image dst{frame.output.data(),frame.output.size(),src.width,src.height,src.row_bytes,src.components,SampleType::u8};process(src,dst,p);
    return frame;
  })){if(frame.input){c.source=*frame.input;c.processed=std::move(frame.output);c.display.resize(std::size_t(c.source.width)*c.source.height*3);c.display_dirty=true;}}}
  catch(...){c.error=memFullErr;ui.close(false);return;}
  if(c.source.pixels.empty()||!c.display_dirty)return;
  const auto &src=c.source;
  c.background.prepare(c.host,src);
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
  c.display_dirty=false;
}
void draw(psui::LibraryPanel &ui, Context &c) {
 ui.parameter_defaults(c.values, Parameters{});
  const auto s = ui.scale();
  bool changed = false;
  ui.begin_filter_preview(c.view, psui::filter_bounds(c.host));
  update(ui, c);
  ui.image_view(c.view, c.source);
  changed |= ui.check("预览 / Preview", c.preview);
  c.worker.controls(ui,c.dirty);
  if(psui::preview_background_control(ui,c.background))c.dirty=true;
  ui.preview_zoom(c.view);
  ImGui::EndChild();
  ImGui::SameLine();
  ui.begin_filter_parameters(ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse);
  const float footer = ui.filter_footer_height();
  ImGui::BeginChild("tint-controls", {0, -footer}, 0,
                    ImGuiWindowFlags_NoScrollWithMouse);
  for (int i = 0; i < 2; ++i) {
    auto *color = i == 0 ? c.values.black : c.values.white;
    ImGui::PushID(i);
    if (ui.begin_parameter_row("mapping")) {
      ImGui::TableNextColumn();
      changed |= ui.parameter_label(i == 0 ? "将黑色映射到 / Map Black To" : "将白色映射到 / Map White To",
                                    color, 3 * sizeof(double));
      ImGui::TableNextColumn();
      changed |= ui.color_swatch(i == 0 ? "black" : "white", c.host, color, {64*s, 22*s});
      ImGui::EndTable();
    }
    ImGui::PopID();
  }
  ui.number_default("着色量 / Amount", Parameters{}.amount * 100);
  double amount = c.values.amount * 100;
  if (ui.number("着色量 / Amount", amount, 0, 100, 0, 100)) {
    c.values.amount = amount / 100;
    changed = true;
  }
  if (ui.button("swap", "交换颜色 / Swap Colors")) {
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
  ui.end_filter_parameters();
  if (changed)
    c.dirty = true;
}
} // namespace
INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord *host,
                    Parameters &p) {
  try {
    Context c{p, host};
    const auto result = psui::run_filter_panel(
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
