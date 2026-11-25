#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "texturewrap_host.hpp"
#include "psui/adaptive_preview.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include <algorithm>

namespace texturewrap {

OSErr load_texture_source(FilterRecord* host, const std::vector<psui::HostLayer>& layers,
                          std::uint64_t key, Inputs& inputs) {
    inputs.canvas = canvas_bounds(host);
    inputs.texture.reset();
    inputs.reference = {};
    if (!key) return noErr;
    psui::LayerPixels pixels;
    const auto error = psfilter::read_named_layer(host, layers, key, pixels);
    if (error) return error;
    inputs.reference = from_host(std::move(pixels));
    return inputs.reference.valid() ? noErr : filterBadParameters;
}

namespace {

struct Context {
    Parameters values;
    FilterRecord* host;
    const Image& source;
    const std::vector<psui::HostLayer>& layers;
    Inputs& inputs;
    Image last;
    Bounds requested, rendered;
    psui::PreviewView view;
    psui::ProxyImage display_geometry, selection_geometry;
    psui::PreviewBackground background;
    psui::PreviewSpace space;
    psui::PreviewAreaCache area_cache;
    std::vector<float> selection;
    std::vector<std::uint8_t> display;
    bool native_requested = false;
    bool dirty = true, display_dirty = true, preview = true;
    OSErr error = noErr, source_error = noErr;
    psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;

    Context(const Parameters& p, FilterRecord* f, const Image& src, const std::vector<psui::HostLayer>& available,
            Inputs& in)
        : values(p), host(f), source(src), layers(available), inputs(in) {}

