#include "texturewrap_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>

namespace texturewrap {
namespace {

constexpr double pi = std::numbers::pi;
using Pixel = std::array<double, 4>;

struct Vec3 {
    double x, y, z;
    Vec3 operator*(double factor) const { return {x * factor, y * factor, z * factor}; }
    Vec3 operator+(Vec3 other) const { return {x + other.x, y + other.y, z + other.z}; }
};

double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

struct Rotation {
    Vec3 x, y, z;
    Vec3 to_local(Vec3 v) const { return {dot(x, v), dot(y, v), dot(z, v)}; }
    Vec3 to_world(Vec3 v) const { return x * v.x + y * v.y + z * v.z; }
};

Rotation rotation(const Parameters& p) {
    const double ax = p.rotation_x * pi / 180, ay = p.rotation_y * pi / 180, az = p.rotation_z * pi / 180;
    const double cx = std::cos(ax), sx = std::sin(ax);
    const double cy = std::cos(ay), sy = std::sin(ay);
    const double cz = std::cos(az), sz = std::sin(az);
    const auto rotate = [&](Vec3 v) {
        const Vec3 x{v.x, cx * v.y - sx * v.z, sx * v.y + cx * v.z};
        const Vec3 y{cy * x.x + sy * x.z, x.y, -sy * x.x + cy * x.z};
        return Vec3{cz * y.x - sz * y.y, sz * y.x + cz * y.y, y.z};
    };
    return {rotate({1, 0, 0}), rotate({0, 1, 0}), rotate({0, 0, 1})};
}

struct Light {
    Vec3 direction, halfway;
    double ambient, diffuse, specular;
    bool enabled;

    explicit Light(const Parameters& p) : ambient(p.ambient * .01), diffuse(p.diffuse * .01),
        specular(p.specular * .01), enabled(p.lighting) {
        const double azimuth = p.light_direction * pi / 180;
        const double elevation = p.light_elevation * pi / 180;
        const double plane = std::cos(elevation);
        direction = {std::cos(azimuth) * plane, std::sin(azimuth) * plane, std::sin(elevation)};
        halfway = {direction.x, direction.y, 1 + direction.z};
        const double length = std::sqrt(dot(halfway, halfway));
        halfway = length > 1e-9 ? Vec3{halfway.x / length, halfway.y / length, halfway.z / length}
                                : Vec3{0, 0, 0};
    }

    void shade(Vec3 normal, Pixel& color) const {
        if (!enabled || color[3] == 0) return;
        const double facing = std::max(0., dot(normal, direction));
        const double intensity = ambient + diffuse * facing;
        double highlight = 0;
        if (specular > 0 && facing > 0) {
            const double h = std::clamp(dot(normal, halfway), 0., 1.);
            const double h2 = h * h, h4 = h2 * h2, h8 = h4 * h4;
            highlight = specular * h8 * h8 * facing * color[3];
        }
        for (int k = 0; k < 3; ++k) color[k] = color[k] * intensity + highlight;
    }
};

struct Texture {
    const Image& image;
    int left, top, width, height;

    Pixel sample(double u, double v, bool periodic) const {
        // A complete wrap repeats across its seam. A partial map holds its edge texels.
        const double source_x = u * width - .5;
        const double x = periodic ? source_x : std::clamp(source_x, 0., double(width - 1));
        const double y = std::clamp(v * height - .5, 0., double(height - 1));
        const int ix = int(std::floor(x)), iy = int(y);
        const int x0 = ix < 0 ? width - 1 : ix;
        const int x1 = x0 + 1 == width ? (periodic ? 0 : x0) : x0 + 1;
        const int y1 = std::min(iy + 1, height - 1);
        const double fx = x - ix, fy = y - iy;
        const auto texel = [&](int column, int row) {
            return image.rgba.data() + (std::size_t(top + row) * image.width + left + column) * 4;
        };
        const std::array texels{texel(x0, iy), texel(x1, iy), texel(x0, y1), texel(x1, y1)};
        const std::array weights{(1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy};
        Pixel color{};
        for (std::size_t i = 0; i < texels.size(); ++i) {
            const double alpha = std::clamp(double(texels[i][3]), 0., 1.) * weights[i];
            if (alpha == 0) continue;
            for (int k = 0; k < 3; ++k) color[k] += texels[i][k] * alpha;
            color[3] += alpha;
        }
        return color;
    }
};

enum class Coverage { empty, edge, interior };

double longitude(Vec3 local) { return .5 + std::atan2(local.x, local.z) / (2 * pi); }

Pixel over(Pixel front, const Pixel& back) {
    const double transmission = 1 - front[3];
    for (int k = 0; k < 4; ++k) front[k] += back[k] * transmission;
    return front;
}

struct SphereDecal {
    Rotation frame;
    std::array<Vec3, 4> edges;
    double half_width, half_height;

