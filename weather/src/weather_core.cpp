#include "weather_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <array>
#include <cmath>
#include <numbers>

namespace weather {
namespace {

// Cell coordinates and seed define the same particles at every view scale.
std::uint32_t scramble(std::uint32_t x) {
    x = (x ^ (x >> 16)) * 0x7feb352dU;
    x = (x ^ (x >> 15)) * 0x846ca68bU;
    return x ^ (x >> 16);
}

struct Random {
    std::uint32_t state;
    double next() {
        state += 0x9e3779b9U;
        return double(scramble(state) >> 8) / 16777216.;
    }
};

struct Field {
    int spacing;
    std::uint32_t seed;
    double length, radius, blur, probability, opacity;
};

struct Brush {
    double x, y, half_length, radius;
    float opacity;
};

struct Raster {
    Image& image;
    double left, top, step, wind_x, wind_y;

    static int pixel(double coordinate, int limit) {
        return int(std::clamp(coordinate, 0., double(limit)));
    }

    void splat(const Brush& b, int first, int end) const {
        const double reach_y = b.half_length * wind_y + b.radius;
        const int y0 = std::max(first, pixel(std::floor((b.y - reach_y - top) / step), image.height));
        const int y1 = std::min(end, pixel(std::ceil((b.y + reach_y - top) / step), image.height));
        const double inverse_radius_squared = 1 / (b.radius * b.radius);
        for (int y = y0; y < y1; ++y) {
            const double dy = top + (y + .5) * step - b.y;
            double t0 = -b.half_length, t1 = b.half_length;
            if (wind_y > 1e-8) {
                t0 = std::max(t0, (dy - b.radius) / wind_y);
                t1 = std::min(t1, (dy + b.radius) / wind_y);
                if (t0 > t1) continue;
            }
            const double x0 = b.x + std::min(t0 * wind_x, t1 * wind_x) - b.radius;
            const double x1 = b.x + std::max(t0 * wind_x, t1 * wind_x) + b.radius;
            const int begin = pixel(std::floor((x0 - left) / step), image.width);
            const int finish = pixel(std::ceil((x1 - left) / step), image.width);
            auto* dest = image.rgba.data() + (std::size_t(y) * image.width + begin) * 4;
            for (int x = begin; x < finish; ++x, dest += 4) {
                const double dx = left + (x + .5) * step - b.x;
                const double along = std::clamp(dx * wind_x + dy * wind_y, -b.half_length, b.half_length);
                const double rx = dx - along * wind_x, ry = dy - along * wind_y;
                const double falloff = 1 - (rx * rx + ry * ry) * inverse_radius_squared;
                if (falloff <= 0) continue;
                const float a = b.opacity * float(falloff * falloff);
                dest[3] += a * (1 - dest[3]);
            }
        }
    }

