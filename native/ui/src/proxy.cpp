#define WIN32_LEAN_AND_MEAN
#include "psui/photoshop_host.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace psui {
namespace {
// Save requests, not the host's persistent parameter handles or suite state.
struct Requests {
  FilterRecord *f;
  FilterRecord before;
  BigDocumentStruct big{};
  explicit Requests(FilterRecord *host) : f(host), before(*host) {
    if (f->bigDocumentData)
      big = *f->bigDocumentData;
  }
  ~Requests() {
    f->inRect = before.inRect;
    f->outRect = before.outRect;
    f->maskRect = before.maskRect;
    if (f->bigDocumentData) {
      f->bigDocumentData->inRect32 = big.inRect32;
      f->bigDocumentData->outRect32 = big.outRect32;
      f->bigDocumentData->maskRect32 = big.maskRect32;
    }
    f->inputRate = before.inputRate;
    f->maskRate = before.maskRate;
    f->inputPadding = before.inputPadding;
    f->maskPadding = before.maskPadding;
    f->inLoPlane = before.inLoPlane;
    f->inHiPlane = before.inHiPlane;
    f->wantLayout = before.wantLayout;
    f->inData = before.inData;
    f->outData = before.outData;
    f->maskData = before.maskData;
    f->inRowBytes = before.inRowBytes;
    f->maskRowBytes = before.maskRowBytes;
    f->inColumnBytes = before.inColumnBytes;
    f->inPlaneBytes = before.inPlaneBytes;
  }
};
void set_requests(FilterRecord *f, VRect r, bool mask) {
  if (f->bigDocumentData) {
    f->bigDocumentData->inRect32 = r;
    f->bigDocumentData->outRect32 = {};
    f->bigDocumentData->maskRect32 = mask ? r : VRect{};
  }
  f->inRect = {static_cast<int16>(r.top), static_cast<int16>(r.left),
               static_cast<int16>(r.bottom), static_cast<int16>(r.right)};
  f->outRect = {};
  f->maskRect = mask ? f->inRect : Rect{};
}
int floor_div(int n, int d) { return n / d - ((n % d) < 0 ? 1 : 0); }
int ceil_div(int n, int d) { return n / d + ((n % d) > 0 ? 1 : 0); }
} // namespace
OSErr host_read_proxy(FilterRecord *f, int max_side, ProxyImage &image) {
  if (!f) {
    image = {};
    return noErr;
  }
  const auto r = f->bigDocumentData
                     ? f->bigDocumentData->filterRect32
                     : VRect{f->filterRect.top, f->filterRect.left,
                             f->filterRect.bottom, f->filterRect.right};
  return host_read_proxy_area(f, max_side, r, image);
}
OSErr host_read_proxy_area(FilterRecord *f, int max_side, VRect r,
                           ProxyImage &image) {
  image = ProxyImage{};
  if (!f || !f->advanceState)
    return noErr;
  Requests restore(f);
  const int width = r.right - r.left, height = r.bottom - r.top;
  if (width <= 0 || height <= 0)
    return noErr;
  max_side = std::clamp(max_side, 16, 512);
  const int scale =
      std::max(1, (std::max(width, height) + max_side - 1) / max_side);
  if (scale > 32767 || (!f->samplingSupport && scale > 1))
    return noErr;
  const VRect small{floor_div(r.top, scale), floor_div(r.left, scale),
                    ceil_div(r.bottom, scale), ceil_div(r.right, scale)};
  image.width = small.right - small.left;
  image.height = small.bottom - small.top;
  image.origin_x = double(small.left) * scale;
  image.origin_y = double(small.top) * scale;
  image.step = scale;
  image.channels = f->inLayerPlanes == 3 && f->inTransparencyMask > 0 ? 4 : 3;
  f->inputRate = f->maskRate = scale * 65536;
  f->inputPadding = plugInWantsEdgeReplication;
  f->maskPadding = 0;
  f->wantLayout = 0;
  f->inLoPlane = 0;
  f->inHiPlane = static_cast<int16>(image.channels - 1);
  set_requests(f, small, f->haveMask);
  auto err = f->advanceState();
  if (err != noErr) {
    image = ProxyImage{};
    return err;
  }
  OSErr own_error = noErr;
  try {
    const int bytes = f->depth == 32 ? 4 : f->depth == 16 ? 2 : 1;
    const int col =
        f->inColumnBytes ? f->inColumnBytes : image.channels * bytes;
    const int plane = f->inPlaneBytes ? f->inPlaneBytes : bytes;
    if (!f->inData || col < image.channels * bytes || plane != bytes ||
        f->inRowBytes < (image.width - 1) * col + image.channels * bytes)
      own_error = filterBadParameters;
    else {
      image.pixels.resize(static_cast<std::size_t>(image.width) * image.height *
                          image.channels);
      const auto *source = static_cast<const std::uint8_t *>(f->inData);
      for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
          for (int c = 0; c < image.channels; ++c) {
            const auto *s = source + y * f->inRowBytes + x * col + c * bytes;
            auto &d = image.pixels[(y * image.width + x) * image.channels + c];
            if (bytes == 1)
              d = *s;
            else if (bytes == 2) {
              std::uint16_t v;
              std::memcpy(&v, s, 2);
              d = static_cast<std::uint8_t>(std::min(
                  255U, (static_cast<unsigned>(v) * 255 + 16384) / 32768));
            } else {
              float v;
              std::memcpy(&v, s, 4);
              d = std::isfinite(v) ? static_cast<std::uint8_t>(
                                         std::clamp(v, 0.f, 1.f) * 255 + .5f)
                                   : 0;
            }
          }
      if (f->haveMask && f->maskData && f->maskRowBytes >= image.width) {
        image.mask.resize(static_cast<std::size_t>(image.width) * image.height);
        for (int y = 0; y < image.height; ++y)
          std::memcpy(image.mask.data() + y * image.width,
                      static_cast<const std::uint8_t *>(f->maskData) +
                          y * f->maskRowBytes,
                      image.width);
      }
    }
  } catch (const std::bad_alloc &) {
    own_error = memFullErr;
  }
  set_requests(f, {}, false);
  err = f->advanceState();
  if (err != noErr || own_error != noErr)
    image = ProxyImage{};
  return err != noErr ? err : own_error;
}
bool host_display_rgb(FilterRecord *f, HDC dc, const RECT &r,
                      const std::uint8_t *rgb, int w, int h) {
  if (!f || !f->displayPixels || !rgb || w <= 0 || h <= 0)
    return false;
  PSPixelMap map{};
  map.version = 1;
  map.bounds = {0, 0, h, w};
  map.imageMode = plugInModeRGBColor;
  map.rowBytes = w * 3;
  map.colBytes = 3;
  map.planeBytes = 1;
  map.baseAddr = const_cast<std::uint8_t *>(rgb);
  const int x = r.left + (r.right - r.left - w) / 2,
            y = r.top + (r.bottom - r.top - h) / 2;
  return f->displayPixels(&map, &map.bounds, y, x, dc) == noErr;
}
} // namespace psui
