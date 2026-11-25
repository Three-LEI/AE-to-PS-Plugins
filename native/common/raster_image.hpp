#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

namespace pscpu {

struct Rect {
    int left = 0, top = 0, right = 0, bottom = 0;
    bool empty() const { return left >= right || top >= bottom; }
    int width() const { return right - left; }
    int height() const { return bottom - top; }
    bool contains(Rect r) const {
        return !r.empty() && left <= r.left && top <= r.top && right >= r.right && bottom >= r.bottom;
    }
};

inline Rect intersect(Rect a, Rect b) {
    return {std::max(a.left, b.left), std::max(a.top, b.top),
            std::min(a.right, b.right), std::min(a.bottom, b.bottom)};
}

struct Image {
    int width = 0, height = 0, left = 0, top = 0;
    std::vector<float> rgba;

    bool valid() const {
        return width > 0 && height > 0 && width <= 2000000 && height <= 2000000 &&
               std::uint64_t(width) * height <= 16777216 &&
               std::int64_t(left) + width <= std::numeric_limits<int>::max() &&
               std::int64_t(top) + height <= std::numeric_limits<int>::max() &&
               rgba.size() == std::size_t(width) * height * 4;
    }
    Rect bounds() const { return {left, top, left + width, top + height}; }
    const float* at(int x, int y) const {
        return rgba.data() + (std::size_t(y - top) * width + (x - left)) * 4;
    }
    float* at(int x, int y) { return const_cast<float*>(std::as_const(*this).at(x, y)); }
};

struct SampleGrid {
    double left = 0, top = 0, step = 1;
};

using Cancel = std::function<bool()>;

} // namespace pscpu
