#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "noise_host.hpp"
#include "parameter_schema.hpp"

namespace {
HINSTANCE instance = nullptr;

struct Effect {
    using Parameters = noise::Parameters;
    using Image = noise::Image;
    struct Inputs {};
    static constexpr auto magic = noise::magic;
    static constexpr auto info = noise::plugin_info;
    static constexpr auto title = noise::title;

    static bool valid(const Parameters& p) { return noise::valid(p); }
    static Parameters defaults(FilterRecord*) { return {}; }
    static bool upgrade(const void* bytes, int32 size, Parameters& p) {
        // The 0.1 handle stored its identity and the original parameter prefix.
        constexpr std::size_t legacy_payload = 168;
        constexpr std::size_t values_offset = offsetof(psfilter::Runtime<Effect>::Saved, values);
        static_assert(values_offset == 8 && offsetof(Parameters, grain_midpoint) == legacy_payload);
        if (size != int32(values_offset + legacy_payload)) return false;
        std::uint32_t identity = 0;
        std::memcpy(&identity, bytes, sizeof(identity));
        if (identity != magic) return false;
        Parameters restored;
        std::memcpy(&restored, static_cast<const std::byte*>(bytes) + values_offset, legacy_payload);
        p = restored;
        return true;
    }
    static Image from_host(psui::LayerPixels&& p) { return noise::from_host(std::move(p)); }
    static OSErr read_inputs(FilterRecord*, const std::vector<psui::HostLayer>&, const Parameters&, Inputs&) {
        return noErr;
    }
    static INT_PTR dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& src,
                          const std::vector<psui::HostLayer>&, Parameters& p, Inputs&) {
        return noise::show_dialog(module, owner, host, src, p);
    }
    static bool process(const Image& src, const Parameters& p, Image& output, const Inputs&,
                        const noise::Cancel& cancel) {
        return noise::render(src, p, output, cancel);
    }
};
}

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