    bool particles(Field field, bool rain, int first, int end, const auto& stopped) const {
        const double filter = field.blur + step * .55;
        const double max_radius = std::hypot(field.radius * 1.45, filter);
        const double half_length = field.length * .70;
        const double margin_x = half_length * std::abs(wind_x) + max_radius;
        const double margin_y = half_length * wind_y + max_radius;
        const auto cell = [&](double coordinate) { return std::int64_t(std::floor(coordinate / field.spacing)); };
        const auto x0 = cell(left - margin_x), x1 = cell(left + image.width * step + margin_x);
        const auto y0 = cell(top + first * step - margin_y), y1 = cell(top + end * step + margin_y);
        for (auto y = y0; y <= y1; ++y) {
            if (stopped()) return false;
            for (auto x = x0; x <= x1; ++x) {
                if (((x - x0) & 255) == 0 && stopped()) return false;
                Random random{scramble(std::uint32_t(x) * 0x85ebca6bU ^
                                       std::uint32_t(y) * 0xc2b2ae35U ^ field.seed)};
                if (random.next() >= field.probability) continue;
                const double cx = (double(x) + random.next()) * field.spacing;
                const double cy = (double(y) + random.next()) * field.spacing;
                const double length = field.length * (.60 + .80 * random.next());
                const double solid_radius = field.radius * (.55 + .90 * random.next());
                const double radius = std::hypot(solid_radius, filter);
                const double coverage = solid_radius / radius;
                // Keep blurred or reduced-resolution particles from gaining weight.
                const double area = rain ? coverage * std::min(1., length / step) : coverage * coverage;
                const float opacity = float(field.opacity * (.55 + .45 * random.next()) * area);
                splat({cx, cy, length * .5, radius, opacity}, first, end);
            }
        }
        return true;
    }
};

} // namespace

bool valid(const Parameters& p) {
    const auto range = [](double value, double minimum, double maximum) {
        return value >= minimum && value <= maximum;
    };
    const auto color = [&](const double (&rgb)[3]) {
        return std::all_of(rgb, rgb + 3, [&](double v) { return range(v, 0, 1); });
    };
    return p.mode >= Rain && p.mode <= Snow && p.output >= Composite && p.output <= Particles &&
           p.seed >= 0 && p.seed <= 999999 && range(p.density, 0, 100) && range(p.opacity, 0, 100) &&
           range(p.wind, -90, 90) && range(p.rain_length, 1, 400) && range(p.rain_width, .25, 14) &&
           range(p.snow_size, .5, 40) && range(p.near_blur, 0, 24) && range(p.far_blur, 0, 24) &&
           color(p.rain_color) && color(p.snow_color);
}

bool render(const Image& source, const Parameters& p, Image& output, const Cancel& cancel) {
    return source.valid() && render_region(source, p, source.bounds(),
        {double(source.left), double(source.top), 1}, output, cancel);
}

bool render_region(const Image& source, const Parameters& p, Rect region, SampleGrid grid,
                   Image& output, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || !(grid.step >= 1 && grid.step <= 2000000) ||
        !(std::abs(grid.left) <= 2147483648. && std::abs(grid.top) <= 2147483648.) ||
        (cancel && cancel())) return false;
    const auto area = pscpu::intersect(region, source.bounds());
    if (area.empty()) return false;
    Image next{area.width(), area.height(), area.left, area.top,
               std::vector<float>(std::size_t(area.width()) * area.height() * 4)};
    const double angle = p.wind * std::numbers::pi / 180;
    const Raster raster{next, grid.left + (area.left - source.left) * grid.step,
                        grid.top + (area.top - source.top) * grid.step, grid.step, std::sin(angle),
                        std::cos(angle)};
    const bool rain = p.mode == Rain;
    const double density = p.density * .01, opacity = p.opacity * .01;
    const double length = rain ? p.rain_length : p.snow_size * (.15 + .40 * std::abs(raster.wind_x));
    const double diameter = rain ? p.rain_width : p.snow_size;
    const auto seed = std::uint32_t(p.seed);
    const std::array fields{
        Field{rain ? 12 : 21, seed ^ 0xcb1ab31fU, length * .57, diameter * .28, p.far_blur, density, opacity * .62},
        Field{rain ? 24 : 49, seed ^ 0x72dceb65U, length * 1.36, diameter * .63, p.near_blur, density, opacity}
    };
    const auto& rgb = rain ? p.rain_color : p.snow_color;
    const std::array tint{float(rgb[0]), float(rgb[1]), float(rgb[2])};
    constexpr int band_height = 32;
    const auto bands = (next.height + band_height - 1) / band_height;
    const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
        for (auto band = first; band < end; ++band) {
            if (stopped()) return false;
            const int y0 = int(band) * band_height, y1 = std::min(next.height, y0 + band_height);
            if (density > 0 && opacity > 0) {
                for (const auto& field : fields)
                    if (!raster.particles(field, rain, y0, y1, stopped)) return false;
            }
            for (int y = y0; y < y1; ++y) {
                if (stopped()) return false;
                const float* input = source.at(area.left, area.top + y);
                float* dest = next.at(area.left, area.top + y);
                for (int x = 0; x < next.width; ++x, input += 4, dest += 4) {
                    const float a = dest[3];
                    if (p.output == Particles) {
                        if (a > 0) std::copy(tint.begin(), tint.end(), dest);
                    } else if (a == 0) {
                        std::copy_n(input, 4, dest);
                    } else {
                        const float base = std::clamp(input[3], 0.f, 1.f) * (1 - a);
                        dest[3] = a + base;
                        for (int k = 0; k < 3; ++k) dest[k] = (tint[k] * a + input[k] * base) / dest[3];
                    }
                }
            }
        }
        return true;
    };
    if (!pscpu::parallel_ranges(bands, next.rgba.size(), cancel, rows) || (cancel && cancel())) return false;
    output = std::move(next);
    return true;
}

} // namespace weather
