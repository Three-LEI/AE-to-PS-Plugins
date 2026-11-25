#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace psui {
template<class Image>
std::array<float, 4> sample_area(const Image& image, double left, double top,
                                 double right, double bottom, bool premultiplied = true) {
    const double l = std::clamp(left, 0., double(image.width));
    const double t = std::clamp(top, 0., double(image.height));
    const double r = std::clamp(right, 0., double(image.width));
    const double b = std::clamp(bottom, 0., double(image.height));
    if (r <= l || b <= t) return {};

    const int first_x = int(std::floor(l)), end_x = int(std::ceil(r)), end_y = int(std::ceil(b));
    double sum[4]{}, total = 0;
    for (int y = int(std::floor(t)); y < end_y; ++y) {
        const double height = std::max(0., std::min(b, y + 1.) - std::max(t, double(y)));
        for (int x = first_x; x < end_x; ++x) {
            const double width = std::max(0., std::min(r, x + 1.) - std::max(l, double(x)));
            const double weight = width * height;
            const auto i = (std::size_t(y) * image.width + x) * 4;
            const auto source_alpha = image.rgba[i + 3];
            const double alpha = std::isfinite(source_alpha) ? std::clamp(double(source_alpha), 0., 1.) : 0.;
            total += weight;
            sum[3] += alpha * weight;
            for (int k = 0; k < 3; ++k) {
                const auto sample = image.rgba[i + k];
                sum[k] += (std::isfinite(sample) ? double(sample) : 0.) * weight * (premultiplied ? alpha : 1.);
            }
        }
    }
    const double divisor = premultiplied ? sum[3] : total;
    return {divisor > 0 ? float(sum[0] / divisor) : 0, divisor > 0 ? float(sum[1] / divisor) : 0,
            divisor > 0 ? float(sum[2] / divisor) : 0, total > 0 ? float(sum[3] / total) : 0};
}
}
