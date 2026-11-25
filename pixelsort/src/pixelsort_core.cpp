#include "pixelsort_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <cmath>

namespace pixelsort {
namespace {

struct Pixel {
    int index;
    double value;
};

} // namespace

bool valid(const Parameters& p) {
    return p.direction >= Horizontal && p.direction <= Vertical && p.order >= DarkToLight && p.order <= LightToDark &&
        p.low >= 0 && p.high >= p.low && p.high <= 100 && p.length >= 1 && p.length <= 8192 &&
        p.mix >= 0 && p.mix <= 100;
}

bool render_region(const Image& source, const Parameters& p, Rect region, SampleGrid grid, Image& output, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || !(grid.step >= 1 && grid.step <= 2000000) || (cancel && cancel())) return false;
    const auto area = pscpu::intersect(source.bounds(), region);
    if (area.empty()) return false;
    Image next{area.width(), area.height(), area.left, area.top,
               std::vector<float>(std::size_t(area.width()) * area.height() * 4)};
    const bool horizontal = p.direction == Horizontal;
    const int length = horizontal ? source.width : source.height;
    const int limit = std::min(length, int(std::ceil(p.length / grid.step)));
    const int lines = horizontal ? area.height() : area.width();
    const int first_pixel = horizontal ? area.left - source.left : area.top - source.top;
    const int end_pixel = first_pixel + (horizontal ? area.width() : area.height());
    const double low = .01 * p.low, high = .01 * p.high;
    const float amount = float(.01 * p.mix);
    const bool descending = p.order == LightToDark;

    const auto render_lines = [&](std::size_t first, std::size_t end, const auto& stopped) {
        std::vector<Pixel> run;
        run.reserve(limit);
        for (auto line = first; line < end; ++line) {
            if (stopped()) return false;
            const auto input = [&](int i) {
                return horizontal ? source.at(source.left + i, area.top + int(line))
                                  : source.at(area.left + int(line), source.top + i);
            };
            const auto dest = [&](int i) {
                return horizontal ? next.at(source.left + i, area.top + int(line))
                                  : next.at(area.left + int(line), source.top + i);
            };
            for (int i = first_pixel; i < end_pixel; ++i) std::copy_n(input(i), 4, dest(i));
            if (limit <= 1 || amount == 0) continue;

            // Anchor runs to the full scanline, including pixels outside the
            // viewport or selection. Only the requested outputs are written.
            const auto finish = [&] {
                if (run.size() > 1) {
                    const int begin = run.front().index;
                    const int left = std::max(begin, first_pixel), right = std::min(begin + int(run.size()), end_pixel);
                    if (left < right) {
                        std::sort(run.begin(), run.end(), [descending](const Pixel& a, const Pixel& b) {
                            if (a.value == b.value) return a.index < b.index;
                            return descending ? a.value > b.value : a.value < b.value;
                        });
                        for (int i = left; i < right; ++i) {
                            float* dst = dest(i);
                            const auto* pixel = input(run[i - begin].index);
                            for (int c = 0; c < 3; ++c) dst[c] = std::lerp(dst[c], pixel[c], amount);
                        }
                    }
                }
                run.clear();
            };
            for (int i = 0; i < length; ++i) {
                if ((i & 4095) == 0 && stopped()) return false;
                const auto* pixel = input(i);
                const double value = .2126 * pixel[0] + .7152 * pixel[1] + .0722 * pixel[2];
                const double range = std::clamp(value, 0., 1.);
                if (pixel[3] > 0 && range >= low && range <= high) {
                    run.push_back({i, value});
                    if (int(run.size()) == limit) finish();
                } else {
                    finish();
                }
            }
            finish();
        }
        return true;
    };
    if (!pscpu::parallel_ranges(lines, std::size_t(lines) * length, cancel, render_lines) || (cancel && cancel())) return false;
    output = std::move(next);
    return true;
}

bool render(const Image& source, const Parameters& p, Image& output, const Cancel& cancel) {
    return source.valid() && render_region(source, p, source.bounds(),
        {double(source.left), double(source.top), 1}, output, cancel);
}

} // namespace pixelsort
