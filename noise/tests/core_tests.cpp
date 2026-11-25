#include "noise_core.hpp"
#include "../../native/common/preview_cancel.hpp"
#include "psui/area_sample.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using noise::Image;
using noise::Parameters;

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

Image flat(float r, float g, float b, int width = 256, int height = 128) {
    Image image{width, height, -187, -31, std::vector<float>(std::size_t(width) * height * 4)};
    for (std::size_t i = 0; i < image.rgba.size(); i += 4) {
        image.rgba[i] = r;
        image.rgba[i + 1] = g;
        image.rgba[i + 2] = b;
        image.rgba[i + 3] = 1;
    }
    return image;
}

Image apply(const Image& image, const Parameters& p) {
    Image output;
    require(noise::render(image, p, output), "render");
    return output;
}

Parameters inactive() {
    Parameters p;
    p.basic = false;
    return p;
}

double y(const float* p) { return .299 * p[0] + .587 * p[1] + .114 * p[2]; }

void pixel_cases() {
    const auto input = flat(.5f, .5f, .5f);
    Parameters p;
    p.amount = 40;
    p.distribution = noise::Uniform;
    const auto uniform = apply(input, p);
    p.distribution = noise::Gaussian;
    const auto gaussian = apply(input, p);
    int tails = 0;
    double sum = 0, squares = 0;
    for (std::size_t i = 0; i < input.rgba.size(); i += 4) {
        require(std::abs(uniform.rgba[i] - .5) <= .2000001, "uniform is bounded");
        const double delta = gaussian.rgba[i] - .5;
        tails += std::abs(delta) > .2000001;
        sum += delta;
        squares += delta * delta;
        require(gaussian.rgba[i] == gaussian.rgba[i + 1] && gaussian.rgba[i] == gaussian.rgba[i + 2],
                "monochromatic noise has one sample per pixel");
    }
    const auto n = input.rgba.size() / 4;
    require(tails > 1700 && tails < 3500 && std::abs(sum / n) < .003 &&
            std::abs(squares / n - .04 / 3) < .001, "Gaussian distribution and comparable RMS");
    p.color = 100;
    const auto colored = apply(input, p);
    require(colored.rgba[0] != colored.rgba[1], "independent RGB noise");
    p.color = 0;
    p.size = 8;
    const auto coarse = apply(input, p);
    double fine_difference = 0, coarse_difference = 0;
    for (int pixel = 0; pixel < input.width - 1; ++pixel) {
        fine_difference += std::abs(gaussian.rgba[pixel * 4] - gaussian.rgba[pixel * 4 + 4]);
        coarse_difference += std::abs(coarse.rgba[pixel * 4] - coarse.rgba[pixel * 4 + 4]);
    }
    require(coarse_difference < .3 * fine_difference, "size changes spatial correlation");
    p.phase = 240;
    require(apply(input, p).rgba != coarse.rgba, "phase changes the pattern");
    ++p.seed;
    const auto seeded = apply(input, p);
    Image serial;
    require(noise::render(input, p, serial, pscpu::SerialPreviewCancel{}), "serial preview");
    require(seeded.rgba == serial.rgba &&
            noise::render(input, p, serial, {}, {double(input.left), double(input.top), 1}) &&
            seeded.rgba == serial.rgba, "full preview is identical at 100 percent");
}

void hls_cases() {
    Parameters p;
    p.amount = 0;
    p.hue = 80;
    const auto input = flat(.8f, .4f, .2f);
    auto output = apply(input, p);
    require(output.rgba != input.rgba, "hue noise changes chromatic input");
    for (std::size_t i = 0; i < output.rgba.size(); i += 4) {
        const auto [lo, hi] = std::minmax_element(output.rgba.data() + i, output.rgba.data() + i + 3);
        require(std::abs(*lo - .2) < .000001 && std::abs(*hi - .8) < .000001, "hue preserves lightness and saturation");
    }
    const auto gray = flat(.5f, .5f, .5f);
    require(apply(gray, p).rgba == gray.rgba, "hue alone preserves gray");
    p.hue = 0;
    p.saturation = 40;
    output = apply(gray, p);
    require(output.rgba != gray.rgba, "saturation noise colorizes gray");
    p.saturation = 0;
    p.lightness = 50;
    output = apply(flat(3, 3, 3), p);
    require(output.rgba[0] > 2, "HLS retains HDR headroom");
}

