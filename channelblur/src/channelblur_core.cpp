#include "channelblur_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <array>
#include <cmath>

namespace channelblur {
namespace {

constexpr double red_weight = .2126, green_weight = .7152, blue_weight = .0722;

struct Sample {
    float value, coverage;
};

// Three box passes approximate a Gaussian. Fractional endpoint weights keep
// the variance continuous when dragging through whole and subpixel radii.
struct Box {
    int radius;
    double fraction, scale;

    explicit Box(double sigma) {
        radius = int((std::sqrt(1 + 4 * sigma * sigma) - 1) * .5);
        const double size = 2 * radius + 1;
        const double variance = sigma * sigma / 3;
        const double edge = radius + 1.;
        fraction = std::clamp(size * (variance - radius * edge / 3) / (2 * (edge * edge - variance)), 0., 1.);
        scale = 1 / (size + 2 * fraction);
    }
    int reach() const { return radius + int(fraction > 0); }
};

bool pass(const std::vector<Sample>& source, std::vector<Sample>& output,
          int width, int height, Box box, bool horizontal, const Cancel& cancel) {
    const int lines = horizontal ? height : width, length = horizontal ? width : height;
    const std::size_t line_stride = horizontal ? width : 1, pixel_stride = horizontal ? 1 : width;
    const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
        for (auto line = first; line < end; ++line) {
            if (stopped()) return false;
            const auto* begin = source.data() + line * line_stride;
            auto* dest = output.data() + line * line_stride;
            const auto at = [&](int index) -> const Sample& {
                return begin[std::size_t(std::clamp(index, 0, length - 1)) * pixel_stride];
            };
            double value = 0, weight = 0;
            for (int k = -box.radius; k <= box.radius; ++k) {
                value += at(k).value;
                weight += at(k).coverage;
            }
            for (int k = 0; k < length; ++k) {
                const auto& left = at(k - box.radius - 1);
                const auto& right = at(k + box.radius + 1);
                dest[std::size_t(k) * pixel_stride] = {
                    float((value + box.fraction * (double(left.value) + right.value)) * box.scale),
                    float((weight + box.fraction * (double(left.coverage) + right.coverage)) * box.scale)
                };
                const auto& leaving = at(k - box.radius);
                value += double(right.value) - leaving.value;
                weight += double(right.coverage) - leaving.coverage;
            }
        }
        return true;
    };
    return pscpu::parallel_ranges(lines, source.size(), cancel, rows);
}

float component(const float* rgba, int channel, bool luma_chroma) {
    if (!luma_chroma) return rgba[channel];
    const double y = red_weight * rgba[0] + green_weight * rgba[1] + blue_weight * rgba[2];
    if (channel == 0) return float(y);
    return float(rgba[channel == 1 ? 0 : 2] - y);
}

struct BlurPlanes {
    std::vector<Sample> values, scratch;

    bool filter(const Image& source, Rect area, int channel, bool luma_chroma, int axis, Box box,
                const Cancel& cancel) {
        const int width = area.width(), height = area.height();
        const auto pixels = std::size_t(width) * height;
        values.resize(pixels);
        scratch.resize(pixels);
        const auto initialize = [&](std::size_t first, std::size_t end, const auto& stopped) {
            for (auto row = first; row < end; ++row) {
                if (stopped()) return false;
                const auto* src = source.at(area.left, area.top + int(row));
                auto* dest = values.data() + row * width;
                for (int x = 0; x < width; ++x, src += 4) {
                    const float alpha = std::clamp(src[3], 0.f, 1.f);
                    dest[x] = channel == 3 ? Sample{alpha, float(alpha > 0)}
                        : Sample{alpha > 0 ? component(src, channel, luma_chroma) * alpha : 0.f, alpha};
                }
            }
            return true;
        };
        if (!pscpu::parallel_ranges(height, pixels, cancel, initialize)) return false;
        for (int i = 0; i < 3; ++i) {
            if (axis != Vertical) {
                if (!pass(values, scratch, width, height, box, true, cancel)) return false;
                values.swap(scratch);
            }
            if (axis != Horizontal) {
                if (!pass(values, scratch, width, height, box, false, cancel)) return false;
                values.swap(scratch);
            }
        }
        return true;
    }
};

} // namespace

bool valid(const Parameters& p) {
    const auto radius = [](double r) { return r >= 0 && r <= 256; };
    return p.mode >= Channels && p.mode <= LuminanceChroma && p.axis >= Both && p.axis <= Vertical &&
           radius(p.red) && radius(p.green) && radius(p.blue) && radius(p.alpha) &&
           radius(p.luminance) && radius(p.chroma) && p.mix >= 0 && p.mix <= 100;
}

bool render(const Image& source, const Parameters& p, Image& output, const Cancel& cancel) {
    return source.valid() && render_region(source, p, source.bounds(),
        {double(source.left), double(source.top), 1}, output, cancel);
}

