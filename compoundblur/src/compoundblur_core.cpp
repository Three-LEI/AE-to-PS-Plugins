#include "compoundblur_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <cmath>

namespace compoundblur {
namespace {

using Pixel = std::array<float, 4>;
constexpr double max_radius = 256;
constexpr int max_level = 8;

constexpr double variance(int level) {
    if (level == 0) return 0;
    const double stride_squared = double(1u << (2 * level));
    // The first reduction adds 3/4. Later ones include a coarse binomial
    // prefilter and add 11/4 at the previous scale. Reconstruction adds 1/3
    // at the current scale, independent of the position between samples.
    return .75 + 2.75 * (stride_squared - 4) / 3 + stride_squared / 3;
}

static_assert(variance(max_level) >= max_radius * max_radius);

int upper_level(double target) {
    int level = 1;
    while (variance(level) < target) ++level;
    return level;
}

Pixel premultiply(const float* pixel) {
    const float alpha = std::clamp(pixel[3], 0.f, 1.f);
    if (!(alpha > 0)) return {};
    return {pixel[0] * alpha, pixel[1] * alpha, pixel[2] * alpha, alpha};
}

bool reduce(const Image& source, bool straight, Image& output, const Cancel& cancel) {
    const int width = (source.width + 1) / 2, height = (source.height + 1) / 2;
    Image next{width, height, 0, 0, std::vector<float>(std::size_t(width) * height * 4)};
    const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
        constexpr float tap[] = {.125f, .375f, .375f, .125f};
        for (auto row = first; row < end; ++row) {
            if (stopped()) return false;
            const int y = int(row);
            std::array<const float*, 4> source_rows;
            for (int dy = 0; dy < 4; ++dy)
                source_rows[dy] = source.at(source.left, source.top + std::clamp(2 * y + dy - 1, 0, source.height - 1));
            for (int x = 0; x < width; ++x) {
                auto* dst = next.at(x, y);
                for (int dx = 0; dx < 4; ++dx) {
                    const int offset = 4 * std::clamp(2 * x + dx - 1, 0, source.width - 1);
                    for (int dy = 0; dy < 4; ++dy) {
                        const float* src = source_rows[dy] + offset;
                        const float weight = tap[dx] * tap[dy];
                        if (straight) {
                            const auto pixel = premultiply(src);
                            for (int c = 0; c < 4; ++c) dst[c] += pixel[c] * weight;
                        } else {
                            for (int c = 0; c < 4; ++c) dst[c] += src[c] * weight;
                        }
                    }
                }
            }
        }
        return true;
    };
    if (!pscpu::parallel_ranges(height, std::size_t(width) * height * 16, cancel, rows)) return false;
    output = std::move(next);
    return true;
}

bool soften_level(const Image& source, Image& output, const Cancel& cancel) {
    Image next{source.width, source.height, 0, 0, std::vector<float>(source.rgba.size())};
    const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
        for (auto row = first; row < end; ++row) {
            if (stopped()) return false;
            const int y = int(row);
            const float* above = source.at(0, std::max(0, y - 1));
            const float* center = source.at(0, y);
            const float* below = source.at(0, std::min(source.height - 1, y + 1));
            for (int x = 0; x < source.width; ++x) {
                const int left = 4 * std::max(0, x - 1), middle = 4 * x, right = 4 * std::min(source.width - 1, x + 1);
                const auto horizontal = [&](const float* line, int c) {
                    return .25f * (line[left + c] + line[right + c]) + .5f * line[middle + c];
                };
                auto* dst = next.at(x, y);
                for (int c = 0; c < 4; ++c)
                    dst[c] = .25f * (horizontal(above, c) + horizontal(below, c)) + .5f * horizontal(center, c);
            }
        }
        return true;
    };
    if (!pscpu::parallel_ranges(next.height, next.rgba.size() / 4, cancel, rows)) return false;
    output = std::move(next);
    return true;
}

