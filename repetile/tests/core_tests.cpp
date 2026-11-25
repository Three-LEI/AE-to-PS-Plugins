#include "repetile_core.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

int checks = 0;
void require(bool value, const char* why) {
    checks++;
    if (!value) throw std::runtime_error(why);
}
bool near(double a, double b, double eps = 1e-5) { return std::abs(a - b) <= eps; }

int main() {
    try {
        using namespace repetile;
        Image src{6, 6, 0, 0, std::vector<float>(6 * 6 * 4, 0.f)};
        const float colors[2][2][3] = {{{1, 0, 0}, {0, 1, 0}}, {{0, 0, 1}, {1, 1, 1}}};
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) {
                const auto i = (std::size_t(y + 2) * 6 + (x + 2)) * 4;
                for (int k = 0; k < 3; ++k) src.rgba[i + k] = colors[y][x][k];
                src.rgba[i + 3] = 1.f;
            }
        auto p = defaults();
        p.left = p.right = p.up = p.down = 2;
        Image out;
        require(valid(p) && render(src, p, out), "render expansion");
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) {
                const auto i = (std::size_t(y + 2) * 6 + (x + 2)) * 4;
                require(near(out.rgba[i], colors[y][x][0]) && near(out.rgba[i + 1], colors[y][x][1]) &&
                            near(out.rgba[i + 2], colors[y][x][2]) && out.rgba[i + 3] == 1,
                        "source content preserved");
            }
        require(near(out.rgba[(2 * 6 + 0) * 4], 1) && near(out.rgba[(2 * 6 + 0) * 4 + 1], 0),
                "repeat left expansion samples tile");
        require(near(out.rgba[(2 * 6 + 1) * 4 + 1], 1), "repeat second column");

        p.tiling = CheckerFlipH;
        require(render(src, p, out), "checker flip H");
        require(near(out.rgba[(2 * 6 + 0) * 4 + 1], 1), "checker flips alternate tile");

        p.tiling = Unfold;
        require(render(src, p, out), "unfold");
        // Top-left tile flips around both axes, so its far corner is white.
        require(out.rgba[0] == 1 && out.rgba[1] == 1 && out.rgba[2] == 1, "unfold flips both axes at a corner");
        p.tiling = Checker180;
        require(render(src, p, out), "checker 180");
        require(out.rgba[0] == 1 && out.rgba[1] == 0 && out.rgba[2] == 0, "checker 180 retains even-parity orientations");
        p.tiling = Slide;
        require(render(src,p,out), "slide");
        require(out.rgba[(2*6)*4+2] == 1 && out.rgba[(2*6)*4] == 0, "slide shifts columns vertically");
        p.tiling = Brick;
        require(render(src,p,out), "brick");
        require(out.rgba[1] == 1 && out.rgba[0] == 0, "brick shifts rows horizontally");

        p = defaults();
        p.left = p.right = p.up = p.down = 2;
        p.tiling = None;
        require(render(src, p, out), "none mode");
        require(out.rgba[(2 * 6 + 0) * 4 + 3] == 0, "none leaves expansion transparent");

        p.tiling = Repeat;
        p.blend = 2;
        require(render(src, p, out), "blend borders");
        require(out.rgba[(2 * 6 + 1) * 4 + 3] > 0, "blend fills border");

        p = defaults();
        p.left = -1;
        require(!valid(p) && !render(src, p, out), "negative expansion rejected");
        p = defaults();
        p.tiling = 99;
        require(!valid(p), "unknown tiling rejected");
        p = defaults();
        p.blend = -1;
        require(!valid(p), "negative blend rejected");
        p = defaults();
        p.up = std::numeric_limits<double>::quiet_NaN();
        require(!valid(p), "NaN expansion rejected");

        int polls = 0;
        p = defaults();
        p.left = p.right = p.up = p.down = 2;
        require(!render(src, p, out, [&] { return ++polls > 0; }), "cancellation");

        std::cout << "PASS " << checks << " RepeTile core checks: content bounds, four-way expansion, repeat/checker/"
                     "none modes, blend borders, invalid values and cancellation\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
