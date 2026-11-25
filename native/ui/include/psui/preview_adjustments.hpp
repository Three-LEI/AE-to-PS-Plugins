#pragma once

#include "preview_view.hpp"
#include <array>
#include <span>

namespace psui {

struct PreviewPointSpace {
    double origin_x = 0, origin_y = 0, x_units = 1, y_units = 1;
    double minimum = -1e7, maximum = 1e7;

    static PreviewPointSpace offset(double x, double y) { return {x, y}; }
    static PreviewPointSpace percent(VRect bounds, double minimum = 0, double maximum = 100) {
        return {double(bounds.left), double(bounds.top),
                std::max(1., double(bounds.right) - bounds.left) * .01,
                std::max(1., double(bounds.bottom) - bounds.top) * .01, minimum, maximum};
    }
    double x(double value) const { return origin_x + value * x_units; }
    double y(double value) const { return origin_y + value * y_units; }
};

// Bindings are used only by the current image_view call. The panel retains the
// start values of a drag but never keeps pointers into effect parameters.
class PreviewAdjustments {
public:
    static constexpr std::size_t max_controls = 8;
    enum class Kind { Point, Angle };
    struct Control {
        Kind kind;
        const char* label;
        double* value;
        double* y = nullptr;
        PreviewPointSpace point_space{};
        double minimum = 0, maximum = 100;
        double x_center = 0, y_center = 0;
        double units = 1, phase = 0;
    };
    struct Guide { double x1, y1, x2, y2; };

    PreviewAdjustments& point(const char* label, double& x, double& y, PreviewPointSpace space = {}) {
        Control control{Kind::Point, label, &x, &y, space};
        control.minimum = space.minimum;
        control.maximum = space.maximum;
        return add(control);
    }
    PreviewAdjustments& angle(const char* label, double& value, double cx, double cy, double minimum,
                              double maximum, double phase = 0, double direction = 1) {
        Control control{Kind::Angle, label, &value};
        control.minimum = minimum;
        control.maximum = maximum;
        control.x_center = cx;
        control.y_center = cy;
        control.phase = phase;
        control.units = direction;
        return add(control);
    }
    PreviewAdjustments& line(double x1, double y1, double x2, double y2) {
        guide_ = {x1, y1, x2, y2};
        has_guide_ = true;
        return *this;
    }
    std::span<const Control> controls() const { return {controls_.data(), count_}; }
    const Guide* guide() const { return has_guide_ ? &guide_ : nullptr; }

private:
    PreviewAdjustments& add(Control control) {
        if (count_ < controls_.size()) controls_[count_++] = control;
        return *this;
    }

    std::array<Control, max_controls> controls_{};
    std::size_t count_ = 0;
    Guide guide_{};
    bool has_guide_ = false;
};

} // namespace psui
