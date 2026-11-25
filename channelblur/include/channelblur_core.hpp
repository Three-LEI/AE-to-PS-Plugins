#pragma once

#include "../../native/common/raster_image.hpp"

namespace channelblur {

using pscpu::Cancel;
using pscpu::Image;
using pscpu::Rect;
using pscpu::SampleGrid;

enum Mode { Channels, LuminanceChroma };
enum Axis { Both, Horizontal, Vertical };

struct Parameters {
    int mode = Channels, axis = Both;
    double red = 0, green = 0, blue = 0, alpha = 0;
    double luminance = 0, chroma = 6, mix = 100;
};

bool valid(const Parameters&);
bool render(const Image&, const Parameters&, Image&, const Cancel& = {});
bool render_region(const Image&, const Parameters&, Rect, SampleGrid, Image&, const Cancel& = {});

} // namespace channelblur
