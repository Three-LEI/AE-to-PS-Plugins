#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/filter_runtime.hpp"
#include "fractal_core.hpp"
#include "fractal_info.hpp"
namespace fractal {
INT_PTR show_dialog(HINSTANCE, HWND, FilterRecord *, Parameters &);
}
namespace {
HINSTANCE module_instance = nullptr;
struct FractalEffect {
  using Parameters = fractal::Parameters;
  static constexpr unsigned long magic = 0x46524e32;
  static constexpr auto title = L"Fractal Noise 分形杂色";
  static constexpr auto info = fractal::plugin_info;
  static bool supported_depth(int depth) {
    return depth == 8 || depth == 16 || depth == 32;
  }
  static bool supported_mode(int mode) {
    return mode == plugInModeRGBColor || mode == plugInModeRGB48 ||
           mode == plugInModeRGB96;
  }
  static int sample_bytes(FilterRecord *f) {
    return f->depth == 32 ? 4 : f->depth == 16 ? 2 : 1;
  }
  static bool upgrade(const void *data, int32 size, Parameters &p) {
    struct OldParameters {
      double contrast, brightness, scale, rotation, offset_x, offset_y,
          complexity, sub_influence, sub_scale, evolution, opacity;
      std::uint32_t seed;
      bool invert;
    };
    struct OldSaved {
      unsigned long magic;
      OldParameters p;
    };
    if (size != sizeof(OldSaved))
      return false;
    OldSaved saved;
    std::memcpy(&saved, data, sizeof(saved));
    if (saved.magic != 0x46524e31)
      return false;
    const auto &o = saved.p;
    p.contrast = o.contrast;
    p.brightness = o.brightness;
    p.scale = o.scale;
    p.rotation = o.rotation;
    p.offset_x = o.offset_x;
    p.offset_y = o.offset_y;
    p.complexity = o.complexity;
    p.sub_influence = o.sub_influence;
    p.sub_scale = o.sub_scale;
    p.evolution = o.evolution;
    p.opacity = o.opacity;
    p.seed = o.seed;
    p.invert = o.invert;
    p.overflow = fractal::Overflow::clip;
    return valid(p);
  }
  static Parameters defaults(FilterRecord *f) {
    Parameters p;
    p.offset_x = f->bigDocumentData ? f->bigDocumentData->imageSize32.h / 2.0
                                    : f->imageSize.h / 2.0;
    p.offset_y = f->bigDocumentData ? f->bigDocumentData->imageSize32.v / 2.0
                                    : f->imageSize.v / 2.0;
    return p;
  }
  static bool valid(const Parameters &p) { return fractal::valid(p); }
  static INT_PTR dialog(HINSTANCE m, HWND w, FilterRecord *f, Parameters &p) {
    return fractal::show_dialog(m, w, f, p);
  }
  static int16 render(FilterRecord *f, VRect r, const Parameters &p) {
    const auto w = static_cast<std::size_t>(r.right - r.left),
               h = static_cast<std::size_t>(r.bottom - r.top);
    const auto bytes = sample_bytes(f);
    return fractal::process(
               f->inData, (h - 1) * f->inRowBytes + w * 3 * bytes,
               f->inRowBytes, f->outData,
               (h - 1) * f->outRowBytes + w * 3 * bytes, f->outRowBytes, w, h,
               3, p,
               {static_cast<double>(r.left), static_cast<double>(r.top), 1,
                (f->bigDocumentData ? f->bigDocumentData->imageSize32.h
                                    : f->imageSize.h) /
                    2.0,
                (f->bigDocumentData ? f->bigDocumentData->imageSize32.v
                                    : f->imageSize.v) /
                    2.0},
               f->depth == 32   ? fractal::SampleType::f32
               : f->depth == 16 ? fractal::SampleType::photoshop_u16
                                : fractal::SampleType::u8)
               ? noErr
               : filterBadParameters;
  }
};
} // namespace
BOOL WINAPI DllMain(HINSTANCE m, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH)
    module_instance = m;
  return TRUE;
}
extern "C" __declspec(dllexport) void PluginMain(int16 s, FilterRecord *f,
                                                 intptr_t *d, int16 *r) {
  psfilter::Runtime<FractalEffect>::entry(module_instance, s, f, d, r);
}
