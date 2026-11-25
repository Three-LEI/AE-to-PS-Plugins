#include "toner_core.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

int checks = 0;
void require(bool value, const char* why) {
    checks++;
    if (!value) throw std::runtime_error(why);
}
bool near(double a, double b, double eps = 1e-6) { return std::abs(a - b) <= eps; }
bool near(const std::array<double, 3>& a, std::array<double, 3> b, double eps = 1e-6) {
    return near(a[0], b[0], eps) && near(a[1], b[1], eps) && near(a[2], b[2], eps);
}

int main() {
    try {
        using namespace toner;
        Parameters p = defaults();
        require(valid(p) && p.mode == tritone && p.blend == 0., "native defaults");
        require(p.highlights == std::array<double,3>{1,1,1} && p.midtones == std::array<double,3>{.5,.5,.5} &&
                p.shadows == std::array<double,3>{0,0,0}, "neutral default ramp");

        std::array<double, 3> out{};
        for (double gray : {0., .25, .5, .75, 1.}) {
            toned_color({gray, gray, gray}, p, out);
            require(near(out, {gray, gray, gray}), "default tritone is luminance identity");
        }

        p.mode = duotone;
        p.shadows = {1, 0, 0};
        p.highlights = {0, 0, 1};
        toned_color({0, 0, 0}, p, out); require(near(out, {1, 0, 0}), "duotone shadow");
        toned_color({.5, .5, .5}, p, out); require(near(out, {.5, 0, .5}), "duotone linear midpoint");
        toned_color({1, 1, 1}, p, out); require(near(out, {0, 0, 1}), "duotone highlight");

        p.mode = tritone;
        p.shadows = {1, 0, 0};
        p.midtones = {0, 1, 0};
        p.highlights = {0, 0, 1};
        toned_color({0, 0, 0}, p, out); require(near(out, {1, 0, 0}), "tritone shadow");
        toned_color({.5, .5, .5}, p, out); require(near(out, {0, 1, 0}), "tritone midpoint");
        toned_color({1, 1, 1}, p, out); require(near(out, {0, 0, 1}), "tritone highlight");
        toned_color({.25, .25, .25}, p, out); require(near(out, {.5, .5, 0}), "tritone quarter");

        p.mode = pentone;
        p.shadows = {0, 0, 0};
        p.darktones = {.25, 0, 0};
        p.midtones = {.5, 0, 0};
        p.brights = {.75, 0, 0};
        p.highlights = {1, 0, 0};
        for (double gray : {0., .25, .5, .75, 1.}) {
            toned_color({gray, gray, gray}, p, out);
            require(near(out[0], gray), "pentone stop positions");
        }

        p.mode = solid;
        p.highlights = {1, .5, 0};
        toned_color({0, 0, 0}, p, out); require(near(out, {1, .5, 0}), "solid uses highlights color");
        toned_color({1, 1, 1}, p, out); require(near(out, {1, .5, 0}), "solid ignores luminance");

        // Full render: alpha preservation, blend and HDR/negative luminance clamping.
        Image src{5, 1, -3, 7, std::vector<float>(20)};
        const float pixels[5][4] = {{0, 0, 0, .1f}, {.5f, .5f, .5f, .5f}, {1, 1, 1, .9f},
                                    {2.5f, 2.5f, 2.5f, 1.f}, {-1.f, -1.f, -1.f, .25f}};
        for (int i = 0; i < 5; ++i)
            for (int k = 0; k < 4; ++k) src.rgba[i * 4 + k] = pixels[i][k];
        p = defaults();
        p.mode = duotone;
        p.shadows = {1, 0, 0};
        p.highlights = {0, 0, 1};
        p.blend = 0.;
        Image outimg;
        require(render(src, p, outimg), "render");
        for (int i = 0; i < 5; ++i) require(outimg.rgba[i * 4 + 3] == src.rgba[i * 4 + 3], "alpha preserved");
        require(near(outimg.rgba[16], 1) && near(outimg.rgba[18], 0), "negative luminance clamps to shadow");
        require(near(outimg.rgba[12], 0) && near(outimg.rgba[14], 1), "HDR clamps to highlight");

        p.blend = 0.5;
        require(render(src, p, outimg), "blend render");
        require(near(outimg.rgba[4], .5) && near(outimg.rgba[5], .25) && near(outimg.rgba[6], .5),
                "blend halfway to original");
        p.blend = 1.;
        require(render(src, p, outimg) && outimg.rgba == src.rgba, "blend 100% is the original");

        int polls = 0;
        p.blend = 0.;
        require(!render(src, p, outimg, [&] { return ++polls > 0; }), "cancellation");
        for (int i = 0; i < 6; ++i) {
            auto q = p;
            if (i == 0) q.mode = -1;
            if (i == 1) q.mode = 4;
            if (i == 2) q.blend = -0.1;
            if (i == 3) q.blend = 1.1;
            if (i == 4) q.shadows[0] = std::numeric_limits<double>::quiet_NaN();
            if (i == 5) q.highlights[2] = 1.1;
            require(!valid(q) && !render(src, q, outimg), "invalid parameters rejected");
        }

        std::cout << "PASS " << checks << " Toner core checks: modes, stops, linear sRGB mapping, "
                     "blend direction, alpha, HDR/negative luminance and cancellation\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
