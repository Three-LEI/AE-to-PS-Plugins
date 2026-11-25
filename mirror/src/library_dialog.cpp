#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/adaptive_preview.hpp"
#include "psui/generator_controls.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "mirror_host.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace mirror {
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

bool same_rect(VRect a, VRect b) {
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
}

// The mirror can read from the opposite side of the reflection line. When the
// preview is zoomed, reading only the visible crop would clamp those samples to
// the crop edge and smear them. Read the visible region plus its reflection.
VRect reflection_union(const Parameters& p, VRect visible, VRect document) {
    if (visible.right <= visible.left || visible.bottom <= visible.top) return visible;
    const double radians = p.angle * std::numbers::pi / 180.;
    const double nx = std::cos(radians), ny = std::sin(radians);
    const auto reflect = [&](double x, double y) {
        const double distance = (x - p.cx) * nx + (y - p.cy) * ny;
        return std::array<double, 2>{x - 2 * distance * nx, y - 2 * distance * ny};
    };
    const std::array<std::array<double, 2>, 4> corners{{
        reflect(visible.left, visible.top), reflect(visible.right, visible.top),
        reflect(visible.right, visible.bottom), reflect(visible.left, visible.bottom)}};
    double left = visible.left, top = visible.top, right = visible.right, bottom = visible.bottom;
    for (const auto& corner : corners) {
        left = std::min(left, corner[0]);
        top = std::min(top, corner[1]);
        right = std::max(right, corner[0]);
        bottom = std::max(bottom, corner[1]);
    }
    VRect result{int(std::floor(top)), int(std::floor(left)), int(std::ceil(bottom)), int(std::ceil(right))};
    result.left = std::max(result.left, document.left);
    result.top = std::max(result.top, document.top);
    result.right = std::min(result.right, document.right);
    result.bottom = std::min(result.bottom, document.bottom);
    if (result.right <= result.left || result.bottom <= result.top) return visible;
    return result;
}

struct Crop {
    int x = 0, y = 0, width = 0, height = 0;
};

Crop crop_box(const psui::ProxyImage& source, VRect visible) {
    const double step = std::max(1.0, source.step);
    int x0 = int(std::floor((visible.left - source.origin_x) / step));
    int y0 = int(std::floor((visible.top - source.origin_y) / step));
    int x1 = int(std::ceil((visible.right - source.origin_x) / step));
    int y1 = int(std::ceil((visible.bottom - source.origin_y) / step));
    x0 = std::clamp(x0, 0, std::max(0, source.width - 1));
    y0 = std::clamp(y0, 0, std::max(0, source.height - 1));
    x1 = std::clamp(x1, x0 + 1, source.width);
    y1 = std::clamp(y1, y0 + 1, source.height);
    return {x0, y0, x1 - x0, y1 - y0};
}

psui::ProxyImage crop_proxy(const psui::ProxyImage& source, const Crop& box) {
    psui::ProxyImage out;
    out.width = box.width;
    out.height = box.height;
    out.origin_x = source.origin_x + box.x * source.step;
    out.origin_y = source.origin_y + box.y * source.step;
    out.step = source.step;
    out.channels = source.channels;
    out.pixels.resize(std::size_t(box.width) * box.height * source.channels);
    for (int y = 0; y < box.height; ++y)
        std::copy_n(source.pixels.begin() + (std::size_t(box.y + y) * source.width + box.x) * source.channels,
                    std::size_t(box.width) * source.channels,
                    out.pixels.begin() + std::size_t(y) * box.width * source.channels);
    if (source.mask.size() == std::size_t(source.width) * source.height) {
        out.mask.resize(std::size_t(box.width) * box.height);
        for (int y = 0; y < box.height; ++y)
            std::copy_n(source.mask.begin() + std::size_t(box.y + y) * source.width + box.x, box.width,
                        out.mask.begin() + std::size_t(y) * box.width);
    }
    return out;
}

std::vector<std::uint8_t> crop_rgb(const std::vector<std::uint8_t>& source, const psui::ProxyImage& full,
                                   const Crop& box) {
    std::vector<std::uint8_t> out(std::size_t(box.width) * box.height * full.channels);
    for (int y = 0; y < box.height; ++y)
        std::copy_n(source.begin() + (std::size_t(box.y + y) * full.width + box.x) * full.channels,
                    std::size_t(box.width) * full.channels,
                    out.begin() + std::size_t(y) * box.width * full.channels);
    return out;
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
    VRect loaded_region{};
    psui::AdaptivePreview<PreviewFrame> worker;
    Context(const Parameters& p, FilterRecord* h) : values(p), host(h) {}
};

