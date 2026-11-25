#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/adaptive_preview.hpp"
#include "psui/generator_controls.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "repetile_host.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace repetile {
namespace {
// Render a full source snapshot; zoom and pan only change the displayed crop.
struct Context {
    Parameters values;
    FilterRecord* host;
    const Image& source;
    const pscpu::Rect tile;
    Image last;
    psui::ProxyImage display_geometry, selection_geometry;
    psui::PreviewView view;
    psui::PreviewBackground background;
    std::vector<std::uint8_t> display;
    std::vector<float> selection;
    bool dirty = true, preview = true, display_dirty = true;
    OSErr error = noErr;
    psui::PreviewSpace mapping;
    psui::DraftImageCache<Image> cache;
    psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
    Context(const Parameters& p, FilterRecord* h, const Image& src)
        : values(p), host(h), source(src), tile(tile_bounds(src)) {}
};

void update(psui::LibraryPanel& ui, Context& c) {
    const auto p = c.values;
    try {
        psui::SpatialFrame<Image> frame;
        if (c.worker.update(c.dirty, frame, [&c,p](psui::PreviewPass pass, const Cancel& cancel) {
                psui::SpatialFrame<Image> next;
                next.space = psui::preview_space(c.source, pass);
                const auto& input = c.cache.get(c.source, next.space, cancel);
                if (cancel()) return next;
                auto scaled = p;
                scaled.left /= next.space.step; scaled.right /= next.space.step;
                scaled.up /= next.space.step; scaled.down /= next.space.step;
                scaled.blend /= next.space.step;
                render(input, scaled, next.image, cancel);
                return next;
            })) {
            if (frame.image.valid()) {
                c.last = std::move(frame.image);
                c.mapping = frame.space;
                c.display_dirty = true;
                c.error = noErr;
            } else c.error = filterBadParameters;
        }
    } catch (...) {
        c.error = memFullErr;
    }
    psui::compose_spatial_preview(ui, c.host, c.view, c.display_geometry, c.background,
        c.selection, c.selection_geometry, c.display, c.source, c.last,
        c.mapping, c.preview, c.display_dirty, c.error);
}

void draw(psui::LibraryPanel& ui, Context& c) {
    Parameters factory;
    ui.parameter_defaults(c.values, factory);

    bool changed = false;
    ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host));
    update(ui, c);
    const auto& tile = c.tile;
    auto& p = c.values;
    psui::PreviewAdjustments handles;
    if (!tile.empty()) {
        handles.point("左上扩展 / Expand Top Left", p.left, p.up, {double(tile.left), double(tile.top), -1, -1, 0, 100000})
               .point("右下扩展 / Expand Bottom Right", p.right, p.down, {double(tile.right), double(tile.bottom), 1, 1, 0, 100000});
    }
    changed |= ui.image_view(c.view, c.display_geometry, handles);
    if (ui.check("预览 / Preview", c.preview)) c.display_dirty = true;
    c.worker.controls(ui, c.dirty);
    if (psui::preview_background_control(ui, c.background)) c.display_dirty = true;
    ui.preview_zoom(c.view);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("parameters", {0, 0}, 0, ImGuiWindowFlags_NoScrollbar);
    const float footer = ui.filter_footer_height();
    ImGui::BeginChild("repetile-controls", {0, -footer}, 0, ImGuiWindowFlags_NoScrollWithMouse);
    changed |= ui.number("向左扩展（像素） / Expand Left (px)", c.values.left, 0, 100000, 0, 500);
    changed |= ui.number("向右扩展（像素） / Expand Right (px)", c.values.right, 0, 100000, 0, 500);
    changed |= ui.number("向上扩展（像素） / Expand Up (px)", c.values.up, 0, 100000, 0, 500);
    changed |= ui.number("向下扩展（像素） / Expand Down (px)", c.values.down, 0, 100000, 0, 500);
    const char* tilings[] = {"重复 / Repeat", "水平翻转 / Checker Flip H", "垂直翻转 / Checker Flip V",
                             "展开 / Unfold", "180°翻转 / Checker 180", "随机 / Random", "无 / None",
                             "滑动 / Slide", "砖块 / Brick"};
    changed |= ui.choice("拼贴模式 / Tiling", c.values.tiling, tilings, 9);
    ImGui::BeginDisabled(c.values.tiling == Unfold || c.values.tiling == None || c.values.tiling == Slide);
    changed |= ui.number("接缝混合（像素） / Blend Borders (px)", c.values.blend, 0, 1000, 0, 100);
    ImGui::EndDisabled();
    ImGui::BeginDisabled(c.values.tiling != Random);
    ui.inherit_number_default("随机种子 / Seed", c.values.seed);
    double seed = c.values.seed;
    if (ui.number("随机种子 / Seed", seed, 0, 999999, 0, 999, true, true)) {
        c.values.seed = int(seed);
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::TextWrapped("%s", ui.text("以图层中有内容的区域为瓷砖，向四边扩展；需要透明边距或先扩大画布/图层。 / Tiles the visible content outward; transparent margins or a larger canvas/layer are required.").c_str());
    ImGui::TextWrapped("%s", ui.text("接缝混合在展开/无/滑动模式下不生效。 / Blend Borders is disabled for Unfold, None and Slide.").c_str());
    ImGui::EndChild();
    ImGui::BeginDisabled(c.error != noErr);
    if (ui.button("ok", "确定 / OK", -1)) ui.close(true);
    ImGui::EndDisabled();
    if (ui.button("cancel", "取消 / Cancel", -1)) ui.close(false);
    if (ui.button("reset", "全部重置 / Reset All", -1)) {
        c.worker.stop();
        c.error = noErr;
        c.values = factory;
        c.dirty = true;
    }
    ImGui::EndChild();
    if (changed) c.dirty = true;
}
} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source, Parameters& p) {
    try {
        Context c{p, host, source};
        const auto glyphs = "重复拼贴 向左扩展 向右扩展 向上扩展 向下扩展 像素 拼贴模式 重复 水平翻转 垂直翻转 展开 翻转 随机 无 滑动 砖块 接缝混合 随机种子 预览 显示背景 适合 确定 取消 全部重置 以图层中有内容的区域为瓷砖 向四边扩展 需要透明边距或先扩大画布 图层 在展开 无 滑动模式下不生效";
        const auto answer = psui::run_filter_panel(module, owner, host, title, 800, 620,
                                                    [&](psui::LibraryPanel& ui) { draw(ui, c); },
                                                    glyphs, &plugin_info);
        c.worker.stop();
        if (c.error != noErr) return c.error;
        if (answer == IDOK) p = c.values;
        return answer;
    } catch (...) {
        return memFullErr;
    }
}
} // namespace repetile
