#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/adaptive_preview.hpp"
#include "psui/generator_controls.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "toner_host.hpp"
#include <algorithm>
#include <memory>
#include <vector>

namespace toner {
namespace {
struct PreviewFrame {
    std::shared_ptr<psui::ProxyImage> input;
    std::vector<std::uint8_t> output;
};

Image to_image(const psui::ProxyImage& proxy) {
    Image image;
    image.width = proxy.width;
    image.height = proxy.height;
    image.rgba.resize(std::size_t(proxy.width) * proxy.height * 4);
    for (int y = 0; y < proxy.height; ++y)
        for (int x = 0; x < proxy.width; ++x) {
            const auto i = std::size_t(y) * proxy.width + x;
            const auto p = i * proxy.channels;
            for (int k = 0; k < 3; ++k) image.rgba[i * 4 + k] = proxy.pixels[p + k] / 255.f;
            image.rgba[i * 4 + 3] = proxy.channels == 4 ? proxy.pixels[p + 3] / 255.f : 1.f;
        }
    return image;
}

void to_rgb(const Image& image, const psui::ProxyImage& source, std::vector<std::uint8_t>& output) {
    output.resize(source.pixels.size());
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x) {
            const auto i = std::size_t(y) * image.width + x;
            const auto p = i * source.channels;
            for (int k = 0; k < 3; ++k)
                output[p + k] = std::uint8_t(std::lround(std::clamp(image.rgba[i * 4 + k], 0.f, 1.f) * 255));
            if (source.channels == 4) output[p + 3] = source.pixels[p + 3];
        }
}

struct Context {
    Parameters values;
    FilterRecord* host;
    psui::ProxyImage source;
    psui::PreviewView view;
    psui::PreviewBackground background;
    std::vector<std::uint8_t> processed, display;
    bool dirty = true, preview = true, display_dirty = true;
    OSErr error = noErr;
    std::shared_ptr<psui::ProxyImage> full_input, draft_input;
    int draft_side = 0;
    psui::AdaptivePreview<PreviewFrame> worker;
    Context(const Parameters& p, FilterRecord* h) : values(p), host(h) {}
};

void update(psui::LibraryPanel& ui, Context& c) {

    if (c.view.dirty || c.draft_side != c.worker.draft_side()) {
        c.draft_side = c.worker.draft_side();
        auto full = std::make_shared<psui::ProxyImage>(), small = std::make_shared<psui::ProxyImage>();
        c.error = psui::host_read_proxy_area(c.host, c.view.proxy_side(), c.view.region(), *full);
        if (!c.error)
            c.error = psui::host_read_proxy_area(c.host, std::min(c.draft_side, c.view.proxy_side()),
                                                c.view.region(), *small);

        if (c.error) { ui.close(false); return; }
        c.full_input = std::move(full);
        c.draft_input = std::move(small);
        c.view.dirty = false;
        c.dirty = true;
    }
    const auto p = c.values;
    const auto full = c.full_input, small = c.draft_input;
    try {
        PreviewFrame frame;
        if (c.worker.update(c.dirty, frame, [p, full, small](bool draft, const Cancel& cancel) {
                PreviewFrame next;
                next.input = draft ? small : full;
                const auto input = next.input;
                if (!input || cancel()) return next;
                auto src = to_image(*input);
                Image out;
                if (render(src, p, out, cancel)) to_rgb(out, *input, next.output);
                return next;
            })) {

            if (frame.input) {
                c.source = *frame.input;
                c.processed = std::move(frame.output);
                c.display.resize(std::size_t(c.source.width) * c.source.height * 3);
                c.display_dirty = true;
            }
        }
    } catch (...) {
        c.error = memFullErr;
        ui.close(false);
        return;
    }
    if (c.source.pixels.empty() || !c.display_dirty) return;
    const auto& src = c.source;
    c.background.prepare(c.host, src);
    for (int y = 0; y < src.height; ++y)
        for (int x = 0; x < src.width; ++x) {
            const auto i = std::size_t(y) * src.width + x, p = i * src.channels;
            const int alpha = src.channels == 4 ? src.pixels[p + 3] : 255;
            const int mask = src.mask.empty() ? 255 : src.mask[i];
            const int bg = ((x / 8 + y / 8) & 1) ? 156 : 200;
            if (c.background.enabled) {
                const std::array<float, 4> original{src.pixels[p] / 255.f, src.pixels[p + 1] / 255.f,
                                                    src.pixels[p + 2] / 255.f, alpha / 255.f};
                auto processed = original;
                if (c.preview)
                    for (int k = 0; k < 3; ++k) processed[k] = c.processed[p + k] / 255.f;
                const auto pixel = c.background.pixel(x, y, original, processed, mask / 255.f);
                for (int k = 0; k < 3; ++k) c.display[i * 3 + k] = pixel[k];
                continue;
            }
            for (int k = 0; k < 3; ++k) {
                const int original = src.pixels[p + k];
                const int v = c.preview ? (original * (255 - mask) + c.processed[p + k] * mask + 127) / 255 : original;
                c.display[i * 3 + k] = static_cast<std::uint8_t>((v * alpha + bg * (255 - alpha) + 127) / 255);
            }
        }
    if (!ui.preview(c.host, c.display.data(), src.width, src.height)) {
        c.error = memFullErr;
        ui.close(false);
    }
    c.display_dirty = false;
}

bool color_enabled(int mode, int row) {
    switch (mode) {
        case duotone: return row == 0 || row == 4;
        case tritone: return row == 0 || row == 2 || row == 4;
        case pentone: return true;
        default: return row == 0;
    }
}

void draw(psui::LibraryPanel& ui, Context& c) {
    ui.parameter_defaults(c.values, Parameters{});

    bool changed = false;
    ui.begin_filter_preview(c.view, psui::filter_bounds(c.host));
    update(ui, c);
    ui.image_view(c.view, c.source);
    changed |= ui.check("预览 / Preview", c.preview);
    c.worker.controls(ui, c.dirty);
    if (psui::preview_background_control(ui, c.background)) c.dirty = true;
    ui.preview_zoom(c.view);
    ImGui::EndChild();
    ImGui::SameLine();
    ui.begin_filter_parameters(ImGuiWindowFlags_NoScrollbar);
    const float footer = ui.filter_footer_height();
    ImGui::BeginChild("toner-controls", {0, -footer}, 0, ImGuiWindowFlags_NoScrollWithMouse);
    const char* modes[] = {"双色调 / Duotone", "三色调 / Tritone", "五色调 / Pentone", "纯色 / Solid"};
    changed |= ui.choice("色调 / Tones", c.values.mode, modes, 4);
    const bool solid = c.values.mode == toner::solid;
    const char* labels[] = {"高光 / Highlights", "亮部 / Brights", "中间调 / Midtones",
                            "暗部 / Darktones", "阴影 / Shadows"};
    double* colors[] = {c.values.highlights.data(), c.values.brights.data(), c.values.midtones.data(),
                        c.values.darktones.data(), c.values.shadows.data()};
    for (int row = 0; row < 5; ++row) {
        ImGui::BeginDisabled(!color_enabled(c.values.mode, row));
        changed |= psui::color_control(ui, c.host, labels[row], colors[row]);
        ImGui::EndDisabled();
    }
    double blend = c.values.blend * 100.;
    if (ui.number("混合原图 / Blend w. Original (%)", blend, 0, 100, 0, 100)) {
        c.values.blend = blend / 100.;
        changed = true;
    }
    if (solid)
        ImGui::TextWrapped("%s", ui.text("纯色模式使用“高光”颜色覆盖整个图层；混合原图可保留原图细节。 / Solid uses the Highlights color across the layer; Blend w. Original keeps the source detail.").c_str());
    else
        ImGui::TextWrapped("%s", ui.text("按亮度把阴影、中间调和高光映射到所选颜色。 / Maps shadows, midtones and highlights to the selected colors by luminance.").c_str());
    ImGui::EndChild();
    ImGui::BeginDisabled(c.error != noErr);
    if (ui.button("ok", "确定 / OK", -1)) ui.close(true);
    ImGui::EndDisabled();
    if (ui.button("cancel", "取消 / Cancel", -1)) ui.close(false);
    if (ui.button("reset", "全部重置 / Reset All", -1)) {
        c.values = Parameters{};
        changed = true;
    }
    ui.end_filter_parameters();
    if (changed) c.dirty = true;
}
} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image&, Parameters& p) {
    try {
        Context c{p, host};
        const auto glyphs = "多色调 双 三 五 纯 高光 亮部 中间调 暗调 阴影 混合原图 预览 显示背景 适合 确定 取消 全部重置 "
                            "按亮度把 和 映射到所选颜色 模式使用 颜色覆盖整个图层 可保留原图细节";
        const auto answer = psui::run_filter_panel(module, owner, host, title, 780, 620,
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
} // namespace toner
