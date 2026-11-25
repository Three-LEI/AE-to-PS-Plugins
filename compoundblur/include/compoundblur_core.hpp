#pragma once

#include "../../native/common/gradient.hpp"
#include "../../native/common/raster_image.hpp"

namespace compoundblur {

using pscpu::Cancel;
using pscpu::Image;
using pscpu::Rect;
using pscpu::SampleGrid;

enum Source { Gradient, Reference };
enum Channel { Luminance, Alpha };

struct Parameters {
    int source = Gradient, channel = Luminance;
    std::uint64_t layer_key = 0;
    pscpu::gradient::Ramp gradient;
    pscpu::gradient::Placement placement;
    double radius = 24, mix = 100;
    bool invert = false;
};

// Snapshots supplied to a cache stay immutable for the duration of a dialog.
struct BlurPyramid {
    const float* pixels = nullptr;
    int width = 0, height = 0;
    std::vector<Image> levels;
};

bool valid(const Parameters&);
bool render_region(const Image&, const Parameters&, const Image* reference, Rect domain,
                   Rect region, SampleGrid, Image&, const Cancel& = {}, BlurPyramid* = nullptr,
                   bool show_map = false);
bool render(const Image&, const Parameters&, const Image* reference, Rect domain, Image&,
            const Cancel& = {});

} // namespace compoundblur
