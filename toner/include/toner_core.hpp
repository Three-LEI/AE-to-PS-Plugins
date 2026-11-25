#pragma once
#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace toner {
inline constexpr int duotone = 0;
inline constexpr int tritone = 1;
inline constexpr int pentone = 2;
inline constexpr int solid = 3;

struct Parameters {
    int mode = tritone;
    std::array<double, 3> highlights{1., 1., 1.};
    std::array<double, 3> brights{.75, .75, .75};
    std::array<double, 3> midtones{.5, .5, .5};
    std::array<double, 3> darktones{.25, .25, .25};
    std::array<double, 3> shadows{0., 0., 0.};
    double blend = 0.; // 0 = effect only, 1 = original only (AE "Blend w. Original")
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

// Mapped color before blending with the source. Values are clamped to 0..1.
void toned_color(const std::array<double, 3>& source, const Parameters&, std::array<double, 3>& result);

bool render(const Image&, const Parameters&, Image&, const Cancel& = {});

} // namespace toner