    SphereDecal(Rotation axes, const Parameters& p, const Texture& texture) {
        const double longitude = p.decal_longitude * pi / 180, latitude = p.decal_latitude * pi / 180;
        const double sl = std::sin(longitude), cl = std::cos(longitude);
        const double sp = std::sin(latitude), cp = std::cos(latitude);
        frame = {axes.to_world({cl, 0, -sl}), axes.to_world({-sl * sp, cp, -cl * sp}),
                 axes.to_world({sl * cp, sp, cl * cp})};
        // Project the graphic onto a tangent plane, keeping its aspect ratio there.
        const double half_angle = p.decal_size * pi / 360;
        const double half_size = std::sin(half_angle) / std::cos(half_angle);
        const int longest = std::max(texture.width, texture.height);
        half_width = half_size * texture.width / longest;
        half_height = half_size * texture.height / longest;
        const auto horizontal = frame.z * half_width, vertical = frame.z * half_height;
        edges = {horizontal + frame.x, horizontal + frame.x * -1,
                 vertical + frame.y, vertical + frame.y * -1};
    }

    Coverage coverage(Vec3 point, double half_pixel) const {
        // Bound the change of sqrt(r²-x²-y²) over the pixel footprint.
        const double depth_margin = 2 * half_pixel * (std::abs(point.x) + std::abs(point.y) + half_pixel) / point.z;
        bool crosses_edge = false;
        for (const auto plane : edges) {
            const double distance = dot(plane, point);
            const double margin = (std::abs(plane.x) + std::abs(plane.y)) * half_pixel +
                                  std::abs(plane.z) * depth_margin;
            if (distance <= -margin) return Coverage::empty;
            crosses_edge |= distance < margin;
        }
        return crosses_edge ? Coverage::edge : Coverage::interior;
    }

    Pixel sample(Vec3 normal, const Texture& texture) const {
        const Vec3 local = frame.to_local(normal);
        if (local.z <= 0) return {};
        const double width = half_width * local.z, height = half_height * local.z;
        if (std::abs(local.x) > width || std::abs(local.y) > height) return {};
        return texture.sample(.5 + .5 * local.x / width, .5 - .5 * local.y / height, false);
    }
};

struct Sphere {
    Rotation axes;
    double radius, radius_sq, safe_sq, outer_sq, half_pixel;
    std::optional<SphereDecal> decal;
    const Texture* base;

    Sphere(Rotation rotation, double r, int step, const Parameters& p, const Texture& texture, const Texture* base_texture)
        : axes(rotation), radius(r), radius_sq(r * r), half_pixel(.5 * step), base(base_texture) {
        const double margin = std::numbers::sqrt2 * half_pixel;
        const double safe = std::max(0., r - margin), outer = r + margin;
        safe_sq = safe * safe;
        outer_sq = outer * outer;
        if (p.local_decal) decal.emplace(axes, p, texture);
    }

    double extent_x() const { return radius; }
    double extent_y() const { return radius; }
    double decal_span(double half_size) const { return 2 * radius * half_size / std::hypot(1., half_size); }
    bool minifies(const Texture& texture) const {
        const double span_x = 2 * pi * radius, span_y = pi * radius;
        return texture.width > (decal ? decal_span(decal->half_width) : span_x) ||
               texture.height > (decal ? decal_span(decal->half_height) : span_y) ||
               (base && (base->width > span_x || base->height > span_y));
    }

