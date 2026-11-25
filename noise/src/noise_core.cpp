#include "noise_core.hpp"
#include "../../native/common/grain_noise.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace noise {
namespace {
using RGB = std::array<double, 3>;

double unit(double value) { return std::clamp(value, 0., 1.); }
double clean(float value) { return std::isfinite(value) ? value : 0.; }
double smooth(double t) { return t * t * (3 - 2 * t); }
double luma(const RGB& rgb) { return .299 * rgb[0] + .587 * rgb[1] + .114 * rgb[2]; }

std::uint32_t coordinate_bits(std::int64_t x, std::int64_t y, std::uint32_t seed) {
    return psgrain::hash(seed ^ psgrain::hash(std::uint32_t(x)) ^
                        psgrain::hash(std::uint32_t(y) * 2654435761u));
}

double cell_noise(std::int64_t x, std::int64_t y, std::uint32_t seed, int distribution) {
    const auto first = coordinate_bits(x, y, seed);
    if (distribution == Uniform) return first / 2147483648. - 1.;
    const auto second = coordinate_bits(x, y, seed ^ 0x9e3779b9u);
    auto sum = [](std::uint32_t bits) {
        return (bits & 1023u) + ((bits >> 10) & 1023u) + ((bits >> 20) & 1023u);
    };
    // Six uniforms approximate a bell curve with the same RMS as Uniform.
    return (double(sum(first) + sum(second)) - 3069.) * (.81649697 / 1024.);
}

double pixel_noise(double x, double y, double size, std::uint32_t seed, int distribution) {
    if (size == 1) return cell_noise(std::int64_t(std::floor(x)), std::int64_t(std::floor(y)), seed, distribution);
    const double gx = (x - .5) / size, gy = (y - .5) / size;
    const auto ix = std::int64_t(std::floor(gx)), iy = std::int64_t(std::floor(gy));
    const double tx = smooth(gx - double(ix)), ty = smooth(gy - double(iy));
    const double upper = std::lerp(cell_noise(ix, iy, seed, distribution),
                                   cell_noise(ix + 1, iy, seed, distribution), tx);
    const double lower = std::lerp(cell_noise(ix, iy + 1, seed, distribution),
                                   cell_noise(ix + 1, iy + 1, seed, distribution), tx);
    return std::lerp(upper, lower, ty);
}

void adjust_hls(RGB& rgb, const Parameters& p, double x, double y, double detail, std::uint32_t seed) {
    // Apply the HLS delta to the original RGB so extended-range values survive.
    const RGB base{unit(rgb[0]), unit(rgb[1]), unit(rgb[2])};
    const auto [lo, hi] = std::minmax_element(base.begin(), base.end());
    const double span = *hi - *lo;
    double light = (*hi + *lo) * .5;
    double saturation = 0, hue = 0;
    if (span > 0) {
        saturation = span / (2 * std::min(light, 1 - light));
        if (hi == base.begin()) hue = (base[1] - base[2]) / span;
        else if (hi == base.begin() + 1) hue = 2 + (base[2] - base[0]) / span;
        else hue = 4 + (base[0] - base[1]) / span;
        hue /= 6;
    } else if (p.saturation > 0) {
        hue = .5 + .5 * pixel_noise(x, y, p.size, seed + 43, Uniform);
    }
    if (p.hue > 0) hue += .005 * p.hue * detail * pixel_noise(x, y, p.size, seed + 47, p.distribution);
    if (p.lightness > 0) light = unit(light + .005 * p.lightness * detail * pixel_noise(x, y, p.size, seed + 53, p.distribution));
    if (p.saturation > 0) saturation = unit(saturation + .01 * p.saturation * detail * pixel_noise(x, y, p.size, seed + 59, p.distribution));
    hue -= std::floor(hue);
    const double amplitude = saturation * std::min(light, 1 - light);
    constexpr std::array shifts{0., 8., 4.};
    for (int k = 0; k < 3; ++k) {
        double sector = hue * 12 + shifts[k];
        if (sector >= 12) sector -= 12;
        const double ramp = std::max(-1., std::min({sector - 3, 9 - sector, 1.}));
        rgb[k] += light - amplitude * ramp - base[k];
    }
}

void trail_chroma(const float* row, int width, int x, double radius, double step, RGB& rgb) {
    const double distance = radius / step;
    RGB chroma{};
    double total = 0;
    for (int tap = 0; tap < 3; ++tap) {
        const double position = std::clamp(x - distance * (.25 + .75 * tap), 0., double(width - 1));
        const int i = int(position), j = std::min(width - 1, i + 1);
        const double fraction = position - i;
        const double kernel = tap == 1 ? .5 : .25;
        for (int neighbor = 0; neighbor < 2; ++neighbor) {
            const float* p = row + std::size_t(neighbor ? j : i) * 4;
            const double weight = kernel * (neighbor ? fraction : 1 - fraction) * unit(clean(p[3]));
            if (weight == 0) continue;
            const RGB sample{clean(p[0]), clean(p[1]), clean(p[2])};
            const double brightness = luma(sample);
            for (int k = 0; k < 3; ++k) chroma[k] += weight * (sample[k] - brightness);
            total += weight;
        }
    }
    if (total == 0) return;
    const double brightness = luma(rgb);
    const double blend = .85 * std::min(1., distance);
    for (int k = 0; k < 3; ++k) rgb[k] = std::lerp(rgb[k], brightness + chroma[k] / total, blend);
}

struct SignalRow {
    double interference = 0, dropout_offset = 0;
    std::uint32_t dropout_seed = 0;
    bool dropout = false;
};

SignalRow signal_row(double y, const Parameters& p, double step, std::uint32_t seed) {
    SignalRow row;
    const auto line = std::int64_t(std::floor(y));
    if (p.bands > 0) {
        row.interference = p.bands * .01 * (.28 * psgrain::noise(0, (y - .5) / 6, seed + 101) +
                           .09 * cell_noise(0, line, seed + 103, Uniform)) * std::min(1., 3 / step);
    }
    if (p.dropouts > 0) {
        const auto block = std::int64_t(std::floor(y / 5));
        row.dropout_seed = coordinate_bits(0, block, seed + 107);
        row.dropout = line - block * 5 == row.dropout_seed % 5 &&
                      psgrain::random(row.dropout_seed, 1) < p.dropouts * .009;
        row.dropout_offset = 96 * psgrain::random(row.dropout_seed, 2);
    }
    return row;
}

double signal_noise(double x, double y, double step, const Parameters& p,
                    const SignalRow& row, std::uint32_t seed) {
    double value = row.interference;
    if (p.snow > 0) value += p.snow * .0055 / step * pixel_noise(x, y, 1, seed + 109, Uniform);
    if (row.dropout) {
        const double segment = (x + row.dropout_offset) / 96;
        const auto cell = std::int64_t(std::floor(segment));
        const double phase = segment - double(cell);
        const double mark = psgrain::random(row.dropout_seed, std::uint32_t(cell) * 17 + 3);
        if (mark > .5) {
            const double window = std::min({1., phase * 10., (1 - phase) * 10.});
            value += (mark > .62 ? .48 : -.36) * window / step;
        }
    }
    return value;
}
}

