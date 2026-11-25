#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace mirror {
struct Parameters {
    double cx = 0, cy = 0;
    double angle = 0;
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
Parameters defaults(int width, int height, int left = 0, int top = 0);
Parameters proxy_parameters(const Parameters&, double origin_x, double origin_y, double step);
bool valid(const Parameters&);
bool render(const Image&, const Parameters&, Image&, const Cancel& = {});
} // namespace mirror
