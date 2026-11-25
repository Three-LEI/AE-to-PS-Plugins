#pragma once

#include "../../native/common/raster_image.hpp"

namespace weather {

using pscpu::Cancel;
using pscpu::Image;
using pscpu::Rect;
using pscpu::SampleGrid;

enum Mode { Rain, Snow };
enum Output { Composite, Particles };

struct Parameters {
    int mode = Rain, output = Composite, seed = 17;
    double density = 46, opacity = 72, wind = 14;
    double rain_length = 38, rain_width = 1.6, snow_size = 6;
    double near_blur = 2.4, far_blur = .45;
    double rain_color[3] = {.83, .91, 1};
    double snow_color[3] = {.97, .98, 1};
};

bool valid(const Parameters&);
bool render(const Image&, const Parameters&, Image&, const Cancel& = {});
bool render_region(const Image&, const Parameters&, Rect, SampleGrid, Image&, const Cancel& = {});

} // namespace weather