bool prepare(const Image& source, int count, BlurPyramid& pyramid, const Cancel& cancel) {
    if (pyramid.pixels != source.rgba.data() || pyramid.width != source.width || pyramid.height != source.height) {
        pyramid = {source.rgba.data(), source.width, source.height, {}};
    }
    while (int(pyramid.levels.size()) < count) {
        Image level;
        const bool first = pyramid.levels.empty();
        if (!reduce(first ? source : pyramid.levels.back(), first, level, cancel)) return false;
        if (!first && !soften_level(level, level, cancel)) return false;
        pyramid.levels.push_back(std::move(level));
    }
    return true;
}

struct CubicSpan {
    int first;
    std::array<float, 4> weights;
};

// A positive cubic B-spline keeps the slope continuous across scale texels.
CubicSpan cubic_span(int coordinate, int level) {
    const int stride = 1 << level;
    const int offset = coordinate + stride / 2;
    const float t = (float(offset & (stride - 1)) + .5f) / float(stride);
    const float u = 1 - t, t2 = t * t, t3 = t2 * t;
    const float w0 = u * u * u / 6, w1 = (4 - 6 * t2 + 3 * t3) / 6, w3 = t3 / 6;
    return {(offset >> level) - 2, {w0, w1, 1 - w0 - w1 - w3, w3}};
}

struct ScaleRows {
    std::array<const float*, 4> pixels;
    std::array<float, 4> weights;
    int width;
    struct Column {
        int x = -1;
        Pixel value{};
    };
    std::array<Column, 4> columns{};
};

ScaleRows scale_rows(const Image& image, int level, int y) {
    const auto span = cubic_span(y, level);
    ScaleRows rows{{}, span.weights, image.width};
    for (int i = 0; i < 4; ++i) rows.pixels[i] = image.at(0, std::clamp(span.first + i, 0, image.height - 1));
    return rows;
}

Pixel sample_level(ScaleRows& rows, int level, int x) {
    const auto span = cubic_span(x, level);
    Pixel result{};
    // Adjacent native pixels reuse the vertically interpolated coarse columns.
    for (int i = 0; i < 4; ++i) {
        const int column = std::clamp(span.first + i, 0, rows.width - 1);
        auto& sample = rows.columns[column & 3];
        if (sample.x != column) {
            sample = {column, {}};
            for (int y = 0; y < 4; ++y) {
                const auto* pixel = rows.pixels[y] + 4 * column;
                for (int c = 0; c < 4; ++c) sample.value[c] += pixel[c] * rows.weights[y];
            }
        }
        for (int c = 0; c < 4; ++c) result[c] += sample.value[c] * span.weights[i];
    }
    return result;
}

template<class T>
double luminance(T r, T g, T b) {
    const double value = .2126 * r + .7152 * g + .0722 * b;
    return std::isnan(value) ? 0 : std::clamp(value, 0., 1.);
}

struct Map {
    const Parameters& p;
    const Image* reference;
    pscpu::gradient::Field field;

    Map(const Parameters& values, const Image* image, Rect domain) : p(values), reference(image),
        field(p.placement, domain.left, domain.top, domain.width(), domain.height()) {}

    double at(const float* current, double x, double y) const {
        double amount = 0;
        if (p.source == Gradient) {
            const auto color = pscpu::gradient::sample(p.gradient, field.at(x, y));
            amount = luminance(color[0], color[1], color[2]) * color[3];
        } else {
            auto* pixel = current;
            if (reference) {
                const auto bounds = reference->bounds();
                pixel = x >= bounds.left && x < bounds.right && y >= bounds.top && y < bounds.bottom
                    ? reference->at(int(std::floor(x)), int(std::floor(y))) : nullptr;
            }
            if (pixel && pixel[3] > 0) {
                const double alpha = std::min(double(pixel[3]), 1.);
                amount = p.channel == Alpha ? alpha : alpha * luminance(pixel[0], pixel[1], pixel[2]);
            }
        }
        return p.invert ? 1 - amount : amount;
    }
};

} // namespace