    Coverage coverage(double x, double y) const {
        const double distance_sq = x * x + y * y;
        if (distance_sq >= outer_sq) return Coverage::empty;
        if (distance_sq >= safe_sq) return Coverage::edge;
        if (!decal) return Coverage::interior;
        const auto cut = decal->coverage({x, y, std::sqrt(radius_sq - distance_sq)}, half_pixel);
        return base && cut == Coverage::empty ? Coverage::interior : cut;
    }

    Pixel sample(double x, double y, const Texture& texture, const Light& light) const {
        const double z_sq = radius_sq - (x * x + y * y);
        if (z_sq <= 0) return {};
        const double scale = 1 / radius;
        const Vec3 normal{x * scale, y * scale, std::sqrt(z_sq) * scale};
        auto pixel = decal ? decal->sample(normal, texture) : Pixel{};
        if (!decal || (base && pixel[3] < 1)) {
            const Vec3 local = axes.to_local(normal);
            const double u = longitude(local), v = .5 - std::asin(std::clamp(local.y, -1., 1.)) / pi;
            if (!decal) pixel = texture.sample(u, v, true);
            if (base && pixel[3] < 1) pixel = over(pixel, base->sample(u, v, true));
        }
        light.shade(normal, pixel);
        return pixel;
    }
};

struct Cylinder {
    Vec3 along{}, across{}, front{}, local_across{}, local_front{};
    double radius, radius_sq, axis_z, half_height, half_projected_height;
    double width_extent, height_extent, side_margin = 0, end_margin = 0;
    double arc_fraction, arc_cosine = 0, cut_left = 0, cut_right = 0;
    double decal_sine = 0, decal_cosine = 1;
    double decal_center, decal_half_height, decal_bottom, decal_top;
    int sides;
    bool visible, decal_rotated;
    const Texture* base;

    Cylinder(Rotation axes, double r, double h, int step, const Parameters& p, const Texture* base_texture)
        : radius(r), radius_sq(r * r), axis_z(axes.y.z), half_height(h * .5),
          arc_fraction(p.wrap_angle / 360), sides(p.render_sides),
          decal_rotated(p.cylinder_decal_angle != 0), base(base_texture) {
        const Vec3 axis = axes.y;
        const double projected = std::hypot(axis.x, axis.y);
        half_projected_height = half_height * projected;
        decal_center = (.5 - p.cylinder_decal_position * .01) * (2 * half_projected_height);
        decal_half_height = half_projected_height * (p.cylinder_decal_height * .01);
        decal_bottom = std::max(-half_projected_height, decal_center - decal_half_height);
        decal_top = std::min(half_projected_height, decal_center + decal_half_height);
        width_extent = std::abs(axis.x) * half_height + r * std::sqrt(std::max(0., 1 - axis.x * axis.x));
        height_extent = std::abs(axis.y) * half_height + r * std::sqrt(std::max(0., 1 - axis.y * axis.y));
        visible = projected > 1e-10;
        if (!visible) return;

        along = {axis.x / projected, axis.y / projected, 0};
        across = {along.y, -along.x, 0};
        front = {-along.x * axis_z, -along.y * axis_z, projected};
        local_across = axes.to_local(across);
        local_front = axes.to_local(front);
        if (decal_rotated) {
            const double angle = p.cylinder_decal_angle * pi / 180;
            decal_sine = std::sin(angle);
            decal_cosine = std::cos(angle);
        }
        if (arc_fraction < 1) {
            // The two cuts are parallel to the cylinder axis in the projection.
            const double half_arc = pi * arc_fraction;
            arc_cosine = std::cos(half_arc);
            const Vec3 across_decal = to_decal(local_across);
            const double midpoint = r * across_decal.z * arc_cosine;
            const double offset = r * across_decal.x * std::sin(half_arc);
            cut_left = midpoint - offset;
            cut_right = midpoint + offset;
        }
        side_margin = (std::abs(across.x) + std::abs(across.y)) * .5 * step;
        const double along_margin = (std::abs(along.x) + std::abs(along.y)) * .5 * step;
        // Conservative bound on the square-root depth change within a pixel.
        end_margin = along_margin + std::abs(axis_z) * std::sqrt(2 * r * side_margin);
    }

