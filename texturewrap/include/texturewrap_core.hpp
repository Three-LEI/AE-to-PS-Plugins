#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

namespace texturewrap {

enum class Shape { sphere, cylinder };
enum RenderSides { FrontOnly, BackOnly, FrontAndBack };

struct Parameters {
    double center_x = 50, center_y = 50;
    double radius = 42, height = 72;
    double rotation_x = 0, rotation_y = 0, rotation_z = 0;
    double light_direction = 135, light_elevation = 50;
    double ambient = 32, diffuse = 68, specular = 20;
    bool lighting = true;
    // Leave the trailing padding of the 0.1 parameter block intact.
    alignas(double) int render_sides = FrontOnly;
    double wrap_angle = 360;
    double decal_longitude = 0, decal_latitude = 0, decal_size = 60;
    bool local_decal = false;
    std::uint64_t texture_key = 0;
    bool keep_current = false;
    // The 0.4.1 parameter block ends at the next double-aligned offset.
    double cylinder_decal_angle = 0, cylinder_decal_position = 50, cylinder_decal_height = 100;
    // Older descriptors may retain keep_current after switching back to the current layer.
    bool keep_current_for_self_decal = false;
    bool operator==(const Parameters&) const = default;
};

inline bool can_overlay_current_layer(Shape shape, const Parameters& parameters) {
    return shape == Shape::cylinder || parameters.local_decal;
}

struct Bounds {
    int left = 0, top = 0, right = 0, bottom = 0;
    std::int64_t width() const { return std::int64_t(right) - left; }
    std::int64_t height() const { return std::int64_t(bottom) - top; }
    bool empty() const { return width() <= 0 || height() <= 0; }
    bool operator==(const Bounds&) const = default;
};

struct Image {
    int width = 0, height = 0, left = 0, top = 0;
    std::vector<float> rgba;
    bool valid() const {
        return width > 0 && height > 0 && width <= 2000000 && height <= 2000000 &&
               std::uint64_t(width) * height <= 16777216 &&
               std::int64_t(left) + width <= std::numeric_limits<int>::max() &&
               std::int64_t(top) + height <= std::numeric_limits<int>::max() &&
               rgba.size() == std::size_t(width) * height * 4;
    }
    Bounds bounds() const { return {left, top, left + width, top + height}; }
};

struct Mapping {
    Bounds canvas;
    Bounds texture;
    const Image* reference = nullptr;
    std::optional<Bounds> current_texture = std::nullopt;
};

using Cancel = std::function<bool()>;

Bounds intersect(Bounds, Bounds);
Parameters defaults(Shape);
bool valid(const Parameters&);
bool find_texture_bounds(const Image&, Bounds&, const Cancel& = {});
bool render(Shape, const Image&, const Parameters&, const Mapping&, Image&, const Cancel& = {});

// Bounds and step use document pixels. A unit step reproduces the matching part
// of the full output while still sampling the complete source texture.
bool render_region(Shape, const Image&, const Parameters&, const Mapping&, Bounds, int step,
                   Image&, const Cancel& = {});

} // namespace texturewrap
