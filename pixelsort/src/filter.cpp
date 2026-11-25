#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "pixelsort_host.hpp"
#include "parameter_schema.hpp"

namespace {
HINSTANCE instance = nullptr;

struct Effect {
    using Parameters = pixelsort::Parameters;
    using Image = pixelsort::Image;
    struct Inputs {};
    static constexpr auto magic = pixelsort::magic;
    static constexpr auto info = pixelsort::plugin_info;
    static constexpr auto title = pixelsort::title;

    static Parameters defaults(FilterRecord*) { return {}; }
    static bool valid(const Parameters& p) { return pixelsort::valid(p); }
    static Image from_host(psui::LayerPixels&& p) { return pixelsort::from_host(std::move(p)); }
    static OSErr read_inputs(FilterRecord*, const std::vector<psui::HostLayer>&, const Parameters&, Inputs&) {
        return noErr;
    }
    static INT_PTR dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source,
                          const std::vector<psui::HostLayer>&, Parameters& p, Inputs&) {
        return pixelsort::show_dialog(module, owner, host, source, p);
    }
    static bool process(const Image& source, const Parameters& p, Image& output, const Inputs&,
                        const pixelsort::Cancel& cancel) {
        return pixelsort::render(source, p, output, cancel);
    }
};
} // namespace

extern "C" __declspec(dllexport) void PluginMain(int16 selector, FilterRecord* host, intptr_t* data, int16* result) {
    psfilter::CachedRuntime<Effect>::entry(instance, selector, host, data, result);
}

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        instance = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
