#include "texturewrap_core.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace texturewrap;

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

Image image(int width, int height, int left = 0, int top = 0) {
    return {width, height, left, top, std::vector<float>(std::size_t(width) * height * 4)};
}

template<class T> auto pixel(T& img, int x, int y) {
    return img.rgba.data() + (std::size_t(y - img.top) * img.width + x - img.left) * 4;
}

void near(float a, float b, const char* message) { require(std::abs(a - b) < 1e-5f, message); }

void sphere_mapping() {
    auto src = image(161, 121, -20, 8);
    const Bounds texture{-20, 13, -12, 17};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 8; ++x) {
        auto* p = pixel(src, texture.left + x, texture.top + y);
        p[0] = float(x) * .25f;
        p[1] = float(y) / 3;
        p[2] = -.25f;
        p[3] = 1;
    }
    Bounds found;
    require(find_texture_bounds(src, found) && found == texture, "texture includes hidden document-coordinate pixels");
    const Mapping map{src.bounds(), found};
    auto p = defaults(Shape::sphere);
    p.radius = 40.25 * 100 / 121;
    p.lighting = false;
    Image out;
    require(render(Shape::sphere, src, p, map, out), "sphere renders");
    auto* center = pixel(out, 60, 68);
    near(center[0], .875f, "longitude at front");
    near(center[1], .5f, "latitude at front");
    near(center[2], -.25f, "unlit signed HDR retained");
    near(center[3], 1, "sphere center opaque");
    require(pixel(out, 100, 68)[3] > 0 && pixel(out, 100, 68)[3] < 1, "geometric antialiasing");
    near(pixel(out, 102, 68)[3], 0, "transparent exterior");

    p.rotation_y = 180;
    require(render(Shape::sphere, src, p, map, out), "seam rotated to front");
    near(pixel(out, 60, 68)[0], .875f, "seam blends last and first columns");
    require(std::abs(pixel(out, 59, 68)[0] - pixel(out, 61, 68)[0]) < .12f, "seam is continuous");

    p.rotation_y = 0;
    p.rotation_x = 90;
    require(render(Shape::sphere, src, p, map, out), "north pole to front");
    near(pixel(out, 60, 68)[1], 0, "north pole clamps to top row");
    p.rotation_x = -90;
    require(render(Shape::sphere, src, p, map, out), "south pole to front");
    near(pixel(out, 60, 68)[1], 1, "south pole clamps to bottom row");

    p.rotation_x = 0;
    p.rotation_z = 90;
    require(render(Shape::sphere, src, p, map, out), "roll");
    require(pixel(out, 80, 68)[1] > pixel(out, 40, 68)[1], "roll reorients texture");
}

void cylinder_mapping() {
    auto src = image(161, 121);
    const Bounds texture{0, 0, 8, 4};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 8; ++x) {
        auto* t = pixel(src, x, y);
        t[0] = float(x) * .25f;
        t[1] = float(y) / 3;
        t[3] = 1;
    }
    const Mapping map{src.bounds(), texture};
    auto p = defaults(Shape::cylinder);
    p.radius = 35.25 * 100 / 121;
    p.height = 60.5 * 100 / 121;
    p.lighting = false;
    Image out;
    require(render(Shape::cylinder, src, p, map, out), "upright cylinder");
    const auto* center = pixel(out, 80, 60);
    near(center[0], .875f, "front longitude on cylinder");
    near(center[1], .5f, "half height maps to texture middle");
    near(pixel(out, 80, 34)[1], 0, "top maps to top edge");
    near(pixel(out, 80, 86)[1], 1, "bottom maps to bottom edge");
    require(pixel(out, 80, 30)[3] > 0 && pixel(out, 80, 30)[3] < 1, "cylinder ends are antialiased");
    near(pixel(out, 80, 27)[3], 0, "outside cylinder height is transparent");
    require(pixel(out, 115, 60)[3] > 0 && pixel(out, 115, 60)[3] < 1, "cylinder sides are antialiased");

    p.rotation_y = 180;
    require(render(Shape::cylinder, src, p, map, out), "cylinder can rotate to seam");
    near(pixel(out, 80, 60)[0], .875f, "cylinder seam wraps opposite end texels");
    p.rotation_y = 0;
    p.rotation_x = 30;
    require(render(Shape::cylinder, src, p, map, out), "tilted cylinder");
    require(pixel(out, 80, 60)[1] < .35f, "tilt changes vertical mapping");
    near(pixel(out, 80, 31)[3], 0, "open top shows no cap or inside surface");
    near(pixel(out, 80, 96)[3], 1, "bottom rim moves with tilt");
    p.rotation_x = 90;
    require(render(Shape::cylinder, src, p, map, out), "axis-on cylinder");
    require(std::none_of(out.rgba.begin(), out.rgba.end(), [](float v) { return v != 0; }),
            "axis-on sidewall is transparent and finite");

    p.rotation_x = 0;
    p.rotation_z = 90;
    require(render(Shape::cylinder, src, p, map, out), "rolled cylinder");
    require(pixel(out, 60, 60)[1] < pixel(out, 100, 60)[1], "roll rotates the cylinder axis");
}

