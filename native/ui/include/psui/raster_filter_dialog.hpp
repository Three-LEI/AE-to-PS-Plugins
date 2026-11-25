#pragma once

#include "adaptive_preview.hpp"
#include "../../../common/raster_image.hpp"

namespace psui {

struct NoPreviewAdjustments {
    PreviewAdjustments operator()(auto&, const PreviewView&) const { return {}; }
};

// For current-layer filters whose preview and native output share a region renderer.
template<class Parameters, class DrawControls, class Render, class MakeAdjustments = NoPreviewAdjustments>
INT_PTR run_raster_filter_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const wchar_t* title,
                                 const PluginInfo& info, const char* glyphs, const pscpu::Image& source,
                                 Parameters& parameters, DrawControls controls, Render&& render, MakeAdjustments make_adjustments = {}) {
    using pscpu::Image;
    using pscpu::Rect;
    try {
        const Parameters factory;
        Parameters values = parameters;
        PreviewView view;
        ProxyImage geometry, selection_geometry;
        PreviewBackground background;
        PreviewAreaCache area_cache;
        std::vector<float> selection;
        std::vector<std::uint8_t> display;
        DraftImageCache<Image> cache;
        SpatialFrame<Image> last;
        Rect requested, rendered;
        bool native_requested = false;
        bool dirty = true, display_dirty = true, preview = true;
        OSErr error = noErr;
        AdaptivePreview<SpatialFrame<Image>> worker;

        const auto update = [&](LibraryPanel& ui) {
            const auto bounds = source.bounds();
            const auto view_rect = view.region();
            const auto viewport = pscpu::intersect(bounds, {view_rect.left, view_rect.top, view_rect.right, view_rect.bottom});
            const bool native = view.percent >= 100 && !viewport.empty();
            const auto required = native ? viewport : bounds;
            if (native != native_requested || !requested.contains(required)) {
                worker.invalidate();
                requested = required;
                if (native) {
                    // Keep small pans inside the last native frame.
                    const auto dx = std::min(64, required.width() / 4), dy = std::min(64, required.height() / 4);
                    requested.left -= int(std::min<std::int64_t>(dx, std::int64_t(required.left) - bounds.left));
                    requested.top -= int(std::min<std::int64_t>(dy, std::int64_t(required.top) - bounds.top));
                    requested.right += int(std::min<std::int64_t>(dx, std::int64_t(bounds.right) - required.right));
                    requested.bottom += int(std::min<std::int64_t>(dy, std::int64_t(bounds.bottom) - required.bottom));
                }
                native_requested = native;
                dirty = display_dirty = true;
            }
            const auto area = requested;
            SpatialFrame<Image> frame;
            if (worker.update(dirty, frame, [&, p = values, area, native, renderer = render](PreviewPass pass, const pscpu::Cancel& cancel) {
                    const auto space = preview_space(source, pass);
                    const auto& input = cache.get(source, space, cancel);
                    SpatialFrame<Image> next;
                    if (!input.valid()) return next;
                    const auto crop = native ? area : input.bounds();
                    if (renderer(input, p, crop, {space.left, space.top, space.step}, next.image, cancel)) {
                        next.space = {space.left + (next.image.left - input.left) * space.step,
                                      space.top + (next.image.top - input.top) * space.step, space.step};
                    }
                    return next;
                }, native)) {
                if (frame.image.valid()) {
                    last = std::move(frame);
                    rendered = area;
                    error = noErr;
                    display_dirty = true;
                } else {
                    error = filterBadParameters;
                }
            }
            if (preview && !viewport.empty() && !rendered.contains(viewport)) {
                view.dirty = true;
                return;
            }
            compose_spatial_preview(ui, host, view, geometry, background, selection, selection_geometry,
                display, source, last.image, last.space, preview, display_dirty, error, nullptr, false, &area_cache);
        };

        const auto result = run_filter_panel(module, owner, host, title, 940, 720,
            [&](LibraryPanel& ui) {
                ui.parameter_defaults(values, factory);
                const float footer = ui.filter_footer_height();
                ui.begin_filter_preview(view, filter_document_bounds(host), footer);
                update(ui);
                if (ui.image_view(view, geometry, make_adjustments(values, view))) {
                    dirty = true;
                    error = noErr;
                }
                if (ui.check("预览 / Preview", preview)) display_dirty = true;
                worker.controls(ui, dirty);
                if (preview_background_control(ui, background)) display_dirty = true;
                ui.preview_zoom(view);
                if (error) {
                    ImGui::TextWrapped("%s", ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
                }
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.f, ImGui::GetContentRegionAvail().y - footer));
                ImGui::BeginDisabled(error != noErr);
                if (ui.button("ok", "确定 / OK", -1)) ui.close(true);
                ImGui::EndDisabled();
                if (ui.button("cancel", "取消 / Cancel", -1)) ui.close(false);
                if (ui.button("reset", "全部重置 / Reset All", -1)) {
                    values = factory;
                    dirty = true;
                    error = noErr;
                }
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::BeginChild("parameters", {0, 0});
                if (controls(ui, values)) {
                    dirty = true;
                    error = noErr;
                }
                ImGui::EndChild();
            }, glyphs, &info);
        worker.stop();
        if (result != IDOK) return result;
        if (error) return error;
        parameters = values;
        return IDOK;
    } catch (const std::bad_alloc&) {
        return memFullErr;
    } catch (...) {
        return filterBadParameters;
    }
}

} // namespace psui
