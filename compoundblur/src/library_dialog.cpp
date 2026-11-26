#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "compoundblur_host.hpp"
#include "../../native/common/cached_filter_runtime.hpp"
#include "psui/gradient_editor.hpp"
#include "psui/raster_filter_dialog.hpp"

namespace compoundblur {

OSErr load_reference(FilterRecord* host, const std::vector<psui::HostLayer>& layers, std::uint64_t key,
                     std::shared_ptr<const Image>& reference) {
    reference.reset();
    if (!key) return noErr;
    psui::LayerPixels pixels;
    const auto error = psfilter::read_named_layer(host, layers, key, pixels);
    if (!error) reference = std::make_shared<Image>(from_host(std::move(pixels)));
    return error;
}

namespace {

struct PreviewRenderer {
    struct Cache { BlurPyramid full, draft; };
    Inputs inputs;
    std::shared_ptr<Cache> cache = std::make_shared<Cache>();
    bool show_map = false;

    bool operator()(const Image& source, const Parameters& p, Rect area, SampleGrid grid,
                    Image& output, const Cancel& cancel) const {
        auto& levels = grid.step == 1 ? cache->full : cache->draft;
        return render_region(source, p, inputs.reference.get(), inputs.domain, area, grid, output, cancel, &levels, show_map);
    }
};

bool layer_control(psui::LibraryPanel& ui, const std::vector<psui::HostLayer>& layers, std::uint64_t& key) {
    bool changed = false;
    if (ui.begin_parameter_row("blur-reference")) {
        ImGui::TableNextColumn();
        changed |= ui.parameter_label("参考图层 / Reference Layer", key);
        ImGui::TableNextColumn();
        const auto current = ui.text("当前图层 / Current Layer");
        auto selected = key ? ui.text("图层缺失 / Missing Layer") : current;
        for (const auto& layer : layers) if (key == layer.name_key) selected = layer.name;
        ImGui::SetNextItemWidth(-1);
        const bool open = ImGui::BeginCombo("##layer", selected.c_str());
        ui.mark("compoundblur.reference");
        if (open) {
            if (ImGui::Selectable(current.c_str(), !key)) {
                key = 0;
                changed = true;
            }
            ui.mark("compoundblur.reference/current");
            for (std::size_t i = 0; i < layers.size(); ++i) {
                const auto& layer = layers[i];
                if (layer.target) continue;
                ImGui::PushID(int(i));
                ImGui::BeginDisabled(!unique_layer(layers, layer.name_key));
                if (ImGui::Selectable((layer.name + "###reference").c_str(), key == layer.name_key)) {
                    key = layer.name_key;
                    changed = true;
                }
                ui.mark(("compoundblur.reference/layer/" + std::to_string(i)).c_str());
                ImGui::EndDisabled();
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::EndTable();
    }
    return changed;
}

} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source,
                    const std::vector<psui::HostLayer>& layers, Parameters& p, Inputs& in) {
    PreviewRenderer renderer;
    renderer.inputs.domain = document_bounds(host);
    if (renderer.inputs.domain.empty()) renderer.inputs.domain = source.bounds();
    psui::GradientEditorState editor;
    std::uint64_t loaded_key = 0;
    OSErr map_error = noErr;
    const auto load = [&](std::uint64_t key) {
        loaded_key = key;
        map_error = load_reference(host, layers, key, renderer.inputs.reference);
    };
    if (p.source == Reference) load(p.layer_key);
    const auto controls = [&](psui::LibraryPanel& ui, Parameters& values) {
        constexpr const char* sources[] = {"内置渐变 / Built-in Gradient", "参考图层 / Reference Layer"};
        constexpr const char* channels[] = {"亮度 / Luminance", "透明度 / Alpha"};
        bool changed = ui.choice("模糊来源 / Blur Source", values.source, sources, 2);
        if (values.source == Reference) {
            const bool selected = layer_control(ui, layers, values.layer_key);
            if (selected || loaded_key != values.layer_key) {
                load(values.layer_key);
                changed = true;
            }
            changed |= ui.choice("参考通道 / Reference Channel", values.channel, channels, 2);
            if (map_error)
                ImGui::TextWrapped("%s", ui.text("无法读取参考图层，请重新选择。 / Choose a readable reference layer.").c_str());
        } else changed |= psui::gradient_type(ui, values.placement);
        changed |= ui.number("最大半径（像素） / Maximum Radius (px)", values.radius, 0, 256, 0, 64);
        changed |= ui.number("效果混合（%） / Effect Mix (%)", values.mix, 0, 100, 0, 100);
        changed |= ui.check("反相 / Invert Map", values.invert);
        changed |= ui.check("查看模糊图 / Show Blur Map", renderer.show_map);
        if (values.source == Gradient) {
            if (ui.section("渐变编辑器 / Gradient Editor", true))
                changed |= psui::gradient_editor(ui, host, "compound-gradient", values.gradient, editor, psui::GradientColors::Grayscale);
            if (ui.section("渐变位置 / Gradient Placement")) changed |= psui::gradient_placement(ui, values.placement);
        }
        editor.selected = std::clamp(editor.selected, 0, values.gradient.count - 1);
        ui.items["compoundblur.state"] = {float(values.source), float(values.gradient.count),
                                           float(values.gradient.stops[editor.selected].position), float(map_error)};
        return changed;
    };
    const auto glyphs = std::string("复合模糊 内置渐变 参考图层 当前图层 图层缺失 模糊来源 参考通道 亮度透明度 无法读取参考图层请重新选择 "
        "最大半径像素 效果混合 反相 查看模糊图 渐变编辑器 渐变位置 渐变形状 线性径向对称 角度 跨度 中心 预览失败请重置或取消 确定全部重置 ") +
        psui::gradient_editor_glyphs;
    const auto make_adjustments = [&](Parameters& values, const psui::PreviewView&) {
        using pscpu::gradient::Shape;
        psui::PreviewAdjustments handles;
        if (values.source == Gradient) {
            const auto& domain = renderer.inputs.domain;
            auto& place = values.placement;
            const auto space = psui::PreviewPointSpace::percent({domain.top, domain.left, domain.bottom, domain.right});
            const double cx = space.x(place.center_x), cy = space.y(place.center_y);
            handles.point("渐变中心 / Gradient Center", place.center_x, place.center_y, space);
            if (place.shape != int(Shape::Radial))
                handles.angle("渐变角度 / Gradient Angle", place.angle, cx, cy, -180, 180);
        }
        return handles;
    };
    const auto result = psui::run_raster_filter_dialog(module, owner, host, title, plugin_info, glyphs.c_str(),
                                                      source, p, controls, renderer, make_adjustments);
    if (result != IDOK) return result;
    if (p.source == Reference && map_error) return map_error;
    if (p.source == Gradient && !unique_layer(layers, p.layer_key)) p.layer_key = 0;
    in = std::move(renderer.inputs);
    return IDOK;
}

} // namespace compoundblur
