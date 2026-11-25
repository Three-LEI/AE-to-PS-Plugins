#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "noise_host.hpp"
#include "psui/adaptive_preview.hpp"
#include "psui/generator_controls.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include <algorithm>
#include <iterator>
#include <optional>

namespace noise {
namespace {
struct Context {
    Parameters values;
    FilterRecord* host;
    const Image& source;
    Image last;
    psui::PreviewView view;
    psui::ProxyImage display_geometry, selection_geometry;
    psui::PreviewBackground background;
    psui::PreviewSpace mapping;
    std::vector<float> selection;
    std::vector<std::uint8_t> display;
    psui::PreviewAreaCache area_cache;
    psui::DraftImageCache<Image> cache;
    psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
    std::optional<Region> requested_region, last_region;
    bool dirty = true, preview = true, display_dirty = true;
    int preset_index = 0;
    OSErr error = noErr;

    Context(const Parameters& p, FilterRecord* h, const Image& src) : values(p), host(h), source(src) {}
};

std::optional<Region> native_viewport(const Context& c) {
    if (c.view.percent < 100) return {};
    const auto roi = c.view.region();
    auto relative = [](int position, int origin, int extent) {
        return int(std::clamp(std::int64_t(position) - origin, std::int64_t(0), std::int64_t(extent)));
    };
    const int x0 = relative(roi.left, c.source.left, c.source.width);
    const int y0 = relative(roi.top, c.source.top, c.source.height);
    const int x1 = relative(roi.right, c.source.left, c.source.width);
    const int y1 = relative(roi.bottom, c.source.top, c.source.height);
    if (x1 <= x0 || y1 <= y0) return {};
    return Region{x0, y0, x1 - x0, y1 - y0};
}

void update_preview(psui::LibraryPanel& ui, Context& c) {
    const Parameters p = c.values;
    const auto region = native_viewport(c);
    if (region != c.requested_region) {
        c.worker.invalidate();
        c.requested_region = region;
        c.dirty = c.display_dirty = true;
    }
    try {
        psui::SpatialFrame<Image> frame;
        if (c.worker.update(c.dirty, frame, [&c, p, region](psui::PreviewPass pass, const Cancel& cancel) {
                psui::SpatialFrame<Image> next;
                if (region) {
                    render_region(c.source, p, *region, next.image, cancel);
                    next.space = {double(next.image.left), double(next.image.top), 1};
                } else {
                    next.space = psui::preview_space(c.source, pass);
                    const auto& input = c.cache.get(c.source, next.space, cancel);
                    render(input, p, next.image, cancel, {next.space.left, next.space.top, next.space.step});
                }
                return next;
            }, region.has_value())) {
            if (frame.image.valid()) {
                c.last = std::move(frame.image);
                c.mapping = frame.space;
                c.last_region = region;
                c.display_dirty = true;
                c.error = noErr;
            } else {
                c.error = filterBadParameters;
            }
        }
        psui::compose_spatial_preview(ui, c.host, c.view, c.display_geometry, c.background,
            c.selection, c.selection_geometry, c.display, c.source, c.last,
            c.mapping, c.preview && c.last_region == region, c.display_dirty, c.error, nullptr, false, &c.area_cache);
    } catch (...) {
        c.error = memFullErr;
    }
}

void draw(psui::LibraryPanel& ui, Context& c) {
    const Parameters factory;
    auto& p = c.values;
    ui.parameter_defaults(p, factory);
    const float footer = ui.filter_footer_height();
    bool changed = false;

    ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);
    update_preview(ui, c);
    ui.image_view(c.view, c.display_geometry);
    if (ui.check("预览 / Preview", c.preview)) c.display_dirty = true;
    c.worker.controls(ui, c.dirty);
    if (psui::preview_background_control(ui, c.background)) c.display_dirty = true;
    ui.preview_zoom(c.view);
    if (c.error) {
        ImGui::TextWrapped("%s", ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.f, ImGui::GetContentRegionAvail().y - footer));
    ImGui::BeginDisabled(c.error != noErr);
    if (ui.button("ok", "确定 / OK", -1)) ui.close(true);
    ImGui::EndDisabled();
    if (ui.button("cancel", "取消 / Cancel", -1)) ui.close(false);
    if (ui.button("reset", "全部重置 / Reset All", -1)) {
        c.worker.stop();
        p = factory;
        c.preset_index = 0;
        changed = true;
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("parameters", {0, 0});
    constexpr const char* presets[] = {
        "自定义 / Custom", "细微杂色 / Fine Noise", "柔和胶片 / Soft Film",
        "电视雪花 / TV Snow", "VHS 干扰 / VHS Interference", "胶片与 VHS / Film and VHS",
        "粗颗粒胶片 / Coarse Film"
    };
    if (ui.choice("预设 / Preset", c.preset_index, presets, int(std::size(presets))) && c.preset_index > 0) {
        Parameters next = preset(Preset(c.preset_index - 1));
        next.seed = p.seed;
        next.phase = p.phase;
        p = next;
        c.dirty = true;
        c.error = noErr;
        changed = false;
    }

    auto percent = [&](const char* label, double& value, double slider_max = 100) {
        changed |= ui.number(label, value, 0, 100, 0, slider_max);
    };
    auto integer = [&](const char* label, int& value) {
        ui.inherit_number_default(label, value);
        double number = value;
        if (ui.number(label, number, 0, 999999, 0, 999, true, true)) {
            value = int(number);
            changed = true;
        }
    };
    integer("随机种子 / Seed", p.seed);
    integer("相位 / Phase", p.phase);
    percent("效果混合（%） / Effect Mix (%)", p.mix);

    changed |= ui.check("基础杂色 / Basic Noise", p.basic);
    if (p.basic) {
        constexpr const char* distributions[] = {"平均（均匀） / Uniform", "高斯 / Gaussian"};
        percent("杂色数量（%） / Noise Amount (%)", p.amount, 30);
        changed |= ui.choice("分布 / Distribution", p.distribution, distributions, 2);
        changed |= ui.number("噪点尺寸（像素） / Noise Size (px)", p.size, 1, 64, 1, 12);
        percent("彩色杂色（%） / Color Noise (%)", p.color);
        if (ui.section("色相、明度和饱和度 / Hue, Lightness and Saturation")) {
            percent("色相杂色（%） / Hue Noise (%)", p.hue);
            percent("明度杂色（%） / Lightness Noise (%)", p.lightness);
            percent("饱和度杂色（%） / Saturation Noise (%)", p.saturation);
        }
    }
    changed |= ui.check("胶片颗粒 / Film Grain", p.grain);
    if (p.grain) {
        percent("颗粒强度（%） / Grain Amount (%)", p.grain_amount, 40);
        changed |= ui.number("颗粒尺寸（像素） / Grain Size (px)", p.grain_size, .25, 40, .25, 8);
        percent("颗粒柔和度（%） / Grain Softness (%)", p.grain_softness);
        percent("颗粒粗糙度（%） / Grain Roughness (%)", p.grain_roughness);
        percent("彩色颗粒（%） / Grain Color (%)", p.grain_color);
        percent("暗部颗粒（%） / Shadow Grain (%)", p.shadows);
        percent("中间调颗粒（%） / Midtone Grain (%)", p.midtones);
        percent("亮部颗粒（%） / Highlight Grain (%)", p.highlights);
        changed |= ui.number("中间调位置（%） / Grain Midpoint (%)", p.grain_midpoint, 1, 99, 1, 99);
    }
    changed |= ui.check("模拟信号 / Analog Signal", p.signal);
    if (p.signal) {
        percent("电视雪花（%） / TV Snow (%)", p.snow);
        percent("横向干扰（%） / Line Interference (%)", p.bands);
        changed |= ui.number("色度拖尾（像素） / Chroma Trail (px)", p.chroma_bleed, 0, 128, 0, 32);
        percent("局部掉讯（%） / Dropouts (%)", p.dropouts);
    }
    ImGui::EndChild();

    if (changed) {
        c.preset_index = 0;
        c.dirty = true;
        c.error = noErr;
    }
}
}

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source, Parameters& p) {
    try {
        Context c(p, host, source);
        constexpr auto glyphs = "综合杂色预设自定义细微柔和胶片电视雪花干扰与随机种子相位基础"
            "平均匀高斯数量分布噪点尺寸像素彩明饱度颗粒强暗部中间调亮模拟信号横向拖尾局掉效果混合粗糙"
            "预览失败请重置或取消确定全部显示背景适合精细快速";
        const auto result = psui::run_filter_panel(module, owner, host, title, 1040, 820,
            [&](psui::LibraryPanel& ui) { draw(ui, c); }, glyphs, &plugin_info);
        c.worker.stop();
        if (result == IDOK && c.error == noErr) p = c.values;
        return result == IDOK && c.error != noErr ? c.error : result;
    } catch (...) {
        return memFullErr;
    }
}
}