bool render_region(const Image& source, const Parameters& p, Rect region, SampleGrid grid,
                   Image& output, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || !(grid.step >= 1 && grid.step <= 2000000) ||
        (cancel && cancel())) return false;
    const auto area = pscpu::intersect(region, source.bounds());
    if (area.empty()) return false;
    const bool luma_chroma = p.mode == LuminanceChroma;
    const std::array<double, 4> radii = luma_chroma ? std::array{p.luminance, p.chroma, p.chroma, p.alpha}
                                                  : std::array{p.red, p.green, p.blue, p.alpha};
    const std::array boxes{Box(radii[0] / grid.step), Box(radii[1] / grid.step),
                           Box(radii[2] / grid.step), Box(radii[3] / grid.step)};
    const int halo = 3 * Box(*std::max_element(radii.begin(), radii.end()) / grid.step).reach();
    Image next{area.width(), area.height(), area.left, area.top,
               std::vector<float>(std::size_t(area.width()) * area.height() * 4)};
    pscpu::ParallelScope parallel;
    const auto for_pixels = [&](const auto& visit) {
        const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
            for (auto row = first; row < end; ++row) {
                if (stopped()) return false;
                const int y = area.top + int(row);
                const auto* src = source.at(area.left, y);
                auto* dest = next.at(area.left, y);
                for (int x = area.left; x < area.right; ++x, src += 4, dest += 4) visit(src, dest, x, y);
            }
            return true;
        };
        return pscpu::parallel_ranges(next.height, next.rgba.size() / 4, cancel, rows);
    };
    if (p.mix == 0 || halo == 0) {
        if (!for_pixels([](const float* src, float* dest, int, int) { std::copy_n(src, 4, dest); })) return false;
        output = std::move(next);
        return true;
    }

    // Three passes need the same neighbourhood in a native viewport as in
    // the full document. Only the requested part is returned to the caller.
    const auto bounds = source.bounds();
    const auto inset = [halo](int high, int low) {
        return int(std::min<std::int64_t>(halo, std::int64_t(high) - low));
    };
    Rect input = area;
    if (p.axis != Vertical) {
        input.left -= inset(area.left, bounds.left);
        input.right += inset(bounds.right, area.right);
    }
    if (p.axis != Horizontal) {
        input.top -= inset(area.top, bounds.top);
        input.bottom += inset(bounds.bottom, area.bottom);
    }
    BlurPlanes planes;
    const auto filter = [&](int channel, Box box) {
        return !box.reach() || planes.filter(source, input, channel, luma_chroma, p.axis, box, cancel);
    };
    const auto sample = [&](int x, int y) -> const Sample& {
        return planes.values[std::size_t(y - input.top) * input.width() + (x - input.left)];
    };
    const bool blur_alpha = boxes[3].reach() != 0;
    const bool blend_edge = blur_alpha && *std::min_element(radii.begin(), radii.begin() + 3) < radii[3];
    std::vector<float> edge_mix;
    if (blend_edge) edge_mix.resize(next.rgba.size() / 4);
    const auto edge_index = [&](int x, int y) {
        return std::size_t(y - area.top) * next.width + (x - area.left);
    };
    std::atomic_bool has_feather = false;
    if (!filter(3, boxes[3]) || !for_pixels([&](const float* src, float* dest, int x, int y) {
            dest[3] = blur_alpha ? std::clamp(sample(x, y).value, 0.f, 1.f) : src[3];
            if (!blend_edge || dest[3] <= 0) return;
            // Blurring the support of the original matte locates the feather
            // independently of the opacity of an otherwise uniform layer.
            const float coverage = sample(x, y).coverage;
            const float t = src[3] <= 0 ? 1.f : std::clamp((1.f - coverage) / .35f, 0.f, 1.f);
            const float weight = t * t * (3.f - 2.f * t);
            edge_mix[edge_index(x, y)] = weight;
            if (weight > 0 && !has_feather.load(std::memory_order_relaxed))
                has_feather.store(true, std::memory_order_relaxed);
        })) return false;

    const bool feathered = has_feather.load(std::memory_order_relaxed);
    for (int channel = 0; channel < 3; ++channel) {
        const bool feather_color = feathered && radii[channel] < radii[3];
        const auto color = [&](int x, int y) {
            const auto& value = sample(x, y);
            return value.coverage > 0 ? value.value / value.coverage : 0.f;
        };
        if (!filter(channel, boxes[channel]) || !for_pixels([&](const float* src, float* dest, int x, int y) {
                if (dest[3] <= 0 || (src[3] <= 0 && feather_color)) return;
                dest[channel] = boxes[channel].reach() ? color(x, y) : component(src, channel, luma_chroma);
            })) return false;
        if (feather_color) {
            // Use the same visible-color footprint on both sides of the old
            // edge, tapering smoothly to the independently blurred interior.
            if (!filter(channel, boxes[3]) || !for_pixels([&](const float*, float* dest, int x, int y) {
                    const float weight = edge_mix[edge_index(x, y)];
                    if (weight > 0) dest[channel] = std::lerp(dest[channel], color(x, y), weight);
                })) return false;
        }
    }
    const double mix = p.mix * .01;
    if (!for_pixels([&](const float* src, float* dest, int, int) {
            if (luma_chroma && dest[3] > 0) {
                const double y = dest[0], r = dest[1], b = dest[2];
                dest[0] = float(y + r);
                dest[1] = float(y - (red_weight * r + blue_weight * b) / green_weight);
                dest[2] = float(y + b);
            }
            if (mix < 1) {
                const double a = dest[3] * mix, original = src[3] * (1 - mix);
                const double combined = a + original;
                if (combined > 0) for (int k = 0; k < 3; ++k)
                    dest[k] = float((dest[k] * a + src[k] * original) / combined);
                dest[3] = float(combined);
            }
        }) || (cancel && cancel())) return false;
    output = std::move(next);
    return true;
}

} // namespace channelblur
