#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace psgrain {
inline std::uint32_t hash(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

inline double random(std::uint32_t seed, std::uint32_t id) {
    return hash(seed ^ hash(id)) / 4294967296.;
}

inline double noise(double x, double y, std::uint32_t seed) {
    const double floor_x = std::floor(x), floor_y = std::floor(y);
    const auto ix = std::int64_t(floor_x), iy = std::int64_t(floor_y);
    const double fx = x - floor_x, fy = y - floor_y;
    const double a = fx * fx * (3 - 2 * fx), b = fy * fy * (3 - 2 * fy);
    auto at = [&](int dx, int dy) {
        return random(seed ^ hash(std::uint32_t(ix + dx)), std::uint32_t(iy + dy) * 2654435761u);
    };
    return std::lerp(std::lerp(at(0, 0), at(1, 0), a), std::lerp(at(0, 1), at(1, 1), a), b) * 2 - 1;
}

inline double grain_noise(double x, double y, std::uint32_t seed, double softness) {
    if (softness == 0) return noise(x, y, seed);
    const double fx = std::floor(x), fy = std::floor(y);
    const auto ix = std::int64_t(fx), iy = std::int64_t(fy);
    auto smooth = [](double value) {
        const double t = std::clamp(value, 0., 1.);
        return t * t * (3 - 2 * t);
    };
    const double bx = smooth(x - fx), by = smooth(y - fy);
    const auto keys = std::array{seed ^ hash(std::uint32_t(ix - 1)), seed ^ hash(std::uint32_t(ix)),
                                 seed ^ hash(std::uint32_t(ix + 1)), seed ^ hash(std::uint32_t(ix + 2))};
    double center[4], sides[2];
    for (int row = 0; row < 4; ++row) {
        const auto key = std::uint32_t(iy + row - 1) * 2654435761u;
        const double middle_left = random(keys[1], key), middle_right = random(keys[2], key);
        center[row] = std::lerp(middle_left, middle_right, bx);
        if (row == 1 || row == 2) {
            sides[row - 1] = std::lerp(random(keys[0], key), middle_left, bx) +
                             std::lerp(middle_right, random(keys[3], key), bx);
        }
    }
    const double base = std::lerp(center[1], center[2], by);
    const double softened = .5 * base + .125 * (std::lerp(sides[0], sides[1], by) +
                            std::lerp(center[0], center[1], by) + std::lerp(center[2], center[3], by));
    return std::lerp(base, softened, softness) * 2 - 1;
}
}
