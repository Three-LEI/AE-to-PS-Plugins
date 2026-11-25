#include "weather_core.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace weather;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void check_mode(Image& source, int mode) {
    Parameters p;
    p.mode = mode;
    p.output = Particles;
    Image particles, again, crop, composite;
    require(render(source, p, particles), "render particles without using source colors");
    int occupied = 0;
    for (std::size_t i = 3; i < particles.rgba.size(); i += 4) {
        require(particles.rgba[i] >= 0 && particles.rgba[i] <= 1, "bounded coverage");
        occupied += particles.rgba[i] > 0;
    }
    require(occupied > 100 && occupied < int(particles.rgba.size() / 4), "particles over transparent space");
    require(render(source, p, again) && again.rgba == particles.rgba, "fixed seed is repeatable");
    Parameters colored = p;
    constexpr double tint[] = {.2, .55, .35};
    auto* inactive_color = mode == Rain ? colored.snow_color : colored.rain_color;
    std::copy_n(tint, 3, inactive_color);
    require(render(source, colored, again) && again.rgba == particles.rgba, "the other mode keeps its own color");
    auto* active_color = mode == Rain ? colored.rain_color : colored.snow_color;
    std::copy_n(tint, 3, active_color);
    require(render(source, colored, again) && again.rgba != particles.rgba, "the active mode uses its custom color");
    for (std::size_t i = 0; i < particles.rgba.size(); i += 4) {
        const auto alpha = particles.rgba[i + 3];
        require(again.rgba[i + 3] == alpha, "changing color preserves particle coverage");
        if (alpha <= 0) continue;
        for (int channel = 0; channel < 3; ++channel)
            require(again.rgba[i + channel] == float(tint[channel]), "particle RGB matches the selected tint");
    }
    const Rect region{source.left + 18, source.top + 29, source.left + 161, source.top + 96};
    require(render_region(source, p, region, {double(source.left), double(source.top), 1}, crop),
            "render a native viewport");
    for (int y = region.top; y < region.bottom; ++y) for (int x = region.left; x < region.right; ++x) {
        const auto* pixel = crop.at(x, y);
        require(std::equal(pixel, pixel + 4, particles.at(x, y)), "cropped preview matches full pixels");
    }
    --p.seed;
    require(render(source, p, again) && again.rgba != particles.rgba, "seed changes the pattern");
    ++p.seed;
    p.near_blur = 0;
    require(render(source, p, again) && again.rgba != particles.rgba, "near blur affects particles");
    p.near_blur = Parameters{}.near_blur;
    p.output = Composite;
    require(render(source, p, composite), "composite pixels");
    for (std::size_t i = 0; i < source.rgba.size(); i += 4) {
        const float a = particles.rgba[i + 3], source_weight = source.rgba[i + 3] * (1 - a);
        require(std::abs(composite.rgba[i + 3] - (a + source_weight)) < 1e-6, "source-over alpha");
        if (a <= 0) continue;
        const float color = (particles.rgba[i] * a + source.rgba[i] * source_weight) / composite.rgba[i + 3];
        require(composite.rgba[i] == color, "source-over straight HDR color");
    }
    p.density = 0;
    require(render(source, p, again) && source.rgba == again.rgba, "zero density preserves source");
    p.output = Particles;
    require(render(source, p, again) && std::all_of(again.rgba.begin(), again.rgba.end(),
            [](float value) { return value == 0; }), "zero density produces transparency");
    require(!render(source, p, again, [] { return true; }) && again.valid(), "cancel preserves last output");
}
} // namespace

int main() {
    try {
        Image source{293, 277, -79, -56, std::vector<float>(293 * 277 * 4)};
        for (std::size_t i = 0; i < source.rgba.size(); i += 4) {
            source.rgba[i] = 1.5f;
            source.rgba[i + 3] = (i / 4) % 7 == 0 ? 0.f : .6f;
        }
        check_mode(source, Rain);
        check_mode(source, Snow);
        std::cout << "PASS rain and snow: independent colors, deterministic alpha, depth blur, HDR compositing and exact native crops\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