void signal_cases() {
    auto input = flat(.15f, .4f, .8f);
    for (int row = 0; row < input.height; ++row) {
        for (int x = 0; x < input.width / 2; ++x) {
            float* px = &input.rgba[(std::size_t(row) * input.width + x) * 4];
            px[0] = .9f;
            px[2] = .15f;
            px[3] = x < 20 ? 0 : .5f;
        }
    }
    auto p = inactive();
    p.signal = true;
    p.snow = p.bands = p.dropouts = 0;
    p.chroma_bleed = 16;
    const auto trailing = apply(input, p);
    require(trailing.rgba[128 * 4] > .3f, "color trails to the right of the edge");
    require(trailing.rgba[180 * 4] == input.rgba[180 * 4], "chroma trail stays local");
    for (std::size_t i = 0; i < input.rgba.size(); i += 4) {
        require(trailing.rgba[i + 3] == input.rgba[i + 3], "signal preserves partial alpha");
        require(std::abs(y(input.rgba.data() + i) - y(trailing.rgba.data() + i)) < 1e-7, "chroma trail preserves luma");
    }
    auto hidden = input;
    for (std::size_t i = 0; i < hidden.rgba.size(); i += 4) {
        if (hidden.rgba[i + 3] == 0) hidden.rgba[i + 1] = 10;
    }
    const auto hidden_result = apply(hidden, p);
    for (std::size_t i = 0; i < hidden.rgba.size(); i += 4) {
        if (hidden.rgba[i + 3] > 0) require(hidden_result.rgba[i + 1] == trailing.rgba[i + 1], "hidden RGB cannot bleed");
    }

    const auto gray = flat(.5f, .5f, .5f);
    p.chroma_bleed = 0;
    p.bands = 100;
    const auto bands = apply(gray, p);
    require(bands.rgba[0] == bands.rgba[100 * 4] && bands.rgba[0] != bands.rgba[gray.width * 4],
            "interference follows horizontal rows");
    p.bands = 0;
    p.dropouts = 100;
    const auto dropouts = apply(gray, p);
    int affected = 0;
    for (std::size_t i = 0; i < gray.rgba.size(); i += 4) affected += dropouts.rgba[i] != gray.rgba[i];
    require(affected > 0 && affected < gray.width * gray.height / 5, "dropouts are sparse");
}

void stack_cases() {
    auto input = flat(.5f, .5f, .5f);
    for (std::size_t i = 0; i < input.rgba.size(); i += 4) input.rgba[i + 3] = i % 7 == 0 ? .27f : 1.f;
    auto p = inactive();
    require(apply(input, p).rgba == input.rgba, "switches off is bit-exact");
    p.basic = p.grain = p.signal = true;
    p.snow = p.bands = p.dropouts = p.chroma_bleed = p.amount = p.grain_amount = 0;
    require(apply(input, p).rgba == input.rgba, "strengths zero is bit-exact");
    p.amount = 11;
    p.grain_amount = 13;
    p.snow = 16;
    const auto stacked = apply(input, p);
    auto basic = p, grain = p, signal = p;
    basic.grain = basic.signal = grain.basic = grain.signal = signal.basic = signal.grain = false;
    const auto a = apply(input, basic), b = apply(input, grain), c = apply(input, signal);
    for (std::size_t i = 0; i < input.rgba.size(); i += 4) {
        require(stacked.rgba[i + 3] == input.rgba[i + 3], "stack preserves alpha");
        require(std::abs(double(stacked.rgba[i]) + 1 - a.rgba[i] - b.rgba[i] - c.rgba[i]) < 2e-7,
                "three effects stack");
    }
    p.shadows = p.highlights = 0;
    p.basic = p.signal = false;
    const auto middle = apply(input, p);
    p.midtones = 0;
    require(middle.rgba != input.rgba && apply(input, p).rgba == input.rgba, "midtone weighting");
    auto invalid = p;
    invalid.size = std::numeric_limits<double>::quiet_NaN();
    auto output = input;
    require(!noise::render(input, invalid, output) && output.rgba == input.rgba, "invalid parameters leave output untouched");
    require(!noise::render(input, p, output, [] { return true; }) && output.rgba == input.rgba, "cancel is atomic");
}