    void read_source() {
        error = noErr;
        source_error = load_texture_source(host, layers, values.texture_key, inputs);
        if (source_error) return;
        const auto cancel = [&] { return host->abortProc && host->abortProc(); };
        Bounds current, selected;
        const bool external = inputs.reference.valid();
        if ((!inputs.current_texture && !find_texture_bounds(source, current, cancel)) ||
            (external && !find_texture_bounds(inputs.reference, selected, cancel))) {
            source_error = cancel() ? userCanceledErr : filterBadParameters;
            return;
        }
        if (!inputs.current_texture) inputs.current_texture = current;
        inputs.texture = external ? selected : *inputs.current_texture;
    }
};

bool covers(Bounds image, Bounds viewport) {
    return !viewport.empty() && image.left <= viewport.left && image.top <= viewport.top &&
           image.right >= viewport.right && image.bottom >= viewport.bottom;
}

Bounds padded_viewport(Bounds viewport, Bounds source) {
    const auto x = std::min<std::int64_t>(64, viewport.width() / 4);
    const auto y = std::min<std::int64_t>(64, viewport.height() / 4);
    return {int(std::max<std::int64_t>(source.left, viewport.left - x)),
            int(std::max<std::int64_t>(source.top, viewport.top - y)),
            int(std::min<std::int64_t>(source.right, viewport.right + x)),
            int(std::min<std::int64_t>(source.bottom, viewport.bottom + y))};
}

void update_preview(psui::LibraryPanel& ui, Context& c) {
    const Parameters p = c.values;
    const Bounds viewport = intersect(bounds(c.view.region()), c.source.bounds());
    const bool native = c.view.percent >= 100;
    if (!covers(c.requested, viewport) || native != c.native_requested) {
        c.worker.invalidate();
        c.requested = native ? padded_viewport(viewport, c.source.bounds()) : viewport;
        c.native_requested = native;
        c.dirty = c.display_dirty = true;
    }
    const Bounds requested = c.requested;
    const Mapping map = c.inputs.mapping();
    try {
        psui::SpatialFrame<Image> frame;
        if (!c.source_error && c.worker.update(c.dirty, frame, [&c, p, requested, map](psui::PreviewPass pass, const Cancel& cancel) {
                const auto extent = std::max(requested.width(), requested.height());
                const int step = pass.draft ? std::max(1, int((extent + pass.side - 1) / pass.side)) : 1;
                psui::SpatialFrame<Image> next;
                if (render_region(effect_shape, c.source, p, map, requested, step, next.image, cancel)) {
                    next.space = {double(next.image.left), double(next.image.top), double(step)};
                }
                return next;
            }, native)) {
            if (frame.image.valid()) {
                c.last = std::move(frame.image);
                c.space = frame.space;
                c.rendered = requested;
                c.display_dirty = true;
                c.error = noErr;
            } else {
                c.error = filterBadParameters;
            }
        }
        // image_view keeps the previous texture and its transform while the
        // requested view is dirty. Use that until a color frame covers the pan.
        if (c.preview && !covers(c.rendered, viewport)) {
            c.view.dirty = true;
            return;
        }
        psui::compose_spatial_preview(ui, c.host, c.view, c.display_geometry, c.background,
            c.selection, c.selection_geometry, c.display, c.source, c.last, c.space,
            c.preview, c.display_dirty, c.error, nullptr, false, &c.area_cache);
    } catch (...) {
        c.error = memFullErr;
    }
}

bool source_control(psui::LibraryPanel& ui, Context& c) {
    auto& key = c.values.texture_key;
    bool changed = false;
    if (ImGui::BeginTable("texture-source", 2)) {
        ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch, .43f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch, .57f);
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        changed = ui.parameter_label("贴图来源 / Texture Source", key);
        ImGui::TableNextColumn();

        const auto self = ui.text("当前图层 / Current Layer");
        auto selected = self;
        if (key) {
            const auto it = std::find_if(c.layers.begin(), c.layers.end(), [key](const auto& layer) {
                return layer.name_key == key;
            });
            selected = it != c.layers.end() ? it->name : ui.text("图层缺失 / Missing Layer");
        }
        ImGui::SetNextItemWidth(-1);
        const bool open = ImGui::BeginCombo("##layer", selected.c_str());
        ui.mark("texturewrap.source");
        if (open) {
            if (ImGui::Selectable(self.c_str(), !key)) {
                key = 0;
                changed = true;
            }
            ui.mark("texturewrap.source/current");
            if (!key) ImGui::SetItemDefaultFocus();
            for (std::size_t i = 0; i < c.layers.size(); ++i) {
                const auto& layer = c.layers[i];
                if (layer.target) continue;
                ImGui::PushID(int(i));
                const std::string label = layer.name + "###item";
                if (ImGui::Selectable(label.c_str(), key == layer.name_key)) {
                    key = layer.name_key;
                    changed = true;
                }
                ui.mark(("texturewrap.source/layer/" + std::to_string(i)).c_str());
                if (key == layer.name_key) ImGui::SetItemDefaultFocus();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::EndTable();
    }
    return changed;
}

void draw(psui::LibraryPanel& ui, Context& c) {
    const auto factory = defaults(effect_shape);
    auto& p = c.values;
    ui.parameter_defaults(p, factory);
    const float footer = ui.filter_footer_height();
    bool changed = false, reload = false;

    ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);
    update_preview(ui, c);
    const auto canvas = c.inputs.canvas.empty() ? c.source.bounds() : c.inputs.canvas;
    const auto position = psui::PreviewPointSpace::percent({canvas.top, canvas.left, canvas.bottom, canvas.right}, -200, 300);
    const auto cx = position.x(p.center_x), cy = position.y(p.center_y);
    psui::PreviewAdjustments handles;
    handles.point("中心 / Center", p.center_x, p.center_y, position)
           .angle("Z 轴 / Z Rotation", p.rotation_z, cx, cy, -360, 360);
    changed |= ui.image_view(c.view, c.display_geometry, handles);
    if (ui.check("预览 / Preview", c.preview)) c.display_dirty = true;
    c.worker.controls(ui, c.dirty);
    if (psui::preview_background_control(ui, c.background)) c.display_dirty = true;
    ui.preview_zoom(c.view);
    if (c.source_error) {
        ImGui::TextWrapped("%s", ui.text("贴图图层缺失、重名或无法读取。请选择有效图层。 / The texture layer is missing, ambiguous, or unreadable. Choose another layer.").c_str());
    } else if (c.error) {
        ImGui::TextWrapped("%s", ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.f, ImGui::GetContentRegionAvail().y - footer));
    ImGui::BeginDisabled(c.error != noErr || c.source_error != noErr);
    if (ui.button("ok", "确定 / OK", -1)) ui.close(true);
    ImGui::EndDisabled();
    if (ui.button("cancel", "取消 / Cancel", -1)) ui.close(false);
    if (ui.button("reset", "全部重置 / Reset All", -1)) {
        p = factory;
        reload = true;
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("parameters", {0, 0});
    if (ui.section("位置与尺寸 / Position and Size", true)) {
        changed |= ui.number("中心 X（%） / Center X (%)", p.center_x, -200, 300, 0, 100);
        changed |= ui.number("中心 Y（%） / Center Y (%)", p.center_y, -200, 300, 0, 100);
        changed |= ui.number("半径（%） / Radius (%)", p.radius, .1, 200, 1, 100);
        if constexpr (effect_shape == Shape::cylinder) {
            changed |= ui.number("高度（%） / Height (%)", p.height, .1, 400, 1, 150);
            constexpr const char* sides[] = {"仅正面 / Front Only", "仅背面 / Back Only", "正反面 / Front and Back"};
            changed |= ui.choice("渲染面 / Render Sides", p.render_sides, sides, 3);
        }
    }
    if (ui.section("贴图 / Texture Mapping", true)) {
        reload |= source_control(ui, c);
        if constexpr (effect_shape == Shape::sphere) {
            changed |= ui.check("局部贴花 / Local Decal", p.local_decal);
        }
        const bool using_reference = p.texture_key != 0;
        const bool can_keep_current = using_reference || can_overlay_current_layer(effect_shape, p);
        auto& keep_current = using_reference ? p.keep_current : p.keep_current_for_self_decal;
        if (can_keep_current) {
            changed |= ui.check("保留当前层效果 / Keep Current Layer Effect", keep_current);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
                const auto hint = using_reference
                    ? "将所选贴图叠在当前图层生成的完整曲面上。 / Layer the selected texture over the full surface mapped from the current layer."
                    : "用当前图层生成完整曲面，再叠加同一图层的局部贴花。 / Wrap the full surface with the current layer, then overlay a decal from that layer.";
                ImGui::SetTooltip("%s", ui.text(hint).c_str());
            }
        }
        ui.items["texturewrap.source.state"] = {using_reference ? 1.f : 0.f, c.source_error ? 1.f : 0.f,
                                                can_keep_current && keep_current ? 1.f : 0.f, 0};
        if constexpr (effect_shape == Shape::sphere) {
            if (p.local_decal) {
                changed |= ui.number("贴花经度（度） / Decal Longitude (deg)", p.decal_longitude, -180, 180, -180, 180);
                changed |= ui.number("贴花纬度（度） / Decal Latitude (deg)", p.decal_latitude, -90, 90, -90, 90);
                changed |= ui.number("贴花尺寸（度） / Decal Size (deg)", p.decal_size, 1, 160, 1, 160);
            }
        } else {
            changed |= ui.number("包裹角度（度） / Wrap Angle (deg)", p.wrap_angle, 1, 360, 1, 360);
            changed |= ui.number("周向位置（度） / Decal Angle (deg)", p.cylinder_decal_angle, -180, 180, -180, 180);
            changed |= ui.number("上下位置（%） / Decal Position (%)", p.cylinder_decal_position, 0, 100, 0, 100);
            changed |= ui.number("贴花高度（%） / Decal Height (%)", p.cylinder_decal_height, 1, 100, 1, 100);
        }
    }
    if (ui.section("旋转 / Rotation", true)) {
        changed |= ui.number("X 轴（度） / X (deg)", p.rotation_x, -360, 360, -180, 180);
        changed |= ui.number("Y 轴（度） / Y (deg)", p.rotation_y, -360, 360, -180, 180);
        changed |= ui.number("Z 轴（度） / Z (deg)", p.rotation_z, -360, 360, -180, 180);
    }
    if (ui.section("光照 / Lighting", true)) {
        changed |= ui.check("启用光照 / Enable Lighting", p.lighting);
        ImGui::BeginDisabled(!p.lighting);
        changed |= ui.number("灯光方向（度） / Direction (deg)", p.light_direction, -360, 360, -180, 180);
        changed |= ui.number("灯光仰角（度） / Elevation (deg)", p.light_elevation, -90, 90, -90, 90);
        changed |= ui.number("环境光（%） / Ambient (%)", p.ambient, 0, 100, 0, 100);
        changed |= ui.number("主光强度（%） / Light (%)", p.diffuse, 0, 200, 0, 100);
        changed |= ui.number("高光（%） / Highlight (%)", p.specular, 0, 100, 0, 100);
        ImGui::EndDisabled();
    }
    ImGui::EndChild();
    if (reload) {
        c.worker.stop();
        c.read_source();
    }
    c.dirty |= changed || reload;
}

} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source,
                    const std::vector<psui::HostLayer>& layers, Parameters& p, Inputs& inputs) {
    try {
        Context c{p, host, source, layers, inputs};
        c.read_source();
        constexpr auto glyphs = "球面圆柱贴图 位置与尺寸 中心 半径 高度 包裹角度 周向位置 上下位置 贴花高度 渲染面 仅正面 仅背面 正反面 "
                                "局部贴花 贴花经度 贴花纬度 贴花尺寸 旋转 轴度 当前图层 贴图来源 图层缺失 保留当前层效果 "
                                "将所选贴图叠在当前图层生成的完整曲面上 用当前图层生成完整曲面再叠加同一图层的局部贴花 "
                                "贴图图层缺失重名或无法读取请选择有效图层 "
                                "光照 启用光照 灯光方向 灯光仰角 环境光 主光强度 高光 预览 确定取消全部重置 预览失败请重置或取消";
        const auto answer = psui::run_filter_panel(module, owner, host, title, 900, 690,
            [&](psui::LibraryPanel& ui) { draw(ui, c); }, glyphs, &plugin_info);
        c.worker.stop();
        if (answer != IDOK) return answer;
        if (c.source_error) return c.source_error;
        if (c.error) return c.error;
        p = c.values;
        return answer;
    } catch (...) {
        return memFullErr;
    }
}

} // namespace texturewrap
