#include "compoundblur_core.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace compoundblur;

void require(bool ok, const char* reason) {
    if (!ok) throw std::runtime_error(reason);
}

void ramp_editing() {
    namespace grad = pscpu::gradient;
    grad::Ramp ramp;
    require(grad::valid(ramp) && grad::sample(ramp, .25)[0] == .25, "valid factory ramp");
    require(grad::insert(ramp, {.3, {.4, .5, .6, .7}, .24}) == 1, "add an RGBA stop");
    require(grad::insert(ramp, {.6, {.8, .6, .4, .8}}) == 2, "add a second stop");
    require(grad::move(ramp, 1, .74) == 2 && grad::valid(ramp), "a stop can cross another stop");
    ramp.interpolation = grad::Smooth;
    const auto saved = grad::encode(ramp);
    grad::Ramp restored;
    require(grad::decode(saved, restored) && grad::encode(restored) == saved, "gradient serialization is lossless");
    auto bad = saved;
    bad[2] = 25;
    require(!grad::decode(bad, restored) && grad::encode(restored) == saved, "invalid gradient leaves prior settings intact");
    grad::reverse(restored);
    for (double t : {.13, .42, .69, .91}) {
        const auto expected = grad::sample(ramp, 1 - t);
        const auto result = grad::sample(restored, t);
        for (int c = 0; c < 4; ++c) require(std::abs(expected[c] - result[c]) < 1e-12, "reverse mirrors colors and midpoints");
    }
    require(grad::remove(restored, 1) && grad::remove(restored, 1) && !grad::remove(restored, 1), "at least two stops remain");

    grad::Placement p;
    const auto coordinate = [&](grad::Shape shape, double x, double y) {
        p.shape = int(shape);
        return grad::Field(p, -18, 7, 100, 100).at(x, y);
    };
    require(coordinate(grad::Shape::Linear, 57, 57) == .75, "linear shape");
    require(coordinate(grad::Shape::Radial, 47, 77) == .5, "radial shape");
    require(coordinate(grad::Shape::Reflected, 7, 57) == .5, "reflected shape");
    require(coordinate(grad::Shape::Angular, 32, 82) == .25, "angular shape");
    require(coordinate(grad::Shape::Diamond, 47, 77) == .7, "diamond shape");
}

void variable_blur() {
    constexpr int width = 173, height = 125;
    Image source{width, height, -38, 16, std::vector<float>(width * height * 4)}, result, crop;
    for (int y = source.top; y < source.top + height; ++y) for (int x = source.left; x < source.left + width; ++x) {
        auto* pixel = source.at(x, y);
        pixel[0] = (x & 1) ? 1.4f : .1f;
        pixel[1] = .45f;
        pixel[2] = .6f;
        pixel[3] = (x + y) % 11 ? .8f : .3f;
    }
    auto* hidden = source.at(88, 55);
    hidden[0] = 50;
    hidden[3] = 0;
    Parameters p;
    p.gradient.stops[0].position = .27;
    p.gradient.stops[1].position = .7;
    require(render(source, p, nullptr, source.bounds(), result), "gradient-driven blur");
    require(std::equal(source.at(-20, 55), source.at(-20, 55) + 4, result.at(-20, 55)), "black is unblurred");
    require(result.at(87, 55)[0] < 1.01f && result.at(87, 55)[0] > .5f, "white strongly blurs and transparent RGB is ignored");
    for (std::size_t i = 3; i < source.rgba.size(); i += 4)
        require(source.rgba[i] == result.rgba[i], "blur preserves alpha");
    const Rect area{-3, 30, 97, 107};
    require(render_region(source, p, nullptr, source.bounds(), area, {-38, 16, 1}, crop), "render native region");
    for (int y = area.top; y < area.bottom; ++y) for (int x = area.left; x < area.right; ++x) {
        const auto* expected = result.at(x, y);
        require(std::equal(expected, expected + 4, crop.at(x, y)), "native preview matches final output");
    }
    p.source = Reference;
    p.layer_key = 1;
    p.channel = Alpha;
    Image reference{57, height, 51, 16, std::vector<float>(57 * height * 4)};
    for (std::size_t i = 3; i < reference.rgba.size(); i += 4) reference.rgba[i] = 1;
    require(!render(source, p, nullptr, source.bounds(), crop), "missing external input is reported");
    require(render(source, p, &reference, source.bounds(), result), "alpha of an offset reference controls blur");
    require(result.at(81, 60)[0] != source.at(81, 60)[0], "reference coordinates hit the blurred region");
    require(result.at(41, 60)[0] == source.at(41, 60)[0], "outside reference is black");
    p.channel = Luminance;
    require(render(source, p, &reference, source.bounds(), result) && result.rgba == source.rgba, "luminance of black has no effect");
    p.invert = true;
    require(render(source, p, &reference, source.bounds(), result) && result.rgba != source.rgba, "reverse black and white");
    p.radius = 0;
    require(render(source, p, &reference, source.bounds(), result) && result.rgba == source.rgba, "zero radius is exact");
    require(!render(source, p, &reference, source.bounds(), result, [] { return true; }) && result.rgba == source.rgba,
        "cancel keeps prior output");
}

