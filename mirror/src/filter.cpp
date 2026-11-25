#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "mirror_host.hpp"
#include "parameter_schema.hpp"

namespace {
HINSTANCE instance = nullptr;
struct Effect {
    using Parameters = mirror::Parameters;
    using Image = mirror::Image;
    struct Inputs {};
    static constexpr auto magic = mirror::magic;
    static constexpr auto info = mirror::plugin_info;
    static constexpr auto title = mirror::title;
    static bool valid(const Parameters& p) { return mirror::valid(p); }
    static Parameters defaults(FilterRecord* f) {
        const auto& b = f->documentInfo->bounds;
        return mirror::defaults(b.right - b.left, b.bottom - b.top, b.left, b.top);
    }
    static Image from_host(psui::LayerPixels&& p) { return mirror::from_host(std::move(p)); }
    static OSErr read_inputs(FilterRecord*, const std::vector<psui::HostLayer>&, const Parameters&, Inputs&) { return noErr; }
    static INT_PTR dialog(HINSTANCE m, HWND owner, FilterRecord* f, const Image& src,
                          const std::vector<psui::HostLayer>&, Parameters& p, Inputs&) {
        return mirror::show_dialog(m, owner, f, src, p);
    }
    static bool process(const Image& src, const Parameters& p, Image& out, const Inputs&,
                        const mirror::Cancel& cancel) {
        return mirror::render(src, p, out, cancel);
    }
};
} // namespace

extern "C" __declspec(dllexport) void PluginMain(int16 s, FilterRecord* f, intptr_t* data, int16* result) {
    psfilter::CachedRuntime<Effect>::entry(instance, s, f, data, result);
}
BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        instance = h;
        DisableThreadLibraryCalls(h);
    }
    return TRUE;
}