void cylinder_render_sides() {
    auto src = image(161, 121);
    const Bounds texture{0, 0, 8, 4};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 8; ++x) {
        auto* t = pixel(src, x, y);
        const bool front = x >= 2 && x < 6;
        t[front ? 0 : 2] = 1;
        t[3] = .5f;
    }
    const Mapping map{src.bounds(), texture};
    auto p = defaults(Shape::cylinder);
    p.radius = 35.25 * 100 / 121;
    p.height = 60.5 * 100 / 121;
    p.lighting = false;
    Image front, back, both, region;
    require(p.render_sides == FrontOnly && render(Shape::cylinder, src, p, map, front), "legacy default renders front");
    p.render_sides = BackOnly;
    require(render(Shape::cylinder, src, p, map, back), "render far side independently");
    near(pixel(back, 80, 60)[2], 1, "back samples the far longitude");
    near(pixel(back, 80, 60)[3], .5f, "back preserves texture alpha");
    p.render_sides = FrontAndBack;
    require(render(Shape::cylinder, src, p, map, both), "render both sides");
    near(pixel(both, 80, 60)[0], 2.f / 3, "front is above the back");
    near(pixel(both, 80, 60)[2], 1.f / 3, "back shows through semitransparent front");
    near(pixel(both, 80, 60)[3], .75f, "ordered alpha compositing");

    p.rotation_x = 30;
    require(render(Shape::cylinder, src, p, map, both), "render the rear wall through the opening");
    near(pixel(both, 80, 31)[2], 1, "tilted opening shows the rear wall");
    near(pixel(both, 80, 96)[0], 1, "near wall below the opening");
    require(render_region(Shape::cylinder, src, p, map, {50, 20, 120, 107}, 1, region),
            "both-sided native preview crop");
    for (int y = 20; y < 107; ++y) for (int x = 50; x < 120; ++x) {
        const auto* q = pixel(region, x, y);
        require(std::equal(q, q + 4, pixel(both, x, y)), "both-sided crop equals final");
    }

    p.rotation_x = 0;
    p.render_sides = BackOnly;
    p.lighting = true;
    p.ambient = 0;
    p.diffuse = 100;
    p.specular = 0;
    p.light_elevation = 90;
    require(render(Shape::cylinder, src, p, map, back), "light the inside of the far wall");
    near(pixel(back, 80, 60)[2], 1, "inside normal faces the camera");
    p.render_sides = 3;
    require(!valid(p), "unknown render mode rejected");
}

void cylinder_wrap_angle() {
    auto src = image(161, 121);
    const Bounds texture{0, 0, 8, 4};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 8; ++x) {
        auto* t = pixel(src, x, y);
        t[x < 4 ? 0 : 2] = 1;
        t[3] = 1;
    }
    const Mapping map{src.bounds(), texture};
    auto p = defaults(Shape::cylinder);
    p.radius = 40 * 100. / 121;
    p.lighting = false;
    Image full, arc;
    require(render(Shape::cylinder, src, p, map, full), "complete wrap");
    near(pixel(full, 116, 60)[3], 1, "complete wrap fills sidewall");
    p.wrap_angle = 120;
    require(render(Shape::cylinder, src, p, map, arc), "partial cylinder arc");
    near(pixel(arc, 46, 60)[0], 1, "left end holds the left texels");
    near(pixel(arc, 114, 60)[2], 1, "right end holds the right texels");
    near(pixel(arc, 114, 60)[0], 0, "partial edge does not sample the opposite end");
    require(pixel(arc, 115, 60)[3] > 0 && pixel(arc, 115, 60)[3] < 1, "arc cuts receive subpixel coverage");
    near(pixel(arc, 116, 60)[3], 0, "unused part of wall is transparent");

    p.render_sides = BackOnly;
    require(render(Shape::cylinder, src, p, map, arc), "arc on back only");
    near(pixel(arc, 80, 60)[3], 0, "front-centered arc outside the rear wall");
    p.rotation_y = 180;
    require(render(Shape::cylinder, src, p, map, arc), "rotate the arc to the rear");
    near(pixel(arc, 80, 60)[3], 1, "rotation moves the whole arc");
}

