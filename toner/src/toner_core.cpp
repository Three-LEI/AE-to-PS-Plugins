#include "toner_core.hpp"
#include <algorithm>
#include <cmath>

namespace toner {
namespace {
struct Stop {
    double at;
    std::array<double, 3> color;
};

double luminance(const std::array<double, 3>& c) {
    return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2];
}

void clamp_color(std::array<double, 3>& c) {
    for (auto& v : c) v = std::clamp(v, 0., 1.);
}

int build_stops(const Parameters& p, Stop (&stops)[5]) {
    switch (p.mode) {
        case duotone:
            stops[0] = {0., p.shadows};
            stops[1] = {1., p.highlights};
            return 2;
        case tritone:
            stops[0] = {0., p.shadows};
            stops[1] = {.5, p.midtones};
            stops[2] = {1., p.highlights};
            return 3;
        case pentone:
            stops[0] = {0., p.shadows};
            stops[1] = {.25, p.darktones};
            stops[2] = {.5, p.midtones};
            stops[3] = {.75, p.brights};
            stops[4] = {1., p.highlights};
            return 5;
        default:
            return 0;
    }
}
} // namespace

Parameters defaults() { return {}; }

bool valid(const Parameters& p) {
    if (p.mode < duotone || p.mode > solid) return false;
    if (!std::isfinite(p.blend) || p.blend < 0. || p.blend > 1.) return false;
    for (const auto* color : {&p.highlights, &p.brights, &p.midtones, &p.darktones, &p.shadows})
        for (const auto v : *color)
            if (!std::isfinite(v) || v < 0. || v > 1.) return false;
    return true;
}

void toned_color(const std::array<double, 3>& source, const Parameters& p, std::array<double, 3>& result) {
    if (p.mode == solid) {
        result = p.highlights;
        clamp_color(result);
        return;
    }
    Stop stops[5];
    const int count = build_stops(p, stops);
    const double l = std::clamp(luminance(source), 0., 1.);
    if (count == 0) {
        result = source;
        return;
    }
    if (l <= stops[0].at) {
        result = stops[0].color;
    } else if (l >= stops[count - 1].at) {
        result = stops[count - 1].color;
    } else {
        for (int i = 0; i + 1 < count; ++i) {
            if (l <= stops[i + 1].at) {
                const double span = stops[i + 1].at - stops[i].at;
                const double t = span > 0. ? (l - stops[i].at) / span : 0.;
                for (int k = 0; k < 3; ++k)
                    result[k] = stops[i].color[k] + (stops[i + 1].color[k] - stops[i].color[k]) * t;
                break;
            }
        }
    }
    clamp_color(result);
}

bool render(const Image& source, const Parameters& p, Image& out, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || (cancel && cancel())) return false;
    Image next = source;
    const double keep = p.blend;
    for (int y = 0; y < source.height; ++y) {
        if (cancel && cancel()) return false;
        for (int x = 0; x < source.width; ++x) {
            const auto i = (std::size_t(y) * source.width + x) * 4;
            const std::array<double, 3> src{source.rgba[i], source.rgba[i + 1], source.rgba[i + 2]};
            std::array<double, 3> toned;
            toned_color(src, p, toned);
            for (int k = 0; k < 3; ++k)
                next.rgba[i + k] = float(toned[k] * (1. - keep) + src[k] * keep);
        }
    }
    out = std::move(next);
    return true;
}

} // namespace toner
