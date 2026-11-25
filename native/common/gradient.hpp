#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <span>
#include <vector>

namespace pscpu::gradient {

using Color = std::array<double, 4>;
enum Interpolation { Linear, Smooth, Step };
enum class Shape { Linear, Radial, Reflected, Angular, Diamond };
inline constexpr int max_stops = 16;

struct Stop {
    double position = 0;
    Color color{0, 0, 0, 1};
    double midpoint = .5; // This stop to the next one.
};

struct Ramp {
    int count = 2;
    int interpolation = Linear;
    std::array<Stop, max_stops> stops{Stop{}, Stop{1, {1, 1, 1, 1}}};
};

struct Placement {
    int shape = int(Shape::Linear);
    double angle = 0, span = 100, center_x = 50, center_y = 50;
};

inline bool valid(const Placement& p) {
    return p.shape >= int(Shape::Linear) && p.shape <= int(Shape::Diamond) &&
        p.angle >= -180 && p.angle <= 180 && p.span >= 1 && p.span <= 400 &&
        p.center_x >= 0 && p.center_x <= 100 && p.center_y >= 0 && p.center_y <= 100;
}

// The field maps a rectangular document or layer into the ramp's coordinate.
class Field {
    Shape shape;
    double center_x, center_y, dx, dy, size;
public:
    Field(const Placement& p, double left, double top, double width, double height)
        : shape(Shape(p.shape)), center_x(left + width * p.center_x * .01),
          center_y(top + height * p.center_y * .01) {
        const double radians = p.angle * std::numbers::pi / 180;
        dx = std::cos(radians);
        dy = std::sin(radians);
        size = .01 * p.span * (shape == Shape::Radial || shape == Shape::Diamond ? std::max(width, height)
            : std::abs(dx) * width + std::abs(dy) * height);
    }

    double at(double x, double y) const {
        const double ox = x - center_x, oy = y - center_y;
        const double u = ox * dx + oy * dy, v = -ox * dy + oy * dx;
        switch (shape) {
        case Shape::Linear: return .5 + u / size;
        case Shape::Radial: return 2 * std::hypot(ox, oy) / size;
        case Shape::Reflected: return 2 * std::abs(u) / size;
        case Shape::Angular: {
            const double turn = std::atan2(v, u) / (2 * std::numbers::pi);
            return turn < 0 ? turn + 1 : turn;
        }
        case Shape::Diamond: return 2 * (std::abs(u) + std::abs(v)) / size;
        }
        return 0;
    }
};

inline bool valid(const Ramp& ramp) {
    if (ramp.count < 2 || ramp.count > max_stops || ramp.interpolation < Linear || ramp.interpolation > Step)
        return false;
    const auto unit = [](double value) { return value >= 0 && value <= 1; };
    double previous = 0;
    for (int i = 0; i < ramp.count; ++i) {
        const auto& stop = ramp.stops[i];
        if (!unit(stop.position) || stop.position < previous || !(stop.midpoint >= .01 && stop.midpoint <= .99) ||
            !std::all_of(stop.color.begin(), stop.color.end(), unit)) return false;
        previous = stop.position;
    }
    return true;
}

inline Color sample(const Ramp& ramp, double position) {
    const auto begin = ramp.stops.begin(), end = begin + ramp.count;
    const auto next = std::upper_bound(begin, end, position,
        [](double value, const Stop& stop) { return value < stop.position; });
    if (next == begin) return begin->color;
    const auto& first = *(next - 1);
    if (next == end) return first.color;
    const double t = (position - first.position) / (next->position - first.position);
    double blend = t <= first.midpoint ? .5 * t / first.midpoint
                                      : .5 + .5 * (t - first.midpoint) / (1 - first.midpoint);
    if (ramp.interpolation == Smooth) blend *= blend * (3 - 2 * blend);
    if (ramp.interpolation == Step) blend = t < first.midpoint ? 0 : 1;
    Color color;
    for (int c = 0; c < 4; ++c) color[c] = std::lerp(first.color[c], next->color[c], blend);
    return color;
}

inline int move(Ramp& ramp, int index, double position) {
    ramp.stops[index].position = std::clamp(position, 0., 1.);
    while (index > 0 && position < ramp.stops[index - 1].position) {
        std::swap(ramp.stops[index], ramp.stops[index - 1]);
        --index;
    }
    while (index + 1 < ramp.count && position > ramp.stops[index + 1].position) {
        std::swap(ramp.stops[index], ramp.stops[index + 1]);
        ++index;
    }
    return index;
}

inline int insert(Ramp& ramp, Stop stop) {
    if (ramp.count == max_stops) return -1;
    ramp.stops[ramp.count++] = stop;
    return move(ramp, ramp.count - 1, stop.position);
}

inline bool remove(Ramp& ramp, int index) {
    if (ramp.count <= 2 || index < 0 || index >= ramp.count) return false;
    std::move(ramp.stops.begin() + index + 1, ramp.stops.begin() + ramp.count, ramp.stops.begin() + index);
    ramp.stops[--ramp.count] = {};
    return true;
}

inline void reverse(Ramp& ramp) {
    std::reverse(ramp.stops.begin(), ramp.stops.begin() + ramp.count);
    for (int i = 0; i < ramp.count; ++i) {
        auto& stop = ramp.stops[i];
        stop.position = 1 - stop.position;
        stop.midpoint = i + 1 < ramp.count ? std::clamp(1 - ramp.stops[i + 1].midpoint, .01, .99) : .5;
    }
}

// The Action Manager blob has its own version and an explicit little-endian layout.
inline std::vector<std::uint8_t> encode(const Ramp& ramp) {
    if (!valid(ramp)) return {};
    std::vector<std::uint8_t> data{1, std::uint8_t(ramp.interpolation), std::uint8_t(ramp.count), 0};
    data.reserve(4 + 48 * ramp.count);
    const auto put = [&](double value) {
        const auto bits = std::bit_cast<std::uint64_t>(value);
        for (int byte = 0; byte < 8; ++byte) data.push_back(std::uint8_t(bits >> (8 * byte)));
    };
    for (int i = 0; i < ramp.count; ++i) {
        const auto& stop = ramp.stops[i];
        put(stop.position);
        for (double value : stop.color) put(value);
        put(stop.midpoint);
    }
    return data;
}

inline bool decode(std::span<const std::uint8_t> data, Ramp& ramp) {
    if (data.size() < 4 || data[0] != 1 || data[3] != 0 || data[2] < 2 || data[2] > max_stops ||
        data.size() != 4 + 48 * std::size_t(data[2])) return false;
    Ramp next;
    next.interpolation = data[1];
    next.count = data[2];
    std::size_t index = 4;
    const auto get = [&] {
        std::uint64_t bits = 0;
        for (int byte = 0; byte < 8; ++byte) bits |= std::uint64_t(data[index++]) << (8 * byte);
        return std::bit_cast<double>(bits);
    };
    for (int i = 0; i < next.count; ++i) {
        auto& stop = next.stops[i];
        stop.position = get();
        for (double& value : stop.color) value = get();
        stop.midpoint = get();
    }
    if (!valid(next)) return false;
    ramp = next;
    return true;
}

} // namespace pscpu::gradient