    Vec3 to_decal(Vec3 local) const {
        if (!decal_rotated) return local;
        return {decal_cosine * local.x - decal_sine * local.z, local.y,
                decal_sine * local.x + decal_cosine * local.z};
    }

    double extent_x() const { return visible ? width_extent : 0; }
    double extent_y() const { return visible ? height_extent : 0; }
    bool minifies(const Texture& texture) const {
        const double span_x = 2 * pi * radius, span_y = 2 * half_projected_height;
        return texture.width > span_x * arc_fraction || texture.height > 2 * decal_half_height ||
               (base && (base->width > span_x || base->height > span_y));
    }

    Coverage coverage(double x, double y) const {
        if (!visible) return Coverage::empty;
        const double q = x * across.x + y * across.y;
        const double distance = std::abs(q);
        if (distance >= radius + side_margin) return Coverage::empty;
        if (distance >= radius - side_margin) return Coverage::edge;
        if (arc_fraction < 1 && (std::abs(q - cut_left) <= side_margin || std::abs(q - cut_right) <= side_margin))
            return Coverage::edge;
        const double depth = std::sqrt(radius_sq - q * q);
        const double axial = x * along.x + y * along.y, shift = axis_z * depth;
        const auto wall_coverage = [&](double y) {
            const double low = base ? -half_projected_height : decal_bottom;
            const double high = base ? half_projected_height : decal_top;
            if (y <= low - end_margin || y >= high + end_margin) return Coverage::empty;
            if (y <= low + end_margin || y >= high - end_margin) return Coverage::edge;
            if (base && (std::abs(y - decal_bottom) <= end_margin || std::abs(y - decal_top) <= end_margin))
                return Coverage::edge;
            return Coverage::interior;
        };
        if (sides == BackOnly) return wall_coverage(axial - shift);
        const auto near_wall = wall_coverage(axial + shift);
        if (sides == FrontOnly || near_wall == Coverage::edge) return near_wall;
        const auto far_wall = wall_coverage(axial - shift);
        return far_wall == Coverage::empty ? near_wall : far_wall;
    }

    Pixel wall(double q, double depth, double y, const Texture& texture, const Light& light) const {
        const double axial = y + axis_z * depth;
        if (std::abs(axial) >= half_projected_height) return {};
        const double a = q / radius, b = depth / radius;
        const Vec3 local{local_across.x * a + local_front.x * b, 0, local_across.z * a + local_front.z * b};
        const Vec3 decal = to_decal(local);
        const bool periodic = arc_fraction == 1;
        const bool on_label = axial > decal_bottom && axial < decal_top && (periodic || decal.z >= arc_cosine);
        if (!on_label && !base) return {};
        double decal_u = 0;
        Pixel pixel{};
        if (on_label) {
            decal_u = longitude(decal);
            const double u = periodic ? decal_u : .5 + (decal_u - .5) / arc_fraction;
            const double v = .5 - (axial - decal_center) / (2 * decal_half_height);
            pixel = texture.sample(u, v, periodic);
        }
        if (base && pixel[3] < 1) {
            const double u = on_label && !decal_rotated ? decal_u : longitude(local);
            const double v = .5 - axial / (2 * half_projected_height);
            pixel = over(pixel, base->sample(u, v, true));
        }
        const double face = depth < 0 ? -1 : 1;
        const Vec3 normal{face * (across.x * a + front.x * b), face * (across.y * a + front.y * b), face * front.z * b};
        light.shade(normal, pixel);
        return pixel;
    }