void smooth_reconstruction() {
    constexpr int width = 379, height = 353, shift_x = 13, shift_y = 9;
    const auto target = [](int dx, int dy) {
        Image image{width, height, -19, 12, std::vector<float>(width * height * 4)};
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            const int u = x - dx, v = y - dy;
            const bool bright = (u > 132 && u < 231 && v > 108 && v < 200) ||
                                ((u - 234) * (u - 234) + (v - 205) * (v - 205) < 39 * 39);
            auto* pixel = image.at(x + image.left, y + image.top);
            std::fill_n(pixel, 3, bright ? .91f : .08f);
            pixel[3] = 1;
        }
        return image;
    };
    const auto reference = [](int dx, bool ramp) {
        Image image{width, height, -19, 12, std::vector<float>(width * height * 4)};
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            auto* pixel = image.at(x + image.left, y + image.top);
            std::fill_n(pixel, 3, ramp ? std::clamp((x - dx) / 310.f, 0.f, 1.f) : 1.f);
            pixel[3] = 1;
        }
        return image;
    };
    const auto src = target(0, 0), shifted = target(shift_x, shift_y);
    Parameters p;
    p.source = Reference;
    p.layer_key = 1;
    p.radius = 31.85;
    for (bool ramp : {false, true}) {
        const auto map = reference(0, ramp), shifted_map = reference(shift_x, ramp);
        Image result, moved;
        require(render(src, p, &map, src.bounds(), result) && render(shifted, p, &shifted_map, shifted.bounds(), moved),
                "render high-contrast detail with uniform and gradual reference blur");
        float max_shift_error = 0, max_curvature = 0;
        for (int y = 105; y < 282; ++y) for (int x = 102; x < 284; ++x) {
            const float center = result.at(x, y)[0];
            max_shift_error = std::max(max_shift_error, std::abs(center - moved.at(x + shift_x, y + shift_y)[0]));
            max_curvature = std::max(max_curvature, std::abs(result.at(x - 1, y)[0] + result.at(x + 1, y)[0] - 2 * center));
        }
        std::cout << (ramp ? "Ramp" : "Uniform") << " radius 31.85, maximum shift error " << max_shift_error
                  << ", curvature " << max_curvature << '\n';
        require(max_shift_error < .012f, "moving content by a few pixels does not reveal the scale grid");
        require(max_curvature < .003f, "smooth defocus has no grid-aligned creases");
    }
}

} // namespace

int main() {
    try {
        ramp_editing();
        variable_blur();
        smooth_reconstruction();
        std::cout << "PASS compound blur: five shapes, anchored reference, smooth reconstruction, preserved alpha and exact native crops\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
