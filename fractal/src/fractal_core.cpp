#include "fractal_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
namespace fractal {
namespace {
constexpr double pi = 3.14159265358979323846;
std::uint32_t mix(std::uint32_t h) {
  h ^= h >> 16;
  h *= 0x7feb352dU;
  h ^= h >> 15;
  h *= 0x846ca68bU;
  return h ^ (h >> 16);
}
double lattice(std::int64_t x, std::int64_t y, std::int64_t z,
               std::uint32_t seed) {
  auto h = mix(static_cast<std::uint32_t>(x) ^ seed);
  h = mix(h ^ static_cast<std::uint32_t>(y) * 0x9e3779b9U);
  h = mix(h ^ static_cast<std::uint32_t>(z) * 0x85ebca6bU);
  return (h >> 8) * (1.0 / 16777215.0);
}
double lerp(double a, double b, double t) { return a + (b - a) * t; }
double smooth(double t) { return t * t * (3 - 2 * t); }
double clamp(double v) { return std::clamp(v, 0.0, 1.0); }
double cubic(double a, double b, double c, double d, double t) {
  return b + 0.5 * t *
                 (c - a +
                  t * (2 * a - 5 * b + 4 * c - d + t * (3 * (b - c) + d - a)));
}
double plane(double x, double y, std::int64_t z, std::uint32_t seed,
             NoiseType kind) {
  x = std::remainder(x, 1048576.0);
  y = std::remainder(y, 1048576.0);
  const auto ix = static_cast<std::int64_t>(std::floor(x)),
             iy = static_cast<std::int64_t>(std::floor(y));
  double u = x - ix, v = y - iy;
  if (kind == NoiseType::block)
    return lattice(ix, iy, z, seed);
  if (kind == NoiseType::spline) {
    double rows[4];
    for (int j = -1; j <= 2; ++j)
      rows[j + 1] =
          cubic(lattice(ix - 1, iy + j, z, seed), lattice(ix, iy + j, z, seed),
                lattice(ix + 1, iy + j, z, seed),
                lattice(ix + 2, iy + j, z, seed), u);
    return clamp(cubic(rows[0], rows[1], rows[2], rows[3], v));
  }
  if (kind == NoiseType::soft_linear) {
    u = smooth(u);
    v = smooth(v);
  }
  return lerp(
      lerp(lattice(ix, iy, z, seed), lattice(ix + 1, iy, z, seed), u),
      lerp(lattice(ix, iy + 1, z, seed), lattice(ix + 1, iy + 1, z, seed), u),
      v);
}
struct Octave {
  double frequency, weight, cs, sn, dx, dy, time;
  std::uint32_t seed;
};
double sample(double x, double y, const Octave &o, const Parameters &p) {
  double time = o.time;
  if (p.cycle_evolution) {
    time = std::fmod(time, p.cycles);
    if (time < 0)
      time += p.cycles;
    time *= 4;
  }
  const auto z = static_cast<std::int64_t>(std::floor(time));
  auto next = z + 1;
  if (p.cycle_evolution)
    next %= static_cast<std::int64_t>(p.cycles) * 4;
  const auto t = smooth(time - std::floor(time)),
             a = plane(x, y, z, o.seed, p.noise);
  return t == 0 ? a : lerp(a, plane(x, y, next, o.seed, p.noise), t);
}
double burn(double a, double b) {
  return b <= 0 ? 0 : 1 - std::min(1.0, (1 - a) / b);
}
double dodge(double a, double b) {
  return b >= 1 ? 1 : std::min(1.0, a / (1 - b));
}
double blend(double a, double b, Blend mode) {
  switch (mode) {
  case Blend::darken:
  case Blend::darker_color:
    return std::min(a, b);
  case Blend::multiply:
    return a * b;
  case Blend::color_burn:
    return burn(a, b);
  case Blend::linear_burn:
    return std::max(0.0, a + b - 1);
  case Blend::lighten:
  case Blend::lighter_color:
    return std::max(a, b);
  case Blend::screen:
    return a + b - a * b;
  case Blend::color_dodge:
    return dodge(a, b);
  case Blend::add:
    return a + b;
  case Blend::overlay:
    return a <= .5 ? 2 * a * b : 1 - 2 * (1 - a) * (1 - b);
  case Blend::soft_light: {
    const double d =
        a <= .25 ? ((16 * a - 12) * a + 4) * a : std::sqrt(std::max(0.0, a));
    return b <= .5 ? a - (1 - 2 * b) * a * (1 - a) : a + (2 * b - 1) * (d - a);
  }
  case Blend::hard_light:
    return b <= .5 ? 2 * a * b : 1 - 2 * (1 - a) * (1 - b);
  case Blend::vivid_light:
    return b <= .5 ? burn(a, 2 * b) : dodge(a, 2 * b - 1);
  case Blend::linear_light:
    return a + 2 * b - 1;
  case Blend::pin_light:
    return b <= .5 ? std::min(a, 2 * b) : std::max(a, 2 * b - 1);
  case Blend::hard_mix:
    return (b <= .5 ? burn(a, 2 * b) : dodge(a, 2 * b - 1)) < .5 ? 0 : 1;
  case Blend::difference:
    return std::abs(a - b);
  case Blend::exclusion:
    return a + b - 2 * a * b;
  case Blend::subtract:
    return a - b;
  case Blend::divide:
    return b == 0 ? 1 : a / b;
  default:
    return b;
  }
}
struct RGB {
  double r, g, b;
};
RGB hsl_noise(RGB c, double value, bool hue) {
  const double hi = std::max({c.r, c.g, c.b}), lo = std::min({c.r, c.g, c.b}),
               delta = hi - lo, l = (hi + lo) / 2;
  if (delta < 1e-12)
    return c;
  double h = (hi == c.r   ? (c.g - c.b) / delta
              : hi == c.g ? (c.b - c.r) / delta + 2
                          : (c.r - c.g) / delta + 4) /
             6;
  if (h < 0)
    h += 1;
  double s = delta / (1 - std::abs(2 * l - 1));
  if (hue)
    h = clamp(value);
  else
    s = clamp(value);
  const double chroma = (1 - std::abs(2 * l - 1)) * s, h6 = h * 6,
               x = chroma * (1 - std::abs(std::fmod(h6, 2) - 1)),
               m = l - chroma / 2;
  RGB out = h6 < 1   ? RGB{chroma, x, 0}
            : h6 < 2 ? RGB{x, chroma, 0}
            : h6 < 3 ? RGB{0, chroma, x}
            : h6 < 4 ? RGB{0, x, chroma}
            : h6 < 5 ? RGB{x, 0, chroma}
                     : RGB{chroma, 0, x};
  return {out.r + m, out.g + m, out.b + m};
}
double lum(RGB c) { return .3 * c.r + .59 * c.g + .11 * c.b; }
RGB set_lum(RGB c, double l) {
  const double d = l - lum(c);
  c.r += d;
  c.g += d;
  c.b += d;
  const double lo = std::min({c.r, c.g, c.b}), hi = std::max({c.r, c.g, c.b});
  if (lo < 0) {
    const double t = l / (l - lo);
    c = {l + (c.r - l) * t, l + (c.g - l) * t, l + (c.b - l) * t};
  }
  if (hi > 1) {
    const double t = (1 - l) / (hi - l);
    c = {l + (c.r - l) * t, l + (c.g - l) * t, l + (c.b - l) * t};
  }
  return c;
}
bool extent(std::size_t w, std::size_t h, std::size_t row, unsigned c,
            std::size_t bytes, std::size_t &span) {
  if (w > std::numeric_limits<std::size_t>::max() / c)
    return false;
  const auto packed = w * c;
  if (row < packed ||
      (h - 1) > (std::numeric_limits<std::size_t>::max() - packed) / row)
    return false;
  span = (h - 1) * row + packed;
  return span <= bytes;
}
double load(const std::uint8_t *s, SampleType t) {
  if (t == SampleType::u8)
    return *s / 255.0;
  if (t == SampleType::photoshop_u16) {
    std::uint16_t v;
    std::memcpy(&v, s, 2);
    return v / 32768.0;
  }
  float v;
  std::memcpy(&v, s, 4);
  return std::isfinite(v) ? v : 0;
}
void store(std::uint8_t *d, double v, SampleType t) {
  if (t == SampleType::u8)
    *d = static_cast<std::uint8_t>(clamp(v) * 255 + .5);
  else if (t == SampleType::photoshop_u16) {
    auto a = static_cast<std::uint16_t>(clamp(v) * 32768 + .5);
    std::memcpy(d, &a, 2);
  } else {
    auto a = static_cast<float>(v);
    std::memcpy(d, &a, 4);
  }
}
template <class T> bool enum_valid(T v) {
  return static_cast<unsigned>(v) < static_cast<unsigned>(T::count);
}
} // namespace
bool valid(const Parameters &p) noexcept {
  for (double v : {p.contrast, p.brightness, p.scale, p.rotation, p.offset_x,
                   p.offset_y, p.complexity, p.sub_influence, p.sub_scale,
                   p.evolution, p.opacity, p.scale_width, p.scale_height,
                   p.sub_rotation, p.sub_offset_x, p.sub_offset_y, p.cycles})
    if (!std::isfinite(v))
      return false;
  return enum_valid(p.fractal) && enum_valid(p.noise) &&
         enum_valid(p.overflow) && enum_valid(p.blend) && p.contrast >= 0 &&
         p.contrast <= 1000 && std::abs(p.brightness) <= 1000 && p.scale >= 1 &&
         p.scale <= 10000 && p.scale_width >= 1 && p.scale_width <= 10000 &&
         p.scale_height >= 1 && p.scale_height <= 10000 &&
         std::abs(p.rotation) <= 100000 && std::abs(p.offset_x) <= 2000000 &&
         std::abs(p.offset_y) <= 2000000 && p.complexity >= 1 &&
         p.complexity <= 20 && p.sub_influence >= 0 && p.sub_influence <= 100 &&
         p.sub_scale >= 10 && p.sub_scale <= 100 &&
         std::abs(p.evolution) <= 1000000 && p.opacity >= 0 &&
         p.opacity <= 100 && std::abs(p.sub_rotation) <= 100000 &&
         std::abs(p.sub_offset_x) <= 2000000 &&
         std::abs(p.sub_offset_y) <= 2000000 && p.cycles >= 1 &&
         p.cycles <= 10000 && p.cycles == std::floor(p.cycles);
}
bool process(const void *source, std::size_t src_bytes, std::size_t src_row,
             void *target, std::size_t dst_bytes, std::size_t dst_row,
             std::size_t w, std::size_t h, unsigned channels,
             const Parameters &p, Region r, SampleType type) noexcept {
  if (!valid(p) || static_cast<unsigned>(type) > 2 || !std::isfinite(r.x) ||
      !std::isfinite(r.y) || !std::isfinite(r.step) || r.step <= 0 ||
      r.step > 2000000 || std::abs(r.x) > 2000000 || std::abs(r.y) > 2000000 ||
      !std::isfinite(r.center_x) || !std::isfinite(r.center_y) ||
      std::abs(r.center_x) > 2000000 || std::abs(r.center_y) > 2000000 ||
      w > 2000000 || h > 2000000 || !(channels == 3 || channels == 4))
    return false;
  if (!w || !h)
    return true;
  const unsigned bytes = type == SampleType::u8              ? 1
                         : type == SampleType::photoshop_u16 ? 2
                                                             : 4,
                 pixel = channels * bytes;
  auto *src = static_cast<const std::uint8_t *>(source);
  auto *dst = static_cast<std::uint8_t *>(target);
  std::size_t a, b;
  if (!src || !dst || !extent(w, h, src_row, pixel, src_bytes, a) ||
      !extent(w, h, dst_row, pixel, dst_bytes, b))
    return false;
  const auto sp = reinterpret_cast<std::uintptr_t>(src),
             dp = reinterpret_cast<std::uintptr_t>(dst),
             limit = std::numeric_limits<std::uintptr_t>::max();
  if (a > limit - sp || b > limit - dp)
    return false;
  if (src == dst ? src_row != dst_row : (sp < dp + b && dp < sp + a))
    return false;
  std::array<Octave, 20> oct{};
  int count = 0;
  double weight = 1, frequency = 1, total = 0, dx = 0, dy = 0;
  const double sub_cs = std::cos(p.sub_rotation * pi / 180),
               sub_sn = std::sin(p.sub_rotation * pi / 180),
               step = 100 / p.sub_scale;
  for (int i = 0; i < 20 && i < p.complexity; ++i) {
    const auto fraction = std::min(1.0, p.complexity - i);
    const auto angle = i * p.sub_rotation * pi / 180;
    oct[count++] = {
        frequency,
        weight * fraction,
        std::cos(angle),
        std::sin(angle),
        dx,
        dy,
        p.evolution / 360 *
            (p.fractal == FractalType::dynamic_progressive ? i + 1 : 1),
        mix(p.seed + static_cast<std::uint32_t>(
                         p.center_subscale || p.fractal == FractalType::subscale
                             ? 0
                             : i) *
                         0x9e3779b9U)};
    const double next_dx =
        (dx * sub_cs + dy * sub_sn) * step + p.sub_offset_x / 100;
    dy = (-dx * sub_sn + dy * sub_cs) * step + p.sub_offset_y / 100;
    dx = next_dx;
    total += weight * fraction;
    weight *= p.sub_influence / 100;
    frequency *= step;
  }
  const double radians = p.rotation * pi / 180, cs = std::cos(radians),
               sn = std::sin(radians),
               sx = p.uniform_scale ? p.scale : p.scale_width,
               sy = p.uniform_scale ? p.scale : p.scale_height,
               opacity = p.opacity / 100;
  try {
  return pscpu::parallel_ranges(h,w*h*std::size_t(count),{},[&](std::size_t first,std::size_t last,const auto&){
  for (std::size_t y = first; y < last; ++y)
    for (std::size_t x = 0; x < w; ++x) {
      const auto *s = src + y * src_row + x * pixel;
      auto *d = dst + y * dst_row + x * pixel;
      if (opacity == 0) {
        if (s != d)
          std::memcpy(d, s, pixel);
        continue;
      }
      const double wx = r.x + (x + .5) * r.step, wy = r.y + (y + .5) * r.step,
                   px = wx - r.center_x, py = wy - r.center_y;
      // Offset is a separate translation in the untransformed noise field.
      // Changing it must not move the scale/rotation pivot to a corner.
      const double nx = p.rotation == 0 && sx == 100
                            ? (wx - p.offset_x) / 100
                            : (px * cs + py * sn) / sx +
                                  (r.center_x - p.offset_x) / 100,
                   ny = p.rotation == 0 && sy == 100
                            ? (wy - p.offset_y) / 100
                            : (-px * sn + py * cs) / sy +
                                  (r.center_y - p.offset_y) / 100;
      double noise = 0, previous = .5, peak = 0;
      for (int i = 0; i < count; ++i) {
        const auto &o = oct[i];
        double ax = (nx * o.cs + ny * o.sn) * o.frequency - o.dx,
               ay = (-nx * o.sn + ny * o.cs) * o.frequency - o.dy;
        if (p.perspective_offset) {
          // Perspective translates the octave field before the common outer
          // transform; scaling the layer must not move its center sample.
          ax -= p.offset_x / 100 * (o.frequency - 1);
          ay -= p.offset_y / 100 * (o.frequency - 1);
        }
        const auto warp = previous - .5;
        switch (p.fractal) {
        case FractalType::dynamic:
        case FractalType::dynamic_progressive:
          ax += warp * 2;
          ay -= warp * 2;
          break;
        case FractalType::dynamic_twist: {
          const auto angle = warp * 3;
          const double xx = ax;
          ax = ax * std::cos(angle) + ay * std::sin(angle);
          ay = -xx * std::sin(angle) + ay * std::cos(angle);
          break;
        }
        case FractalType::smeary:
          ax += warp * 8;
          ay *= .35;
          break;
        case FractalType::swirly:
          ax += 2 * std::sin(ny + warp * 4);
          ay += 2 * std::cos(nx + warp * 4);
          break;
        case FractalType::strings:
          ax *= .15;
          ay += warp * 3;
          break;
        case FractalType::threads:
          ax *= .07;
          ay += warp * 5;
          break;
        default:
          break;
        }
        double v = sample(ax, ay, o, p);
        previous = v;
        switch (p.fractal) {
        case FractalType::turbulent_smooth:
          v = std::abs(2 * v - 1);
          break;
        case FractalType::turbulent_basic:
          v = std::sqrt(std::abs(2 * v - 1));
          break;
        case FractalType::turbulent_sharp:
          v = std::pow(std::abs(2 * v - 1), 2);
          break;
        case FractalType::rocky:
          v = clamp(.5 + (v - .5) * 2);
          break;
        case FractalType::cloudy:
          v = smooth(v);
          break;
        case FractalType::terrain:
          v = 1 - std::abs(2 * v - 1);
          break;
        case FractalType::small_bumps:
          v = std::pow(1 - std::abs(2 * v - 1), 8);
          break;
        case FractalType::strings:
          v = std::pow(1 - std::abs(2 * v - 1), 12);
          break;
        case FractalType::threads:
          v = std::pow(1 - std::abs(2 * v - 1), 24);
          break;
        default:
          break;
        }
        noise += o.weight * v;
        peak = std::max(peak, o.weight * v);
      }
      double value =
          .5 +
          ((p.fractal == FractalType::maximum ? peak : noise / total) - .5) *
              p.contrast / 100 +
          p.brightness / 100;
      switch (p.overflow) {
      case Overflow::clip:
        value = clamp(value);
        break;
      case Overflow::soft_clamp:
        value = .5 + .5 * std::tanh(2 * (value - .5));
        break;
      case Overflow::wrap_back: {
        double q = std::fmod(value, 2);
        if (q < 0)
          q += 2;
        value = q <= 1 ? q : 2 - q;
        break;
      }
      default:
        break;
      }
      if (p.invert)
        value = 1 - value;
      double amount = opacity;
      if (p.blend == Blend::dissolve || p.blend == Blend::dancing_dissolve) {
        const auto h =
            lattice(static_cast<std::int64_t>(std::floor(wx)),
                    static_cast<std::int64_t>(std::floor(wy)),
                    p.blend == Blend::dancing_dissolve
                        ? static_cast<std::int64_t>(std::floor(p.evolution))
                        : 0,
                    p.seed);
        amount = h < opacity ? 1 : 0;
      }
      RGB base{load(s, type), load(s + bytes, type), load(s + 2 * bytes, type)},
          out{};
      if (p.blend == Blend::hue || p.blend == Blend::saturation)
        out = hsl_noise(base, value, p.blend == Blend::hue);
      else if (p.blend == Blend::color)
        out = set_lum({value, value, value}, lum(base));
      else if (p.blend == Blend::luminosity)
        out = set_lum(base, value);
      else if (p.blend == Blend::darker_color ||
               p.blend == Blend::lighter_color) {
        const bool use = p.blend == Blend::darker_color ? value < lum(base)
                                                        : value > lum(base);
        out = use ? RGB{value, value, value} : base;
      } else
        out = {blend(base.r, value, p.blend), blend(base.g, value, p.blend),
               blend(base.b, value, p.blend)};
      const double vals[] = {out.r, out.g, out.b},
                   bases[] = {base.r, base.g, base.b};
      for (unsigned c = 0; c < 3; ++c)
        store(d + c * bytes,
              p.blend == Blend::none ? amount * value
                                     : bases[c] + amount * (vals[c] - bases[c]),
              type);
      if (channels == 4 && s != d)
        std::memcpy(d + 3 * bytes, s + 3 * bytes, bytes);
    }
  return true;
  });
  } catch (...) { return false; }
}
} // namespace fractal