void cylinder_decal_placement() {
    auto target = image(161, 121);
    for (int y = 0; y < target.height; ++y) for (int x = 0; x < target.width; ++x) {
        auto* q = pixel(target, x, y);
        q[0] = float(x) / (target.width - 1);
        q[3] = 1;
    }
    auto label = image(8, 4, -12, 6);
    for (int y = 0; y < label.height; ++y) for (int x = 0; x < label.width; ++x) {
        auto* q = pixel(label, label.left + x, label.top + y);
        q[1] = float(x) / (label.width - 1);
        q[2] = float(y) / (label.height - 1);
        q[3] = 1;
    }
    const Mapping map{target.bounds(), label.bounds(), &label};
    auto body = defaults(Shape::cylinder);
    body.radius = 40 * 100. / 121;
    body.height = 60.5 * 100. / 121;
    body.lighting = false;
    auto p = body;
    p.wrap_angle = 36;
    p.cylinder_decal_angle = 45;
    p.cylinder_decal_position = 25;
    p.cylinder_decal_height = 40;
    Image full, out, crop;
    require(render(Shape::cylinder, target, p, map, out), "move and size the cylinder label independently");
    const auto* center = pixel(out, 108, 45);
    require(center[3] == 1 && std::abs(center[1] - .5f) < .06f && std::abs(center[2] - .5f) < .03f,
            "moved decal preserves its sampling coordinates");
    near(pixel(out, 80, 60)[3], 0, "label leaves the old center");
    require(pixel(out, 108, 33)[3] > 0 && pixel(out, 108, 33)[3] < 1, "label height cut is antialiased");
    require(pixel(out, 116, 45)[3] > 0 && pixel(out, 116, 45)[3] < 1, "moved arc cut is antialiased");

    auto rolled = p;
    rolled.rotation_z = 90;
    require(render(Shape::cylinder, target, rolled, map, crop), "rolled cylinder with a decal");
    near(pixel(crop, 65, 32)[3], 1, "placement follows the cylinder's own axes");
    near(pixel(crop, 108, 45)[3], 0, "the decal rotates with the body");

    require(render(Shape::cylinder, target, body, {}, full), "full body from the current layer");
    p.keep_current = true;
    require(render(Shape::cylinder, target, p, map, out), "apply a positioned decal over the full body");
    const auto* uncovered = pixel(out, 80, 60);
    require(std::equal(uncovered, uncovered + 4, pixel(full, 80, 60)), "positioning leaves the base texture in place");
    require(render_region(Shape::cylinder, target, p, map, {65, 30, 125, 74}, 1, crop), "positioned decal in the viewport");
    for (int y = 30; y < 74; ++y) for (int x = 65; x < 125; ++x) {
        const auto* q = pixel(crop, x, y);
        require(std::equal(q, q + 4, pixel(out, x, y)), "native preview and final label pixels match");
    }

    const Mapping own_texture{target.bounds(), target.bounds()};
    auto self = p;
    Image decal_only;
    require(render(Shape::cylinder, target, self, own_texture, decal_only), "current-layer decal with legacy reference setting");
    near(pixel(decal_only, 80, 60)[3], 0, "reference toggle does not enable a same-layer base");
    self.keep_current = false;
    self.keep_current_for_self_decal = true;
    require(render(Shape::cylinder, target, self, own_texture, out), "overlay the current cylinder texture on itself");
    require(std::equal(pixel(out, 80, 60), pixel(out, 80, 60) + 4, pixel(full, 80, 60)),
            "the full same-layer mapping shows outside the label");
    require(std::equal(pixel(out, 108, 45), pixel(out, 108, 45) + 4, pixel(decal_only, 108, 45)),
            "the same-layer decal keeps its shifted sampling coordinates");
    require(std::abs(pixel(out, 108, 45)[0] - pixel(full, 108, 45)[0]) > .05f,
            "the label contributes a different sample to the base");
    const Mapping copied_texture{target.bounds(), target.bounds(), &target, target.bounds()};
    for (const int sides : {FrontOnly, BackOnly, FrontAndBack}) {
        self.render_sides = sides;
        auto external = self;
        external.keep_current_for_self_decal = false;
        external.keep_current = true;
        require(render(Shape::cylinder, target, self, own_texture, out) &&
                render(Shape::cylinder, target, external, copied_texture, crop) && out.rgba == crop.rgba,
                "the same decal uses equivalent wall compositing in all render-side modes");
    }

    p.keep_current = false;
    p.cylinder_decal_angle = 180;
    p.cylinder_decal_position = 50;
    p.render_sides = BackOnly;
    require(render(Shape::cylinder, target, p, map, out), "rear decal spans the cylinder seam");
    near(pixel(out, 80, 60)[1], .5f, "rear seam samples the center of the decal");
    near(pixel(out, 79, 60)[3], 1, "rear label continues across the seam");
    near(pixel(out, 81, 60)[3], 1, "rear label covers both sides of the seam");

    p.cylinder_decal_angle = 0;
    p.cylinder_decal_position = 0;
    p.render_sides = FrontOnly;
    require(render(Shape::cylinder, target, p, map, out), "label centered at the top rim");
    near(pixel(out, 80, 29)[3], 0, "the cylinder rim clips the decal");
    require(pixel(out, 80, 31)[2] > .5f && pixel(out, 80, 31)[2] < .7f,
            "clipped texture keeps its position and scale");
    p.cylinder_decal_height = 0;
    require(!valid(p), "zero decal height rejected");
}

