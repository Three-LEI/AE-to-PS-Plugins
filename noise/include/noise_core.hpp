#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace noise {
enum Distribution { Uniform, Gaussian };
enum class Preset { FineNoise, SoftFilm, TVSnow, VHS, FilmAndVHS, CoarseFilm };

struct Parameters {
    bool basic = true;
    int distribution = Gaussian;
    double amount = 4, size = 1, color = 0;
    double hue = 0, lightness = 0, saturation = 0;
    bool grain = false;
    double grain_amount = 14, grain_size = 1.5, grain_softness = 20, grain_color = 0;
    double shadows = 55, midtones = 100, highlights = 25;
    bool signal = false;
    double snow = 18, bands = 15, chroma_bleed = 5, dropouts = 12;
    int seed = 1, phase = 0;
    double grain_midpoint = 50, grain_roughness = 0, mix = 100;
    bool operator==(const Parameters&) const = default;
};

struct Image {
    int width = 0, height = 0, left = 0, top = 0;
    std::vector<float> rgba;
    bool valid() const {
        return width > 0 && height > 0 && width <= 2000000 && height <= 2000000 &&
               std::uint64_t(width) * height <= 16777216 &&
               rgba.size() == std::size_t(width) * height * 4;
    }
};

// Reduced previews still sample at full-resolution document coordinates.
struct RenderSpace { double left = 0, top = 0, step = 0; };
struct Region {
    int x, y, width, height;
    bool operator==(const Region&) const = default;
};
using Cancel = std::function<bool()>;

Parameters preset(Preset);
bool valid(const Parameters&);
bool render(const Image&, const Parameters&, Image&, const Cancel& = {}, RenderSpace = {});
bool render_region(const Image&, const Parameters&, Region, Image&, const Cancel& = {});
}
