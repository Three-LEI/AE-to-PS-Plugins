#pragma once

#include "../../native/common/raster_image.hpp"

namespace pixelsort {

using pscpu::Cancel;
using pscpu::Image;
using pscpu::Rect;
using pscpu::SampleGrid;

enum Direction { Horizontal, Vertical };
enum Order { DarkToLight, LightToDark };

struct Parameters {
    int direction = Horizontal, order = DarkToLight;
    double low = 22, high = 85, length = 104, mix = 100;
};

bool valid(const Parameters&);
bool render_region(const Image&, const Parameters&, Rect, SampleGrid, Image&, const Cancel& = {});
bool render(const Image&, const Parameters&, Image&, const Cancel& = {});

} // namespace pixelsort