void sphere_local_decal() {
    auto src = image(161, 121);
    const Bounds texture{0, 0, 8, 4};
    for (int y = 0; y < 4; ++y) for (int x = 0; x < 8; ++x) {
        auto* t = pixel(src, x, y);
        t[0] = float(x) / 7;
        t[1] = float(y) / 3;
        t[3] = 1;
    }
    const Mapping map{src.bounds(), texture};
    auto p = defaults(Shape::sphere);
    p.local_decal = true;
    p.lighting = false;
    p.radius = 40 * 100. / 121;
    Image out, crop;
    require(render(Shape::sphere, src, p, map, out), "centered sphere decal");
    near(pixel(out, 80, 60)[0], .5f, "centered decal samples horizontal midpoint");
    near(pixel(out, 80, 60)[1], .5f, "centered decal samples vertical midpoint");
    near(pixel(out, 99, 60)[0], 1, "decal end holds its own color");
    require(pixel(out, 100, 60)[3] > 0 && pixel(out, 100, 60)[3] < 1, "decal cut is antialiased");
    near(pixel(out, 101, 60)[3], 0, "outside decal is transparent");
    near(pixel(out, 92, 60)[3], 1, "inside the long dimension");
    near(pixel(out, 80, 48)[3], 0, "short dimension preserves the input aspect ratio");

    p.decal_longitude = 45;
    p.decal_latitude = 20;
    require(render(Shape::sphere, src, p, map, out), "move decal by latitude and longitude");
    near(pixel(out, 107, 46)[3], 1, "decal follows its independent spherical position");
    near(pixel(out, 80, 60)[3], 0, "moving decal clears the old position");
    require(render_region(Shape::sphere, src, p, map, {92, 37, 123, 70}, 1, crop), "decal preview region");
    for (int y = 37; y < 70; ++y) for (int x = 92; x < 123; ++x) {
        const auto* q = pixel(crop, x, y);
        require(std::equal(q, q + 4, pixel(out, x, y)), "decal preview crop equals final pixels");
    }

    p.decal_latitude = 0;
    p.decal_longitude = 180;
    require(render(Shape::sphere, src, p, map, out), "hidden decal renders a valid image");
    require(std::none_of(out.rgba.begin(), out.rgba.end(), [](float v) { return v != 0; }), "hidden hemisphere is transparent");
    p.decal_latitude = 90;
    p.rotation_x = 90;
    require(render(Shape::sphere, src, p, map, out), "decal at the pole");
    near(pixel(out, 80, 60)[3], 1, "local decal remains defined at a pole");

    p.rotation_x = 0;
    p.decal_latitude = 0;
    p.decal_longitude = 45;
    p.keep_current = true;
    Image decal_only, body;
    require(render(Shape::sphere, src, p, map, decal_only), "legacy current layer decal");
    near(pixel(decal_only, 80, 60)[3], 0, "the legacy external setting does not add a base");
    auto full = p;
    full.local_decal = false;
    require(render(Shape::sphere, src, full, map, body), "full wrap of the same texture");

    p.keep_current = false;
    p.keep_current_for_self_decal = true;
    require(render(Shape::sphere, src, p, map, out), "use the current texture for both the body and decal");
    const auto* outside = pixel(out, 80, 60);
    const auto* inside = pixel(out, 108, 60);
    require(std::equal(outside, outside + 4, pixel(body, 80, 60)), "base shows outside the decal");
    require(std::equal(inside, inside + 4, pixel(decal_only, 108, 60)), "local projection overlays the body");
    require(std::abs(inside[0] - pixel(body, 108, 60)[0]) > .05f, "the decal contributes its own sample coordinates");

    full.keep_current_for_self_decal = true;
    require(render(Shape::sphere, src, full, map, crop) && crop.rgba == body.rgba,
            "the self-decal toggle applies only in local decal mode");
    for (int y = texture.top; y < texture.bottom; ++y) for (int x = texture.left; x < texture.right; ++x)
        pixel(src, x, y)[3] = .5f;
    require(render(Shape::sphere, src, p, map, out), "compose a translucent texture over itself");
    near(pixel(out, 108, 60)[3], .75f, "overlap uses source-over alpha");
    near(pixel(out, 80, 60)[3], .5f, "base retains its transparency outside the decal");
}