Parameters preset(Preset setting) {
    Parameters p;
    switch (setting) {
    case Preset::FineNoise:
        break;
    case Preset::SoftFilm:
        p.basic = false;
        p.grain = true;
        break;
    case Preset::TVSnow:
        p.basic = false;
        p.signal = true;
        p.snow = 68;
        p.bands = 18;
        p.chroma_bleed = 0;
        p.dropouts = 0;
        break;
    case Preset::VHS:
    case Preset::FilmAndVHS:
        p.basic = false;
        p.grain = setting == Preset::FilmAndVHS;
        p.signal = true;
        p.snow = 12;
        p.bands = 27;
        p.chroma_bleed = 8;
        p.dropouts = 23;
        break;
    case Preset::CoarseFilm:
        p.basic = false;
        p.grain = true;
        p.grain_amount = 18;
        p.grain_size = 2.1;
        p.grain_softness = 12;
        p.grain_roughness = 75;
        p.grain_midpoint = 44;
        p.shadows = 65;
        break;
    }
    return p;
}

bool valid(const Parameters& p) {
    for (double percent : {p.amount, p.color, p.hue, p.lightness, p.saturation, p.grain_amount,
                           p.grain_softness, p.grain_color, p.shadows, p.midtones, p.highlights,
                           p.snow, p.bands, p.dropouts, p.grain_roughness, p.mix}) {
        if (!(percent >= 0 && percent <= 100)) return false;
    }
    return (p.distribution == Uniform || p.distribution == Gaussian) && p.size >= 1 && p.size <= 64 &&
           p.grain_size >= .25 && p.grain_size <= 40 && p.chroma_bleed >= 0 && p.chroma_bleed <= 128 &&
           p.grain_midpoint >= 1 && p.grain_midpoint <= 99 &&
           p.seed >= 0 && p.seed <= 999999 && p.phase >= 0 && p.phase <= 999999;
}