void update(psui::LibraryPanel& ui, Context& c) {
    const auto visible = c.view.region();
    const auto document = psui::filter_document_bounds(c.host);
    const auto needed = reflection_union(c.values, visible, document);
    if (c.view.dirty || c.draft_side != c.worker.draft_side() || !same_rect(c.loaded_region, needed)) {
        c.draft_side = c.worker.draft_side();
        auto full = std::make_shared<psui::ProxyImage>(), small = std::make_shared<psui::ProxyImage>();
        const auto request = psui::document_to_filter(c.host, needed);
        c.error = psui::host_read_proxy_area(c.host, c.view.proxy_side(), request, *full);
        if (!c.error)
            c.error = psui::host_read_proxy_area(c.host, std::min(c.draft_side, c.view.proxy_side()),
                                                request, *small);
        if (c.error) { ui.close(false); return; }
        const auto origin = psui::filter_origin(c.host);
        full->origin_x += origin.h;
        full->origin_y += origin.v;
        small->origin_x += origin.h;
        small->origin_y += origin.v;
        c.full_input = std::move(full);
        c.draft_input = std::move(small);
        c.loaded_region = needed;
        c.view.dirty = false;
        c.dirty = true;
    }
    const auto p = c.values;
    const auto full = c.full_input, small = c.draft_input;
    try {
        PreviewFrame frame;
        if (c.worker.update(c.dirty, frame, [p, full, small, visible](bool draft, const Cancel& cancel) {
                PreviewFrame next;
                next.input = draft ? small : full;
                const auto input = next.input;
                if (!input || cancel()) return next;
                auto src = to_image(*input);
                const auto q = proxy_parameters(p, input->origin_x, input->origin_y, input->step);
                Image out;
                if (render(src, q, out, cancel)) {
                    std::vector<std::uint8_t> full_output;
                    to_rgb(out, *input, full_output);
                    const auto box = crop_box(*input, visible);
                    next.output = crop_rgb(full_output, *input, box);
                    next.input = std::make_shared<psui::ProxyImage>(crop_proxy(*input, box));
                }
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

void draw(psui::LibraryPanel& ui, Context& c) {
    const auto bounds = psui::filter_document_bounds(c.host);
    auto factory = defaults(bounds.right - bounds.left, bounds.bottom - bounds.top, bounds.left, bounds.top);
    ui.parameter_defaults(c.values, factory);

    bool changed = false;
    ui.begin_filter_preview(c.view, bounds);
    update(ui, c);
    psui::PreviewAdjustments handles;
    const auto& p = c.values;
    const double radians = p.angle * std::numbers::pi / 180.;
    const double extent = std::hypot(double(bounds.right) - bounds.left, double(bounds.bottom) - bounds.top);
    const double dx = -std::sin(radians) * extent, dy = std::cos(radians) * extent;
    handles.point("反射中心 / Reflection Center", c.values.cx, c.values.cy)
           .angle("反射角度 / Reflection Angle", c.values.angle, p.cx, p.cy, -180, 180, 90)
           .line(p.cx - dx, p.cy - dy, p.cx + dx, p.cy + dy);
    changed |= ui.image_view(c.view, c.source, handles);
    changed |= ui.check("预览 / Preview", c.preview);
    c.worker.controls(ui, c.dirty);
    if (psui::preview_background_control(ui, c.background)) c.dirty = true;
    ui.preview_zoom(c.view);
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("parameters", {0, 0}, 0, ImGuiWindowFlags_NoScrollbar);
    const float footer = ui.filter_footer_height();
    ImGui::BeginChild("mirror-controls", {0, -footer}, 0, ImGuiWindowFlags_NoScrollWithMouse);
    changed |= psui::point_control(ui, "反射中心 / Reflection Center", c.values.cx, c.values.cy);
    changed |= ui.number("反射角度（度） / Reflection Angle (deg)", c.values.angle, -180, 180, -180, 180);
    ImGui::TextWrapped("%s", ui.text("0° 把左侧镜像到右侧；90° 把上侧镜像到下侧。 / 0° mirrors the left side to the right; 90° mirrors the top to the bottom.").c_str());
    ImGui::EndChild();
    ImGui::BeginDisabled(c.error != noErr);
    if (ui.button("ok", "确定 / OK", -1)) ui.close(true);
    ImGui::EndDisabled();
    if (ui.button("cancel", "取消 / Cancel", -1)) ui.close(false);
    if (ui.button("reset", "全部重置 / Reset All", -1)) {
        c.values = factory;
        c.dirty = true;
    }
    ImGui::EndChild();
    if (changed) c.dirty = true;
}
} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image&, Parameters& p) {
    try {
        Context c{p, host};
        const auto glyphs = "镜像 反射中心 反射角度 预览 显示背景 适合 确定 取消 全部重置 把左侧镜像到右侧 把上侧镜像到下侧";
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
} // namespace mirror
