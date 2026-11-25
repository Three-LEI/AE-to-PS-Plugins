#include "repetile_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace repetile {
namespace {
struct Box {
    int left = 0, top = 0, right = 0, bottom = 0;
    int width() const { return right - left; }
    int height() const { return bottom - top; }
    bool empty() const { return right <= left || bottom <= top; }
};

long long parity(long long value) { return ((value % 2) + 2) % 2; }

Box content(const Image& source, const Cancel& cancel, bool& stopped) {
    Box box{std::numeric_limits<int>::max(), std::numeric_limits<int>::max(),
            std::numeric_limits<int>::min(), std::numeric_limits<int>::min()};
    for (int y = 0; y < source.height; ++y) {
        if (cancel && cancel()) { stopped = true; return {}; }
        for (int x = 0; x < source.width; ++x) {
            const auto alpha = source.rgba[(std::size_t(y) * source.width + x) * 4 + 3];
            if (alpha <= 0.001f) continue;
            box.left = std::min(box.left, source.left + x);
            box.top = std::min(box.top, source.top + y);
            box.right = std::max(box.right, source.left + x + 1);
            box.bottom = std::max(box.bottom, source.top + y + 1);
        }
    }
    if (box.right <= box.left || box.bottom <= box.top) return Box{};
    return box;
}

std::array<float, 4> sample(const Image& source, double x, double y) {
    const double max_x = source.left + source.width - 1;
    const double max_y = source.top + source.height - 1;
    x = std::clamp(x, double(source.left), max_x);
    y = std::clamp(y, double(source.top), max_y);
    const int x0 = int(std::floor(x)), y0 = int(std::floor(y));
    const int x1 = std::min(x0 + 1, source.left + source.width - 1);
    const int y1 = std::min(y0 + 1, source.top + source.height - 1);
    const double fx = x - x0, fy = y - y0;
    std::array<float, 4> result{};
    for (int k = 0; k < 4; ++k) {
        const auto at = [&](int px, int py) {
            return source.rgba[(std::size_t(py - source.top) * source.width + (px - source.left)) * 4 + k];
        };
        const double top = at(x0, y0) * (1 - fx) + at(x1, y0) * fx;
        const double bottom = at(x0, y1) * (1 - fx) + at(x1, y1) * fx;
        result[k] = float(top * (1 - fy) + bottom * fy);
    }
    return result;
}

std::array<float, 4> mix(const std::array<float, 4>& a, const std::array<float, 4>& b, double weight_a) {
    std::array<float, 4> result{};
    for (int k = 0; k < 4; ++k) result[k] = float(a[k] * weight_a + b[k] * (1 - weight_a));
    return result;
}

bool blendable(int tiling) {
    return tiling != Unfold && tiling != None && tiling != Slide;
}
} // namespace

Parameters defaults() { return {}; }

pscpu::Rect tile_bounds(const Image& source) {
    bool stopped = false;
    const auto bounds = source.valid() ? content(source, {}, stopped) : Box{};
    return {bounds.left, bounds.top, bounds.right, bounds.bottom};
}

bool valid(const Parameters& p) {
    const auto finite = [](double v) { return std::isfinite(v); };
    if (!finite(p.left) || p.left < 0 || p.left > 100000) return false;
    if (!finite(p.right) || p.right < 0 || p.right > 100000) return false;
    if (!finite(p.up) || p.up < 0 || p.up > 100000) return false;
    if (!finite(p.down) || p.down < 0 || p.down > 100000) return false;
    if (!finite(p.blend) || p.blend < 0 || p.blend > 1000) return false;
    if (p.tiling < Repeat || p.tiling > Brick) return false;
    return true;
}

bool render(const Image& source, const Parameters& p, Image& out, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || (cancel && cancel())) return false;
    if (p.tiling == None || std::max({p.left,p.right,p.up,p.down}) < .5) {
        if (&out != &source) out = source;
        return true;
    }
    Image next = source;
    bool stopped = false;
    const Box box = content(source,cancel,stopped);
    if (stopped) return false;
    if (box.empty()) { out = std::move(next); return true; }
    const int left = std::max(source.left, box.left - int(std::lround(p.left)));
    const int top = std::max(source.top, box.top - int(std::lround(p.up)));
    const int right = std::min(source.left + source.width, box.right + int(std::lround(p.right)));
    const int bottom = std::min(source.top + source.height, box.bottom + int(std::lround(p.down)));
    const double tile_w = box.width(), tile_h = box.height();
    const auto rows=std::size_t(bottom-top);
    if (!pscpu::parallel_ranges(rows,rows*(right-left),cancel,[&](std::size_t first,std::size_t last,const auto& stop) {
    for (int y = top+int(first); y < top+int(last); ++y) {
        if (stop()) return false;
        for (int x = left; x < right; ++x) {
            if (x >= box.left && x < box.right && y >= box.top && y < box.bottom) continue;
            double u = (x + 0.5 - box.left) / tile_w;
            double v = (y + 0.5 - box.top) / tile_h;
            if (p.tiling == Brick && parity(static_cast<long long>(std::floor(v)))) u += 0.5;
            if (p.tiling == Slide && parity(static_cast<long long>(std::floor(u)))) v += 0.5;
            const long long tx = static_cast<long long>(std::floor(u));
            const long long ty = static_cast<long long>(std::floor(v));
            double fx = u - std::floor(u), fy = v - std::floor(v);
            if (p.tiling == CheckerFlipH && parity(tx)) fx = 1 - fx;
            else if (p.tiling == CheckerFlipV && parity(ty)) fy = 1 - fy;
            else if (p.tiling == Unfold) {
                if (parity(tx)) fx = 1 - fx;
                if (parity(ty)) fy = 1 - fy;
            } else if (p.tiling == Checker180 && parity(tx + ty)) {
                fx = 1 - fx;
                fy = 1 - fy;
            } else if (p.tiling == Random) {
                std::uint32_t h = std::uint32_t(p.seed) * 0x9E3779B1u;
                h ^= std::uint32_t(tx) * 0x85EBCA77u;
                h ^= std::uint32_t(ty) * 0xC2B2AE3Du;
                h ^= h >> 15;
                if (h & 1) fx = 1 - fx;
                if (h & 2) fy = 1 - fy;
            }
            double sx = box.left + fx * tile_w - 0.5;
            double sy = box.top + fy * tile_h - 0.5;
            auto color = sample(source, sx, sy);
            if (p.blend > 0 && blendable(p.tiling)) {
                const double dx = std::min(fx, 1 - fx) * tile_w;
                const double dy = std::min(fy, 1 - fy) * tile_h;
                if (dx <= dy && dx < p.blend) {
                    const double weight = .5 + .5 * (dx / p.blend);
                    const double alt = fx < .5 ? box.right - dx - .5 : box.left + dx - .5;
                    color = mix(color, sample(source, alt, sy), weight);
                } else if (dy < p.blend) {
                    const double weight = .5 + .5 * (dy / p.blend);
                    const double alt = fy < .5 ? box.bottom - dy - .5 : box.top + dy - .5;
                    color = mix(color, sample(source, sx, alt), weight);
                }
            }
            const auto i = (std::size_t(y - source.top) * source.width + (x - source.left)) * 4;
            for (int k = 0; k < 4; ++k) next.rgba[i + k] = color[k];
        }
    }
    return true;
    })) return false;
    out = std::move(next);
    return true;
}
} // namespace repetile
