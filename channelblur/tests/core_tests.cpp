#include "channelblur_core.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace channelblur;

void require(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

Image image(int width, int height, int left = 0, int top = 0) {
    return {width, height, left, top, std::vector<float>(std::size_t(width) * height * 4)};
}

double luminance(const float* p) {
    return .2126 * p[0] + .7152 * p[1] + .0722 * p[2];
}

void channels() {
    Image source = image(257, 264, -43, -67), result, crop;
    for (int y = source.top; y < source.top + source.height; ++y)
        for (int x = source.left; x < source.left + source.width; ++x) {
            auto* p = source.at(x, y);
            p[0] = x < 42 ? -.375f : 1.5f;
            p[1] = x % 7 ? 1.f : .5f;
            p[2] = y % 3 ? .25f : 1.f;
            p[3] = 1;
        }
    Parameters p;
    require(render(source, p, result) && source.rgba == result.rgba, "zero RGB/alpha is exact");
    p.red = .25;
    p.axis = Horizontal;
    require(render(source, p, result), "subpixel horizontal blur");
    require(result.at(41, 20)[0] > -.375f && result.at(42, 20)[0] < 1.5f, "subpixel radius affects both sides");
    for (std::size_t i = 0; i < source.rgba.size(); i += 4)
        require(std::equal(source.rgba.data() + i + 1, source.rgba.data() + i + 4, result.rgba.data() + i + 1),
                "other channels and alpha stay exact");
    p.axis = Vertical;
    require(render(source, p, result) && result.rgba == source.rgba, "vertical blur leaves a vertical step sharp");
    p.axis = Both;
    p.red = 33.75;
    p.green = 2.4;
    p.blue = 5.5;
    p.mix = 73;
    const Rect bounds{-19, -18, 113, 122};
    const SampleGrid grid{double(source.left), double(source.top), 1};
    require(render(source, p, result) && render_region(source, p, bounds, grid, crop), "large halo render and native region");
    for (int y = bounds.top; y < bounds.bottom; ++y) for (int x = bounds.left; x < bounds.right; ++x) {
        const auto* got = crop.at(x, y);
        require(std::equal(got, got + 4, result.at(x, y)), "crop sees the same neighbours as full render");
    }
    const auto last = crop.rgba;
    require(!render(source, p, crop, [] { return true; }) && crop.rgba == last, "cancel keeps the last frame");
}

void alpha_and_chroma() {
    Image source = image(65, 33), result;
    for (int y = 0; y < source.height; ++y) for (int x = 0; x < source.width; ++x) {
        auto* rgba = source.at(x, y);
        rgba[0] = x <= 32 ? 1.5f : 0.f;
        rgba[2] = x <= 32 ? 0.f : 1.5f;
        rgba[3] = x <= 32 ? .5f : 0.f;
    }
    Parameters p;
    p.alpha = 5;
    require(render(source, p, result), "soften alpha with transparent neighbours");
    require(result.at(32, 16)[3] < .5f && result.at(33, 16)[3] > 0, "soft alpha spreads across the edge");
    require(std::abs(result.at(33, 16)[0] - 1.5f) < 3e-7f && result.at(33, 16)[2] == 0,
            "expanded alpha extends visible HDR colors only");
    for (std::size_t i = 3; i < source.rgba.size(); i += 4) source.rgba[i] = 1;
    p.mode = LuminanceChroma;
    p.alpha = 0;
    require(render(source, p, result), "blur chroma independently");
    require(result.at(32, 16)[2] > .1f && result.at(33, 16)[0] > .1f, "chroma spreads across the edge");
    for (int x = 0; x < source.width; ++x)
        require(std::abs(luminance(result.at(x, 16)) - luminance(source.at(x, 16))) < 1e-7,
                "unclipped luma stays sharp");
    p.mix = 0;
    require(render(source, p, result) && result.rgba == source.rgba, "zero mix is exact");
}

void feathered_color_edge() {
    Image source = image(196, 154, -17, -9), result, crop;
    constexpr int edge = 18, center_y = 64;
    for (int y = 13; y < 118; ++y) for (int x = edge; x < 146; ++x) {
        auto* pixel = source.at(x, y);
        pixel[0] = x < edge + 4 ? 1.5f : .22f;
        pixel[1] = .4f;
        pixel[2] = (x % 2) * .3f;
        pixel[3] = 1.f;
    }
    // A narrow fringe over a larger body used to retain its sharp crop line.
    Parameters p;
    p.alpha = 13;
    require(render(source, p, result), "feather textured edges");
    const auto* outside = result.at(edge - 1, center_y);
    const auto* inside = result.at(edge, center_y);
    require(outside[3] > 0 && inside[3] < 1 && inside[3] > outside[3], "alpha tapers through the silhouette");
    require(std::abs(inside[0] * inside[3] - outside[0] * outside[3]) < .05f,
            "the feather removes the hard color seam at the original edge");
    const auto* original = source.at(82, center_y);
    require(std::equal(original, original + 4, result.at(82, center_y)), "the fully covered interior stays sharp");
    const Rect area{12, 45, 60, 95};
    require(render_region(source, p, area, {double(source.left), double(source.top), 1}, crop),
            "render the feather at native zoom");
    for (int y = area.top; y < area.bottom; ++y) for (int x = area.left; x < area.right; ++x) {
        const auto* pixel = result.at(x, y);
        require(std::equal(pixel, pixel + 4, crop.at(x, y)), "edge color matches in preview and native output");
    }
}
} // namespace

int main() {
    try {
        channels();
        alpha_and_chroma();
        feathered_color_edge();
        std::cout << "PASS channel blur: independent RGBA, subpixel radius, weighted HDR, chroma and exact native crops\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
