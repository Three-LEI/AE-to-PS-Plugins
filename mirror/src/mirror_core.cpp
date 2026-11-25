#include "mirror_core.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace mirror {
namespace {
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
} // namespace

Parameters defaults(int width, int height, int left, int top) {
    Parameters p;
    p.cx = left + width * 0.5;
    p.cy = top + height * 0.5;
    return p;
}

Parameters proxy_parameters(const Parameters& p, double origin_x, double origin_y, double step) {
    Parameters q = p;
    const double safe_step = std::max(1.0, step);
    q.cx = (p.cx - origin_x) / safe_step;
    q.cy = (p.cy - origin_y) / safe_step;
    return q;
}

bool valid(const Parameters& p) {
    return std::isfinite(p.cx) && std::isfinite(p.cy) && std::isfinite(p.angle) &&
           std::abs(p.cx) <= 1e7 && std::abs(p.cy) <= 1e7 && std::abs(p.angle) <= 360;
}

bool render(const Image& source, const Parameters& p, Image& out, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || (cancel && cancel())) return false;
    Image next = source;
    const double radians = p.angle * std::numbers::pi / 180.;
    const double nx = std::cos(radians), ny = std::sin(radians);
    for (int y = 0; y < source.height; ++y) {
        if (cancel && cancel()) return false;
        for (int x = 0; x < source.width; ++x) {
            const double dx = source.left + x + 0.5, dy = source.top + y + 0.5;
            const double distance = (dx - p.cx) * nx + (dy - p.cy) * ny;
            double sx = dx, sy = dy;
            if (distance >= 0) {
                sx = dx - 2 * distance * nx;
                sy = dy - 2 * distance * ny;
            }
            const auto color = sample(source, sx - 0.5, sy - 0.5);
            const auto i = (std::size_t(y) * source.width + x) * 4;
            for (int k = 0; k < 4; ++k) next.rgba[i + k] = color[k];
        }
    }
    out = std::move(next);
    return true;
}
} // namespace mirror