static bool render_rect(const Image& source, const Parameters& p, Region region, Image& output,
                        const Cancel& cancel, RenderSpace space) {
    if (!source.valid() || !valid(p) || (cancel && cancel())) return false;
    if (region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 ||
        region.x > source.width - region.width || region.y > source.height - region.height) return false;
    if (space.step == 0) space = {double(source.left), double(source.top), 1};
    if (!(space.step >= 1 && space.step <= 2000000 && std::abs(space.left) <= 4294967296. &&
          std::abs(space.top) <= 4294967296.)) return false;

    const bool pixels = p.basic && p.amount > 0;
    const bool hls = p.basic && (p.hue > 0 || p.lightness > 0 || p.saturation > 0);
    const bool grain = p.grain && p.grain_amount > 0 && (p.shadows > 0 || p.midtones > 0 || p.highlights > 0);
    const bool signal = p.signal && (p.snow > 0 || p.bands > 0 || p.dropouts > 0);
    const bool trail = p.signal && p.chroma_bleed > 0;
    const bool bypass = p.mix == 0 || !(pixels || hls || grain || signal || trail);
    const bool whole = region.width == source.width && region.height == source.height;
    if (bypass && whole) {
        output = source;
        return true;
    }

    Image result;
    if (whole) result = source;
    else {
        const auto left = std::int64_t(source.left) + region.x;
        const auto top = std::int64_t(source.top) + region.y;
        if (left > std::numeric_limits<int>::max() || top > std::numeric_limits<int>::max()) return false;
        result = {region.width, region.height, int(left), int(top),
                  std::vector<float>(std::size_t(region.width) * region.height * 4)};
    }
    pscpu::ParallelScope parallel;
    const auto seed = psgrain::hash(std::uint32_t(p.seed) ^ psgrain::hash(std::uint32_t(p.phase) + 0xf1357ae9u));
    const double pixel_detail = std::min(1., p.size / space.step);
    const double grain_detail = std::min(1., p.grain_size / space.step);
    const double pixel_strength = p.amount * .005 * pixel_detail;
    const double grain_strength = p.grain_amount * .02 * grain_detail;
    const double pixel_color = p.color * .01, grain_color = p.grain_color * .01;
    const double softness = p.grain_softness * .01;
    const double midpoint = p.grain_midpoint / 100.;
    const double mix = p.mix / 100.;
    constexpr double finer_scale = 2.3;
    const double finer_gain = .0065 * p.grain_roughness *
        std::min(1., p.grain_size / (finer_scale * space.step)) / grain_detail;
    const double grain_gain = 1. / std::sqrt(1. + finer_gain * finer_gain);
    auto sample_grain = [&](double x, double y, std::uint32_t grain_seed) {
        const double primary = psgrain::grain_noise(x, y, grain_seed, softness);
        if (finer_gain == 0) return primary;
        const double fine = psgrain::grain_noise(x * finer_scale, y * finer_scale, grain_seed ^ 0xb7e15162u, softness);
        return (primary + finer_gain * fine) * grain_gain;
    };
    constexpr double max_float = std::numeric_limits<float>::max();

    const auto write_rows = [&](std::size_t first, std::size_t end, const auto& cancelled) {
        for (auto y = first; y < end; ++y) {
            if (cancelled()) return false;
            const auto source_y = y + region.y;
            const double wy = space.top + (double(source_y) + .5) * space.step;
            const auto* row = source.rgba.data() + source_y * source.width * 4;
            auto* dest = result.rgba.data() + y * region.width * 4;
            if (!whole) std::copy_n(row + std::size_t(region.x) * 4, std::size_t(region.width) * 4, dest);
            if (bypass) continue;
            const auto line = signal ? signal_row(wy, p, space.step, seed) : SignalRow{};
            for (int col = 0; col < region.width; ++col) {
                const int x = region.x + col;
                const auto i = std::size_t(x) * 4;
                auto* result_pixel = dest + std::size_t(col) * 4;
                if (row[i + 3] <= 0) continue;
                const double wx = space.left + (x + .5) * space.step;
                RGB rgb{clean(row[i]), clean(row[i + 1]), clean(row[i + 2])};
                if (trail) trail_chroma(row, source.width, x, p.chroma_bleed, space.step, rgb);
                if (hls) adjust_hls(rgb, p, wx, wy, pixel_detail, seed);
                const double signal_value = signal ? signal_noise(wx, wy, space.step, p, line, seed) : 0;
                const double pixel_value = pixels ? pixel_noise(wx, wy, p.size, seed + 11, p.distribution) : 0;
                const double tone = grain ? unit(luma(rgb)) : midpoint;
                const double grain_weight = tone < midpoint
                    ? std::lerp(p.shadows, p.midtones, smooth(tone / midpoint))
                    : std::lerp(p.midtones, p.highlights, smooth((tone - midpoint) / (1 - midpoint)));
                const double gx = wx / p.grain_size, gy = wy / p.grain_size;
                const double grain_value = grain ? sample_grain(gx, gy, seed + 71) : 0;
                for (int k = 0; k < 3; ++k) {
                    double value = rgb[k] + signal_value;
                    if (pixels) {
                        const double channel = pixel_color > 0
                            ? std::lerp(pixel_value, pixel_noise(wx, wy, p.size, seed + 23 + k * 3, p.distribution), pixel_color)
                            : pixel_value;
                        value += channel * pixel_strength;
                    }
                    if (grain) {
                        const double channel = grain_color > 0
                            ? std::lerp(grain_value, sample_grain(gx, gy, seed + 79 + k * 5), grain_color)
                            : grain_value;
                        value += channel * grain_strength * grain_weight * .01;
                    }
                    if (mix != 1) value = std::lerp(clean(row[i + k]), value, mix);
                    result_pixel[k] = float(std::clamp(value, -max_float, max_float));
                }
            }
        }
        return true;
    };
    if (!pscpu::parallel_ranges(region.height, result.rgba.size(), cancel, write_rows) || (cancel && cancel())) return false;
    output = std::move(result);
    return true;
}

bool render(const Image& source, const Parameters& p, Image& output, const Cancel& cancel, RenderSpace space) {
    return render_rect(source, p, {0, 0, source.width, source.height}, output, cancel, space);
}

bool render_region(const Image& source, const Parameters& p, Region region, Image& output, const Cancel& cancel) {
    return render_rect(source, p, region, output, cancel, {});
}
}
