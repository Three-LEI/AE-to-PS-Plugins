#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "compoundblur_host.hpp"
#include "parameter_schema.hpp"

namespace {
HINSTANCE instance = nullptr;

struct Effect {
    using Parameters = compoundblur::Parameters;
    using Image = compoundblur::Image;
    using Inputs = compoundblur::Inputs;
    static constexpr auto magic = compoundblur::magic;
    static constexpr auto info = compoundblur::plugin_info;
    static constexpr auto title = compoundblur::title;

    static Parameters defaults(FilterRecord*) { return {}; }
    static bool valid(const Parameters& p) { return compoundblur::valid(p); }
    static Image from_host(psui::LayerPixels&& p) { return compoundblur::from_host(std::move(p)); }
    static OSErr read_inputs(FilterRecord* host, const std::vector<psui::HostLayer>& layers, Parameters& p, Inputs& in) {
        in.domain = compoundblur::document_bounds(host);
        if (p.source == compoundblur::Reference) return compoundblur::load_reference(host, layers, p.layer_key, in.reference);
        if (!compoundblur::unique_layer(layers, p.layer_key)) p.layer_key = 0;
        return noErr;
    }
    static INT_PTR dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source,
                          const std::vector<psui::HostLayer>& layers, Parameters& p, Inputs& in) {
        return compoundblur::show_dialog(module, owner, host, source, layers, p, in);
    }
    static bool process(const Image& source, const Parameters& p, Image& output, const Inputs& in,
                        const compoundblur::Cancel& cancel) {
        return compoundblur::render(source, p, in.reference.get(), in.domain, output, cancel);
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
