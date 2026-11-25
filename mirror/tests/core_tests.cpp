#include "mirror_core.hpp"
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
        using namespace mirror;
        Image row{4, 1, 0, 0, std::vector<float>(16)};
        const float values[4] = {0.f, .25f, .5f, .75f};
        for (int i = 0; i < 4; ++i) {
            row.rgba[i * 4] = row.rgba[i * 4 + 1] = row.rgba[i * 4 + 2] = values[i];
            row.rgba[i * 4 + 3] = 1.f;
        }
        auto p = defaults(4, 1);
        Image out;
        require(valid(p) && p.cx == 2, "defaults");
        const auto proxied = proxy_parameters(p, 0, 0, 2);
        require(near(proxied.cx, 1) && near(proxied.cy, 0.25), "document center maps into proxy space");
        const auto panned = proxy_parameters(p, 96, 0, 1);
        require(near(panned.cx, -94) && near(panned.cy, 0.5), "pan maps into proxy space");
        require(render(row, p, out), "render angle 0");
        const double expected[4] = {0, .25, .25, 0};
        for (int i = 0; i < 4; ++i) require(near(out.rgba[i * 4], expected[i]), "left side mirrored to right");
        require(out.rgba[3] == 1 && out.rgba[7] == 1, "alpha preserved");

        Image column{1, 4, 0, 0, std::vector<float>(16)};
        for (int i = 0; i < 4; ++i) {
            column.rgba[i * 4] = column.rgba[i * 4 + 1] = column.rgba[i * 4 + 2] = values[i];
            column.rgba[i * 4 + 3] = .5f;
        }
        p = defaults(1, 4);
        p.angle = 90;
        require(render(column, p, out), "render angle 90");
        for (int i = 0; i < 4; ++i) require(near(out.rgba[i * 4], expected[i]), "top side mirrored to bottom");
        require(out.rgba[3] == .5f && out.rgba[15] == .5f, "vertical alpha preserved");

        p = defaults(4, 1);
        p.angle = 45;
        require(render(row, p, out), "diagonal render");
        require(out.valid(), "diagonal output valid");

        p = defaults(4, 1);
        p.cx = 0;
        p.angle = 0;
        require(render(row, p, out) && near(out.rgba[0], values[0]), "shifted center keeps source side");

        p = defaults(4, 1);
        p.angle = std::numeric_limits<double>::quiet_NaN();
        require(!valid(p) && !render(row, p, out), "NaN angle rejected");
        p = defaults(4, 1);
        p.cx = 1e8;
        require(!valid(p), "out-of-range center rejected");

        int polls = 0;
        p = defaults(4, 1);
        require(!render(row, p, out, [&] { return ++polls > 0; }), "cancellation");

        std::cout << "PASS " << checks << " Mirror core checks: 0°/90° reflection, center, alpha, diagonal, bounds, "
                     "invalid values and cancellation\n";
    } catch (const std::exception& e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