bool valid(const Parameters& p) {
    const auto within = [](double value, double low, double high) { return value >= low && value <= high; };
    return p.source >= Gradient && p.source <= Reference && p.channel >= Luminance && p.channel <= Alpha &&
        within(p.radius, 0, max_radius) && within(p.mix, 0, 100) &&
        pscpu::gradient::valid(p.gradient) && pscpu::gradient::valid(p.placement);
}

bool render_region(const Image& source, const Parameters& p, const Image* reference, Rect domain,
                   Rect region, SampleGrid grid, Image& output, const Cancel& cancel, BlurPyramid* cache, bool show_map) {
    if (!source.valid() || !valid(p) || domain.empty() || !(grid.step >= 1 && grid.step <= 2000000) ||
        (p.source == Reference && p.layer_key && (!reference || !reference->valid())) || (cancel && cancel())) return false;
    const auto area = pscpu::intersect(source.bounds(), region);
    if (area.empty()) return false;
    Image next{area.width(), area.height(), area.left, area.top,
               std::vector<float>(std::size_t(area.width()) * area.height() * 4)};
    const bool apply = !show_map && p.mix > 0 && p.radius > 0;
    const double max_sigma = p.radius / grid.step;
    const double mix = p.mix * .01;
    const Map map(p, p.source == Reference ? reference : nullptr, domain);
    BlurPyramid local;
    auto& pyramid = cache ? *cache : local;
    pscpu::ParallelScope parallel;
    const int count = apply ? upper_level(max_sigma * max_sigma) : 0;
    if (apply && !prepare(source, count, pyramid, cancel)) return false;

    const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
        for (auto row = first; row < end; ++row) {
            if (stopped()) return false;
            const int y = area.top + int(row);
            const int input_y = y - source.top;
            const double doc_y = grid.top + (input_y + .5) * grid.step;
            std::array<ScaleRows, max_level> reconstruction;
            for (int level = 1; level <= count; ++level)
                reconstruction[level - 1] = scale_rows(pyramid.levels[level - 1], level, input_y);
            const float* src = source.at(area.left, y);
            float* dst = next.at(area.left, y);
            for (int x = area.left; x < area.right; ++x, src += 4, dst += 4) {
                std::copy_n(src, 4, dst);
                if (!show_map && (!apply || src[3] <= 0)) continue;
                const int input_x = x - source.left;
                const double amount = map.at(src, grid.left + (input_x + .5) * grid.step, doc_y);
                if (show_map) {
                    std::fill_n(dst, 3, float(amount));
                    dst[3] = 1;
                    continue;
                }
                if (amount <= 0) continue;
                const double sigma = max_sigma * amount;
                const double target = sigma * sigma;
                const int upper = upper_level(target);
                const int lower = upper - 1;
                const auto small = lower == 0 ? premultiply(src) : sample_level(reconstruction[lower - 1], lower, input_x);
                const auto large = sample_level(reconstruction[upper - 1], upper, input_x);
                const float ratio = float((target - variance(lower)) / (variance(upper) - variance(lower)));
                const float weight = std::lerp(small[3], large[3], ratio);
                if (weight > 0) {
                    for (int c = 0; c < 3; ++c)
                        dst[c] = std::lerp(src[c], std::lerp(small[c], large[c], ratio) / weight, float(mix));
                }
            }
        }
        return true;
    };
    if (!pscpu::parallel_ranges(next.height, next.rgba.size() / 4, cancel, rows) || (cancel && cancel())) return false;
    output = std::move(next);
    return true;
}

bool render(const Image& source, const Parameters& p, const Image* reference, Rect domain, Image& output, const Cancel& cancel) {
    return source.valid() && render_region(source, p, reference, domain, source.bounds(),
        {double(source.left), double(source.top), 1}, output, cancel);
}

} // namespace compoundblur
