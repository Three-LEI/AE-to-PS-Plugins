#include "pixelsort_core.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace pixelsort;

void require(bool ok, const char* reason) {
    if (!ok) throw std::runtime_error(reason);
}

void sort_scanlines() {
    constexpr int width = 11, height = 13;
    Image source{width, height, -4, 5, std::vector<float>(width * height * 4)}, result, crop;
    constexpr float values[] = {.75f, .25f, .5f, 1.5f, .8f, .4f, .6f, .21f, .9f, .45f, .2f};
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        auto* pixel = source.at(source.left + x, source.top + y);
        std::fill_n(pixel, 3, values[(x + y) % width]);
        pixel[3] = x == 8 ? 0.f : .6f;
    }
    Parameters p;
    p.low = 20;
    p.high = 90;
    p.length = 3;
    require(render(source, p, result), "horizontal pixel sort");
    constexpr float sorted[] = {.25f, .5f, .75f, 1.5f, .4f, .6f, .8f, .21f, .9f, .2f, .45f};
    for (int x = 0; x < width; ++x)
        require(result.at(source.left + x, source.top)[0] == sorted[x], "bounded runs sort separately");
    for (std::size_t i = 3; i < result.rgba.size(); i += 4)
        require(result.rgba[i] == source.rgba[i], "layer transparency is preserved");

    const Rect area{-2, 7, 6, 15};
    for (int axis : {Horizontal, Vertical}) {
        p.direction = axis;
        p.order = LightToDark;
        require(render(source, p, result), "descending scanlines");
        require(result.rgba != source.rgba, "direction affects image content");
        require(render_region(source, p, area, {-4, 5, 1}, crop), "crop native scanlines");
        for (int y = area.top; y < area.bottom; ++y) for (int x = area.left; x < area.right; ++x) {
            const auto* pixel = result.at(x, y);
            require(std::equal(pixel, pixel + 4, crop.at(x, y)), "native crop does not split sorted runs");
        }
    }
    p.length = 1;
    require(render(source, p, result) && result.rgba == source.rgba, "one-pixel stripe is exact");
    p.length = 24;
    p.mix = 0;
    require(render(source, p, result) && result.rgba == source.rgba, "zero mix is exact");
    require(!render(source, p, result, [] { return true; }) && result.rgba == source.rgba, "cancel preserves prior output");
}
} // namespace

int main() {
    try {
        sort_scanlines();
        std::cout << "PASS pixel sort: threshold, length cap, order, alpha, HDR and exact native crops\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
