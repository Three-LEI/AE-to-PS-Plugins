#pragma once

#include "../../native/common/raster_image.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace repetile {
enum Tiling {
    Repeat = 0,
    CheckerFlipH = 1,
    CheckerFlipV = 2,
    Unfold = 3,
    Checker180 = 4,
    Random = 5,
    None = 6,
    Slide = 7,
    Brick = 8,
};

struct Parameters {
    double left = 0, right = 0, up = 0, down = 0;
    double blend = 0;
    int tiling = Repeat;
    int seed = 1;
    bool operator==(const Parameters&) const = default;
};

struct Image {
    int width = 0, height = 0, left = 0, top = 0;
    std::vector<float> rgba;
    bool valid() const {
        return width > 0 && height > 0 && width <= 2000000 && height <= 2000000 &&
               std::uint64_t(width) * height <= 16777216 && rgba.size() == std::size_t(width) * height * 4;
    }
};

using Cancel = std::function<bool()>;
Parameters defaults();
bool valid(const Parameters&);
pscpu::Rect tile_bounds(const Image&);
bool render(const Image&, const Parameters&, Image&, const Cancel& = {});
} // namespace repetile
