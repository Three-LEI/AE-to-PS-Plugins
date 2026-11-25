#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "weather_host.hpp"
#include "parameter_schema.hpp"

namespace {
HINSTANCE instance = nullptr;

struct Effect {
    using Parameters = weather::Parameters;
    using Image = weather::Image;
    struct Inputs {};
    static constexpr auto magic = weather::magic;
    static constexpr auto info = weather::plugin_info;
    static constexpr auto title = weather::title;

    static Parameters defaults(FilterRecord*) { return {}; }
    static bool valid(const Parameters& p) { return weather::valid(p); }
    static void descriptor_defaults(Parameters& p) {
        const Parameters factory;
        std::copy_n(factory.rain_color, 3, p.rain_color);
        std::copy_n(factory.snow_color, 3, p.snow_color);
    }
    static bool upgrade(const void* bytes, int32 size, Parameters& p) {
        constexpr auto values_offset = offsetof(psfilter::Runtime<Effect>::Saved, values);
        constexpr auto legacy_payload = offsetof(Parameters, rain_color);
        static_assert(values_offset == 8 && legacy_payload == 80);
        if (size != int32(values_offset + legacy_payload)) return false;
        std::uint32_t identity = 0;
        std::memcpy(&identity, bytes, sizeof(identity));
        if (identity != magic) return false;
        Parameters restored;
        std::memcpy(&restored, static_cast<const std::byte*>(bytes) + values_offset, legacy_payload);
        p = restored;
        return true;
    }
    static Image from_host(psui::LayerPixels&& p) { return weather::from_host(std::move(p)); }
    static OSErr read_inputs(FilterRecord*, const std::vector<psui::HostLayer>&, const Parameters&, Inputs&) {
        return noErr;
    }
    static INT_PTR dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source,
                          const std::vector<psui::HostLayer>&, Parameters& p, Inputs&) {
        return weather::show_dialog(module, owner, host, source, p);
    }
    static bool process(const Image& source, const Parameters& p, Image& output, const Inputs&,
                        const weather::Cancel& cancel) {
        return weather::render(source, p, output, cancel);
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