void different_texture_layer(Shape shape) {
    auto target = image(161, 121, -20, -10);
    auto reference = image(16, 12, -350, 420);
    for (std::size_t i = 0; i < target.rgba.size(); i += 4) {
        target.rgba[i] = 1;
        target.rgba[i + 3] = 1;
    }
    for (int y = 2; y < 10; ++y) for (int x = 4; x < 12; ++x) {
        auto* t = pixel(reference, reference.left + x, reference.top + y);
        t[2] = 1.25f;
        t[3] = .5f;
    }
    auto p = defaults(shape);
    p.lighting = false;
    p.local_decal = true;
    p.wrap_angle = 120;
    Image self, other, crop;
    require(render(shape, target, p, {}, self), "default texture is the target layer");
    near(pixel(self, 60, 50)[0], 1, "target color by default");
    Bounds bounds;
    require(find_texture_bounds(reference, bounds), "reference bounds");
    const Mapping map{target.bounds(), bounds, &reference};
    require(render(shape, target, p, map, other), "use texture outside the target bounds");
    require(other.bounds() == target.bounds(), "target output geometry follows the target, not the texture");
    near(pixel(other, 60, 50)[0], 0, "target pixels are not sampled as a texture");
    near(pixel(other, 60, 50)[2], 1.25f, "reference HDR color mapped into the target");
    near(pixel(other, 60, 50)[3], .5f, "reference transparency");
    require(render_region(shape, target, p, map, {42, 35, 80, 66}, 1, crop), "cropped reference render");
    for (int y = 35; y < 66; ++y) for (int x = 42; x < 80; ++x) {
        const auto* q = pixel(crop, x, y);
        require(std::equal(q, q + 4, pixel(other, x, y)), "cropped preview uses the complete reference texture");
    }

    p.keep_current = true;
    Image combined;
    require(render(shape, target, p, {}, combined) && combined.rgba == self.rgba,
            "the overlay setting only applies to an external source");
    require(render(shape, target, p, map, combined), "composite an external texture on the same surface");
    const auto* center = pixel(combined, 60, 50);
    near(center[0], .5f, "the base shows through a translucent texture");
    near(center[2], .625f, "HDR texture alpha is applied once");
    near(center[3], 1, "the opaque base fills transparent coverage");
    near(pixel(other, 26, 50)[3], 0, "outside the selected decal or arc");
    near(pixel(combined, 26, 50)[0], 1, "the base covers the full surface beyond the decal or arc");
    near(pixel(combined, target.left, target.top)[3], 0, "the base is mapped to the surface");
    auto cached = map;
    cached.current_texture = target.bounds();
    require(render_region(shape, target, p, cached, {22, 35, 80, 66}, 1, crop), "cropped overlay preview");
    for (int y = 35; y < 66; ++y) for (int x = 22; x < 80; ++x) {
        const auto* q = pixel(crop, x, y);
        require(std::equal(q, q + 4, pixel(combined, x, y)), "overlay preview matches the final output");
    }

    if (shape == Shape::cylinder) {
        auto back_label = p;
        back_label.rotation_y = 180;
        back_label.render_sides = FrontAndBack;
        require(render(shape, target, back_label, map, crop), "apply a label to the back of a full cylinder");
        near(pixel(crop, 60, 50)[2], 0, "the front base occludes the label on the back wall");
        back_label.render_sides = BackOnly;
        require(render(shape, target, back_label, map, crop), "render only the back wall with the label");
        near(pixel(crop, 60, 50)[2], .625f, "the back-wall label appears when the front is hidden");
    }

    for (std::size_t i = 3; i < target.rgba.size(); i += 4) target.rgba[i] = 0;
    require(render(shape, target, p, map, combined) && combined.rgba == other.rgba,
            "a fully transparent base contributes no pixels");
}