void refinement_cases() {
    const auto input = flat(.25f, .25f, .25f);
    auto p = inactive();
    p.grain = true;
    p.shadows = p.highlights = 0;
    const auto default_peak = apply(input, p);
    p.grain_midpoint = 25;
    const auto shifted_peak = apply(input, p);
    for (std::size_t i = 0; i < input.rgba.size(); i += 4) {
        require(std::abs((shifted_peak.rgba[i] - .25) - 2 * (default_peak.rgba[i] - .25)) < 1e-7,
                "moving midpoint to the input tone doubles the original weight");
    }
    p.grain_roughness = 80;
    const auto textured = apply(input, p);
    require(textured.rgba != shifted_peak.rgba, "roughness changes the grain shape");
    require(apply(input, p).rgba == textured.rgba, "roughness is deterministic");
    p.basic = p.signal = true;
    const auto combined = apply(input, p);
    p.mix = 40;
    const auto blended = apply(input, p);
    for (std::size_t i = 0; i < input.rgba.size(); ++i) {
        const auto wanted = std::lerp(double(input.rgba[i]), double(combined.rgba[i]), .4);
        require(std::abs(blended.rgba[i] - wanted) < 1e-7, "mix scales all three effects");
    }
    p.mix = 0;
    require(apply(input, p).rgba == input.rgba, "zero mix bypasses the three groups");
    p.grain_midpoint = 100;
    require(!noise::valid(p), "midpoint must leave a range on both sides");
}

void area_preview_cases() {
    auto stripe = flat(0, 0, 0, 4, 4);
    for (int x = 0; x < 4; ++x) {
        auto* pixel = stripe.rgba.data() + (4 + x) * 4;
        pixel[0] = pixel[1] = pixel[2] = 1;
    }
    require(stripe.rgba[0] == 0 && psui::sample_area(stripe, 0, 0, 4, 4)[0] == .25f,
            "a one-pixel line contributes its covered area at 25 percent zoom");
    stripe.rgba[3] = 0;
    stripe.rgba[0] = 10;
    require(psui::sample_area(stripe, 0, 0, 4, 4)[0] == float(4. / 15.),
            "a transparent neighbor contributes no RGB");
    const auto input = flat(.5f, .5f, .5f, 512, 256);
    const auto noisy = apply(input, Parameters{});
    double point_energy = 0, area_energy = 0;
    for (int y = 0; y < input.height; y += 4) {
        for (int x = 0; x < input.width; x += 4) {
            const double point = noisy.rgba[(std::size_t(y) * input.width + x) * 4] - .5;
            const double area = psui::sample_area(noisy, x, y, x + 4, y + 4)[0] - .5;
            point_energy += point * point;
            area_energy += area * area;
        }
    }
    const double ratio = std::sqrt(area_energy / point_energy);
    require(ratio > .2 && ratio < .3, "4x4 area preview matches the expected RMS for fine noise");
    std::cout << "25% zoom area/point noise RMS ratio: " << ratio << '\n';
}

void region_cases() {
    auto input = flat(.17f, .4f, .8f);
    for (std::size_t i = 0; i < input.rgba.size(); i += 4) {
        input.rgba[i] = float(i % 383) / 383;
        input.rgba[i + 3] = i % 13 == 0 ? 0.f : .55f;
    }
    auto p = noise::preset(noise::Preset::FilmAndVHS);
    p.basic = true;
    p.hue = 12;
    p.grain_roughness = 60;
    p.mix = 72;
    p.chroma_bleed = 127.5;
    const auto whole = apply(input, p);
    for (noise::Region cut : {noise::Region{0, 0, 31, 37}, noise::Region{99, 37, 107, 68},
                               noise::Region{input.width - 19, input.height - 37, 19, 37}}) {
        Image region;
        require(noise::render_region(input, p, cut, region), "native viewport render");
        require(region.valid() && region.left == input.left + cut.x && region.top == input.top + cut.y,
                "viewport retains the document origin");
        for (int row = 0; row < cut.height; ++row) {
            const auto expected = whole.rgba.data() + (std::size_t(row + cut.y) * input.width + cut.x) * 4;
            require(std::equal(expected, expected + std::size_t(cut.width) * 4,
                             region.rgba.data() + std::size_t(row) * cut.width * 4),
                    "viewport equals the full render, including long chroma trails and alpha");
        }
    }
    auto zero = p;
    zero.mix = 0;
    Image region;
    require(noise::render_region(input, zero, {3, 2, 1, 1}, region), "zero-mix viewport");
    const auto expected = input.rgba.data() + (std::size_t(2) * input.width + 3) * 4;
    require(std::equal(expected, expected + 4, region.rgba.begin()), "zero-mix viewport copies original pixels");
    require(!noise::render_region(input, p, {1, 2, input.width, 1}, region), "region must fit the source");
}
}

int main() {
    pixel_cases();
    hls_cases();
    signal_cases();
    stack_cases();
    refinement_cases();
    area_preview_cases();
    region_cases();
    std::cout << "PASS: distribution, HLS/HDR, grain and VHS, mix, area preview, native viewport and cancel\n";
}
