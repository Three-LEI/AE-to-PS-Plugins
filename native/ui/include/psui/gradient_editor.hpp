#pragma once

#include "library_panel.hpp"
#include "../../../common/gradient.hpp"
#include <cstdio>

namespace psui {

enum class GradientColors { RGBA, Grayscale };

struct GradientEditorState {
    int selected = 0;
    bool dragging = false;
    float drag_offset = 0;
};

inline constexpr const char* gradient_editor_glyphs =
    "渐变 插值 线性 平滑 阶梯 选择预设 黑到白 白到黑 黑白黑 白黑白 添加 删除 复制 反转 色标 "
    "位置 不透明度 灰度 颜色 区段中点 双击色带添加拖动色标删除选中项 类型 径向 对称 角度 菱形 跨度 中心";

inline bool gradient_type(LibraryPanel& ui, pscpu::gradient::Placement& p) {
    constexpr const char* types[] = {"线性 / Linear", "径向 / Radial", "对称 / Reflected",
        "角度 / Angular", "菱形 / Diamond"};
    return ui.choice("渐变类型 / Gradient Type", p.shape, types, 5);
}

inline bool gradient_placement(LibraryPanel& ui, pscpu::gradient::Placement& p) {
    using pscpu::gradient::Shape;
    bool changed = ui.number("角度（度） / Angle (deg)", p.angle, -180, 180, -180, 180, p.shape != int(Shape::Radial));
    changed |= ui.number("跨度（%） / Span (%)", p.span, 1, 400, 1, 200, p.shape != int(Shape::Angular));
    changed |= ui.number("中心 X（%） / Center X (%)", p.center_x, 0, 100, 0, 100);
    changed |= ui.number("中心 Y（%） / Center Y (%)", p.center_y, 0, 100, 0, 100);
    return changed;
}

inline bool gradient_editor(LibraryPanel& ui, FilterRecord* host, const char* key,
                            pscpu::gradient::Ramp& ramp, GradientEditorState& state,
                            GradientColors colors = GradientColors::RGBA) {
    using namespace pscpu::gradient;
    ImGui::PushID(key);
    bool changed = ui.parameter_label("渐变 / Gradient", ramp);
    state.selected = std::clamp(state.selected, 0, ramp.count - 1);
    constexpr const char* presets[] = {"选择预设 / Load Preset", "黑到白 / Black to White",
        "白到黑 / White to Black", "黑白黑 / Black White Black", "白黑白 / White Black White"};
    int preset = 0;
    if (ui.choice("渐变预设 / Gradient Preset", preset, presets, 5) && preset) {
        ramp = Ramp{};
        if (preset >= 3) {
            ramp.stops[1].position = .5;
            insert(ramp, Stop{1, ramp.stops[0].color});
        }
        if (preset == 2 || preset == 4) {
            for (int i = 0; i < ramp.count; ++i)
                for (int c = 0; c < 3; ++c) ramp.stops[i].color[c] = 1 - ramp.stops[i].color[c];
        }
        state.selected = 0;
        changed = true;
    }

    const float scale = ui.scale(), width = std::max(24.f, ImGui::GetContentRegionAvail().x);
    const float handle = 7 * scale, bar_height = 23 * scale;
    ImGui::InvisibleButton("##strip", {width, bar_height + 2 * handle + 4 * scale});
    const auto origin = ImGui::GetItemRectMin();
    const ImVec2 bar_min{origin.x + handle, origin.y}, bar_max{origin.x + width - handle, origin.y + bar_height};
    const float span = std::max(1.f, bar_max.x - bar_min.x);
    const auto marker_x = [&](int index) { return bar_min.x + span * float(ramp.stops[index].position); };
    const auto mouse = ImGui::GetIO().MousePos;
    const auto position_at = [&](float x) { return std::clamp(double((x - bar_min.x) / span), 0., 1.); };
    const auto add = [&](Stop stop) {
        const int inserted = insert(ramp, stop);
        if (inserted >= 0) {
            state.selected = inserted;
            changed = true;
        }
    };
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (mouse.y >= bar_max.y) {
            int nearest = -1;
            float distance = handle + 1;
            for (int i = 0; i < ramp.count; ++i) {
                const float delta = std::abs(mouse.x - marker_x(i));
                if (delta < distance || (delta == distance && i == state.selected)) {
                    nearest = i;
                    distance = delta;
                }
            }
            if (nearest >= 0) {
                state.selected = nearest;
                state.dragging = true;
                state.drag_offset = mouse.x - marker_x(nearest);
            }
        } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            const double position = position_at(mouse.x);
            add(Stop{position, sample(ramp, position)});
        }
    }
    if (state.dragging && ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        const double position = position_at(mouse.x - state.drag_offset);
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0) && position != ramp.stops[state.selected].position) {
            state.selected = move(ramp, state.selected, position);
            changed = true;
        }
    } else {
        state.dragging = false;
    }
    ui.mark((std::string(key) + ".strip").c_str());
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
        ImGui::SetTooltip("%s", ui.text("双击色带添加，拖动色标，删除选中项。 / Double-click to add, drag stops, or delete the selected stop.").c_str());
    }
    auto* draw = ImGui::GetWindowDrawList();
    const auto rgba = [](const Color& color) {
        return ImGui::ColorConvertFloat4ToU32({float(color[0]), float(color[1]), float(color[2]), float(color[3])});
    };
    const float tile = 6 * scale;
    for (int row = 0; row * tile < bar_height; ++row) {
        for (int column = 0; column * tile < span; ++column) {
            const ImVec2 first{bar_min.x + column * tile, bar_min.y + row * tile};
            const ImVec2 last{std::min(bar_max.x, first.x + tile), std::min(bar_max.y, first.y + tile)};
            draw->AddRectFilled(first, last, (row + column) % 2 ? IM_COL32(150, 150, 150, 255) : IM_COL32(205, 205, 205, 255));
        }
    }
    constexpr int slices = 192;
    auto left = rgba(sample(ramp, 0));
    for (int i = 1; i <= slices; ++i) {
        const auto right = rgba(sample(ramp, double(i) / slices));
        draw->AddRectFilledMultiColor({bar_min.x + span * (i - 1) / slices, bar_min.y},
            {bar_min.x + span * i / slices, bar_max.y}, left, right, right, left);
        left = right;
    }
    draw->AddRect(bar_min, bar_max, ImGui::GetColorU32(ImGuiCol_Border));
    const auto draw_stop = [&](int i, bool selected) {
        const float x = marker_x(i), y = bar_max.y + 3 * scale;
        const ImU32 outline = ImGui::GetColorU32(selected ? ImGuiCol_SliderGrabActive : ImGuiCol_TextDisabled);
        draw->AddTriangleFilled({x, bar_max.y}, {x - handle, y + 3 * scale}, {x + handle, y + 3 * scale}, outline);
        const ImVec2 first{x - handle, y + 3 * scale}, last{x + handle, y + 13 * scale};
        auto color = ramp.stops[i].color;
        color[3] = 1;
        draw->AddRectFilled(first, last, rgba(color), 2 * scale);
        draw->AddRect(first, last, outline, 2 * scale, 0, selected ? 2 * scale : scale);
    };
    for (int i = 0; i < ramp.count; ++i) if (i != state.selected) draw_stop(i, false);
    draw_stop(state.selected, true);

    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float button_width = std::max(1.f, (width - 3 * spacing) / 4);
    const auto insertion_position = [&] {
        const int i = state.selected;
        const double here = ramp.stops[i].position;
        const double neighbor = i + 1 < ramp.count ? ramp.stops[i + 1].position
                              : here < 1 ? 1 : ramp.stops[i - 1].position;
        return (here + neighbor) * .5;
    };
    ImGui::BeginDisabled(ramp.count >= max_stops);
    if (ui.button("gradient.add", "添加 / Add", button_width)) {
        const double position = insertion_position();
        add(Stop{position, sample(ramp, position)});
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(ramp.count <= 2);
    if (ui.button("gradient.delete", "删除 / Delete", button_width) && remove(ramp, state.selected)) {
        state.selected = std::min(state.selected, ramp.count - 1);
        changed = true;
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(ramp.count >= max_stops);
    if (ui.button("gradient.duplicate", "复制 / Duplicate", button_width)) {
        auto copy = ramp.stops[state.selected];
        copy.position = insertion_position();
        add(copy);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ui.button("gradient.reverse", "反转 / Reverse", button_width)) {
        reverse(ramp);
        state.selected = ramp.count - 1 - state.selected;
        changed = true;
    }

    constexpr const char* modes[] = {"线性 / Linear", "平滑 / Smooth", "阶梯 / Step"};
    changed |= ui.choice("渐变插值 / Interpolation", ramp.interpolation, modes, 3);
    char stop_names[max_stops][32];
    const char* stop_items[max_stops];
    for (int i = 0; i < ramp.count; ++i) {
        std::snprintf(stop_names[i], sizeof(stop_names[i]), "%d. %.1f%%", i + 1, 100 * ramp.stops[i].position);
        stop_items[i] = stop_names[i];
    }
    ui.choice("选中色标 / Selected Stop", state.selected, stop_items, ramp.count);
    double position = 100 * ramp.stops[state.selected].position;
    if (ui.number("色标位置（%） / Stop Position (%)", position, 0, 100, 0, 100)) {
        state.selected = move(ramp, state.selected, .01 * position);
        changed = true;
    }
    auto& stop = ramp.stops[state.selected];
    ImGui::TextUnformatted(ui.text("色标颜色 / Stop Color").c_str());
    ImGui::SameLine();
    double rgb[] = {stop.color[0], stop.color[1], stop.color[2]};
    const auto gray = [](const double* color) { return .2126 * color[0] + .7152 * color[1] + .0722 * color[2]; };
    if (colors == GradientColors::Grayscale) std::fill_n(rgb, 3, gray(rgb));
    if (ui.color_swatch("gradient.stop-color", host, rgb)) {
        if (colors == GradientColors::Grayscale) std::fill_n(rgb, 3, gray(rgb));
        for (int c = 0; c < 3; ++c) stop.color[c] = std::clamp(rgb[c], 0., 1.);
        changed = true;
    }
    if (colors == GradientColors::Grayscale) {
        double tone = 100 * gray(stop.color.data());
        if (ui.number("灰度（%） / Gray (%)", tone, 0, 100, 0, 100)) {
            stop.color[0] = stop.color[1] = stop.color[2] = .01 * tone;
            changed = true;
        }
    }
    double opacity = 100 * stop.color[3];
    if (ui.number("色标不透明度（%） / Stop Opacity (%)", opacity, 0, 100, 0, 100)) {
        stop.color[3] = .01 * opacity;
        changed = true;
    }
    if (state.selected + 1 < ramp.count) {
        double midpoint = 100 * stop.midpoint;
        if (ui.number("区段中点（%） / Segment Midpoint (%)", midpoint, 1, 99, 1, 99)) {
            stop.midpoint = .01 * midpoint;
            changed = true;
        }
    }
    ImGui::PopID();
    return changed;
}

} // namespace psui