void transparency_and_regions(Shape shape) {
    auto src = image(199, 151, -37, -22);
    const Bounds texture{-13, -11, 29, 16};
    for (int y = texture.top; y < texture.bottom; ++y) for (int x = texture.left; x < texture.right; ++x) {
        auto* q = pixel(src, x, y);
        q[0] = x % 2 ? 1.5f : 0;
        q[1] = x % 2 ? 0 : 1;
        q[3] = x % 2 ? .5f : 0;
    }
    const Mapping map{{0, 0, 163, 123}, texture};
    auto p = defaults(shape);
    p.lighting = false;
    p.rotation_x = 13;
    p.rotation_y = -27;
    p.rotation_z = 9;
    Image full;
    require(render(shape, src, p, map, full), "transparent HDR texture");
    for (std::size_t i = 0; i < full.rgba.size(); i += 4) {
        near(full.rgba[i + 1], 0, "hidden texel RGB does not bleed");
        if (full.rgba[i + 3] > 0) near(full.rgba[i], 1.5f, "straight HDR through partial alpha");
    }
    p.lighting = true;
    Image lit, region, draft;
    require(render(shape, src, p, map, lit), "lit render");
    require(lit.rgba != full.rgba, "light changes RGB");
    for (std::size_t i = 3; i < lit.rgba.size(); i += 4) near(lit.rgba[i], full.rgba[i], "lighting preserves alpha");
    require(render_region(shape, src, p, map, {6, 14, 131, 111}, 1, region), "visible region");
    for (int y = 14; y < 111; ++y) for (int x = 6; x < 131; ++x) {
        const auto* q = pixel(region, x, y);
        require(std::equal(q, q + 4, pixel(lit, x, y)), "100% preview region exactly equals final");
    }
    require(render_region(shape, src, p, map, {-38, -23, 145, 107}, 3, draft) &&
            draft.left == -37 && draft.top == -22 && draft.width == 61 && draft.height == 43, "draft grid and clipping");
    const auto old = region.rgba;
    require(!render(shape, src, p, map, region, [] { return true; }) && region.rgba == old,
            "cancel leaves previous output intact");
    p.radius = std::numeric_limits<double>::quiet_NaN();
    require(!render(shape, src, p, map, region) && region.rgba == old, "invalid inputs rejected");
}
} // namespace

int main() {
    try {
        sphere_mapping();
        cylinder_mapping();
        cylinder_render_sides();
        cylinder_wrap_angle();
        cylinder_decal_placement();
        sphere_local_decal();
        for (const auto shape : {Shape::sphere, Shape::cylinder}) {
            different_texture_layer(shape);
            transparency_and_regions(shape);
        }
        std::cout << "PASS sphere and cylinder: texture overlays, arcs, decals, depth order, alpha/HDR, exact crops and cancellation\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