    Pixel sample(double x, double y, const Texture& texture, const Light& light) const {
        const double q = x * across.x + y * across.y;
        const double depth_sq = radius_sq - q * q;
        if (depth_sq <= 0) return {};
        const double depth = std::sqrt(depth_sq);
        const double axial = x * along.x + y * along.y;
        if (sides == BackOnly) return wall(q, -depth, axial, texture, light);
        auto pixel = wall(q, depth, axial, texture, light);
        if (sides == FrontAndBack && pixel[3] < 1) {
            pixel = over(pixel, wall(q, -depth, axial, texture, light));
        }
        return pixel;
    }
};

template<class Surface>
Pixel sample_pixel(const Surface& surface, const Texture& texture, const Light& light,
                   double x, double y, int step, bool minifying) {
    const auto coverage = surface.coverage(x, y);
    if (coverage == Coverage::empty) return {};
    const auto at = [&](double sx, double sy) { return surface.sample(sx, sy, texture, light); };
    const int samples = coverage == Coverage::edge ? 4 : minifying ? 2 : 1;
    if (samples == 1) return at(x, y);

    Pixel sum{};
    for (int j = 0; j < samples; ++j) for (int i = 0; i < samples; ++i) {
        const double dx = ((i + .5) / samples - .5) * step;
        const double dy = ((j + .5) / samples - .5) * step;
        const auto pixel = at(x + dx, y + dy);
        for (int k = 0; k < 4; ++k) sum[k] += pixel[k];
    }
    for (double& v : sum) v /= samples * samples;
    return sum;
}

template<class Surface>
bool render_surface(const Surface& surface, const Texture& texture, const Light& light,
                    double cx, double cy, int step, Image& image, const Cancel& cancel) {
    const auto clip = [](double i, int extent) { return int(std::clamp(i, 0., double(extent))); };
    const double left = cx - image.left, top = cy - image.top;
    const int x0 = clip(std::floor((left - surface.extent_x()) / step), image.width);
    const int x1 = clip(std::ceil((left + surface.extent_x()) / step), image.width);
    const int y0 = clip(std::floor((top - surface.extent_y()) / step), image.height);
    const int y1 = clip(std::ceil((top + surface.extent_y()) / step), image.height);
    if (x1 <= x0 || y1 <= y0) return true;
    const bool minifying = step > 1 || surface.minifies(texture);
    const auto rows = [&](std::size_t first, std::size_t end, const auto& stopped) {
        for (int y = y0 + int(first); y < y0 + int(end); ++y) {
            if (stopped()) return false;
            const double sy = cy - (double(image.top) + (y + .5) * step);
            for (int x = x0; x < x1; ++x) {
                const double sx = double(image.left) + (x + .5) * step - cx;
                const auto pixel = sample_pixel(surface, texture, light, sx, sy, step, minifying);
                if (pixel[3] == 0) continue;
                auto* dest = image.rgba.data() + (std::size_t(y) * image.width + x) * 4;
                for (int k = 0; k < 3; ++k) dest[k] = float(pixel[k] / pixel[3]);
                dest[3] = float(pixel[3]);
            }
        }
        return true;
    };
    const auto height = std::size_t(y1 - y0);
    return pscpu::parallel_ranges(height, height * (x1 - x0), cancel, rows);
}

} // namespace

Bounds intersect(Bounds a, Bounds b) {
    return {std::max(a.left, b.left), std::max(a.top, b.top),
            std::min(a.right, b.right), std::min(a.bottom, b.bottom)};
}

Parameters defaults(Shape shape) {
    Parameters p;
    if (shape == Shape::cylinder) p.radius = 30;
    return p;
}

bool valid(const Parameters& p) {
    const auto range = [](double value, double low, double high) { return value >= low && value <= high; };
    return range(p.center_x, -200, 300) && range(p.center_y, -200, 300) && range(p.radius, .1, 200) &&
           range(p.height, .1, 400) &&
           range(p.rotation_x, -360, 360) && range(p.rotation_y, -360, 360) && range(p.rotation_z, -360, 360) &&
           range(p.light_direction, -360, 360) && range(p.light_elevation, -90, 90) &&
           range(p.ambient, 0, 100) && range(p.diffuse, 0, 200) && range(p.specular, 0, 100) &&
           p.render_sides >= FrontOnly && p.render_sides <= FrontAndBack && range(p.wrap_angle, 1, 360) &&
           range(p.decal_longitude, -180, 180) && range(p.decal_latitude, -90, 90) && range(p.decal_size, 1, 160) &&
           range(p.cylinder_decal_angle, -180, 180) && range(p.cylinder_decal_position, 0, 100) &&
           range(p.cylinder_decal_height, 1, 100);
}

bool find_texture_bounds(const Image& source, Bounds& bounds, const Cancel& cancel) {
    if (!source.valid()) return false;
    int left = source.width, top = source.height, right = 0, bottom = 0;
    for (int y = 0; y < source.height; ++y) {
        if (cancel && cancel()) return false;
        const float* alpha = source.rgba.data() + std::size_t(y) * source.width * 4 + 3;
        int first = 0, end = source.width;
        while (first < end && alpha[std::size_t(first) * 4] <= 0) ++first;
        if (first == end) continue;
        while (end > first + 1 && alpha[std::size_t(end - 1) * 4] <= 0) --end;
        left = std::min(left, first);
        right = std::max(right, end);
        top = std::min(top, y);
        bottom = y + 1;
    }
    bounds = right > left ? Bounds{source.left + left, source.top + top,
                                   source.left + right, source.top + bottom} : Bounds{};
    return true;
}

bool render(Shape shape, const Image& source, const Parameters& p, const Mapping& map, Image& out,
            const Cancel& cancel) {
    return source.valid() && render_region(shape, source, p, map, source.bounds(), 1, out, cancel);
}

bool render_region(Shape shape, const Image& source, const Parameters& p, const Mapping& map,
                   Bounds area, int step, Image& out, const Cancel& cancel) {
    if (!source.valid() || !valid(p) || step < 1 || (cancel && cancel())) return false;
    const Bounds output = intersect(area, source.bounds());
    if (output.empty()) return false;
    const Bounds canvas = map.canvas.empty() ? source.bounds() : map.canvas;
    const auto& pixels = map.reference ? *map.reference : source;
    if (map.reference && !pixels.valid()) return false;
    const Bounds texture_bounds = map.texture.empty() ? pixels.bounds() : intersect(map.texture, pixels.bounds());
    if (texture_bounds.empty()) return false;
    const Texture texture{pixels, texture_bounds.left - pixels.left, texture_bounds.top - pixels.top,
                          int(texture_bounds.width()), int(texture_bounds.height())};
    std::optional<Texture> base;
    if (map.reference && p.keep_current) {
        Bounds current;
        if (map.current_texture) current = *map.current_texture;
        else if (!find_texture_bounds(source, current, cancel)) return false;
        current = intersect(current, source.bounds());
        if (!current.empty()) {
            base.emplace(Texture{source, current.left - source.left, current.top - source.top,
                                 int(current.width()), int(current.height())});
        }
    }
    const Texture* base_texture = base ? &*base : nullptr;
    if (!map.reference && p.keep_current_for_self_decal && can_overlay_current_layer(shape, p)) {
        base_texture = &texture;
    }

    const int w = int((output.width() + step - 1) / step), h = int((output.height() + step - 1) / step);
    Image next{w, h, output.left, output.top, std::vector<float>(std::size_t(w) * h * 4)};
    const double cx = canvas.left + p.center_x * .01 * canvas.width();
    const double cy = canvas.top + p.center_y * .01 * canvas.height();
    const double radius = p.radius * .01 * std::min(canvas.width(), canvas.height());
    const Light light(p);
    const auto axes = rotation(p);
    const double height = p.height * .01 * canvas.height();
    const bool rendered = shape == Shape::sphere
        ? render_surface(Sphere(axes, radius, step, p, texture, base_texture), texture, light, cx, cy, step, next, cancel)
        : render_surface(Cylinder(axes, radius, height, step, p, base_texture),
                         texture, light, cx, cy, step, next, cancel);
    if (!rendered) return false;
    if (cancel && cancel()) return false;
    out = std::move(next);
    return true;
}

} // namespace texturewrap
