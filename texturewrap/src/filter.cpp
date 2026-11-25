#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "texturewrap_host.hpp"
#include "parameter_schema.hpp"

namespace {
HINSTANCE instance = nullptr;

struct Effect {
    using Parameters = texturewrap::Parameters;
    using Image = texturewrap::Image;
    using Inputs = texturewrap::Inputs;
    static constexpr auto magic = texturewrap::magic;
    static constexpr auto info = texturewrap::plugin_info;
    static constexpr auto title = texturewrap::title;

    static bool valid(const Parameters& p) { return texturewrap::valid(p); }
    static Parameters defaults(FilterRecord*) { return texturewrap::defaults(texturewrap::effect_shape); }
    static void descriptor_defaults(Parameters& p) {
        const auto factory = defaults(nullptr);
        p.render_sides = factory.render_sides;
        p.wrap_angle = factory.wrap_angle;
        p.decal_longitude = factory.decal_longitude;
        p.decal_latitude = factory.decal_latitude;
        p.decal_size = factory.decal_size;
        p.local_decal = factory.local_decal;
        p.texture_key = factory.texture_key;
        p.keep_current = factory.keep_current;
        p.cylinder_decal_angle = factory.cylinder_decal_angle;
        p.cylinder_decal_position = factory.cylinder_decal_position;
        p.cylinder_decal_height = factory.cylinder_decal_height;
        p.keep_current_for_self_decal = factory.keep_current_for_self_decal;
    }
    static bool upgrade(const void* bytes, int32 size, Parameters& p) {
        constexpr auto values_offset = offsetof(psfilter::Runtime<Effect>::Saved, values);
        constexpr auto v01_payload = offsetof(Parameters, render_sides);
        constexpr auto v02_payload = offsetof(Parameters, wrap_angle);
        constexpr auto v03_payload = offsetof(Parameters, texture_key);
        constexpr auto v04_payload = offsetof(Parameters, keep_current);
        constexpr auto v041_payload = offsetof(Parameters, cylinder_decal_angle);
        constexpr auto cylinder_v05_payload = offsetof(Parameters, keep_current_for_self_decal);
        static_assert(values_offset == 8 && v01_payload == 104 && v02_payload == 112 &&
                      v03_payload == 152 && v04_payload == 160 && v041_payload == 168 &&
                      cylinder_v05_payload == 192);
        if (size != int32(values_offset + v01_payload) && size != int32(values_offset + v02_payload) &&
            size != int32(values_offset + v03_payload) && size != int32(values_offset + v04_payload) &&
            size != int32(values_offset + v041_payload) && size != int32(values_offset + cylinder_v05_payload)) return false;
        std::uint32_t identity = 0;
        std::memcpy(&identity, bytes, sizeof(identity));
        if (identity != magic) return false;
        auto restored = defaults(nullptr);
        std::memcpy(&restored, static_cast<const std::byte*>(bytes) + values_offset, std::size_t(size) - values_offset);
        p = restored;
        return true;
    }
    static Image from_host(psui::LayerPixels&& p) { return texturewrap::from_host(std::move(p)); }

    static OSErr read_inputs(FilterRecord* host, const std::vector<psui::HostLayer>& layers,
                            const Parameters& p, Inputs& inputs) {
        return texturewrap::load_texture_source(host, layers, p.texture_key, inputs);
    }

    static INT_PTR dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source,
                          const std::vector<psui::HostLayer>& layers, Parameters& p, Inputs& inputs) {
        return texturewrap::show_dialog(module, owner, host, source, layers, p, inputs);
    }

    static bool process(const Image& source, const Parameters& p, Image& out, const Inputs& inputs,
                        const texturewrap::Cancel& cancel) {
        auto map = inputs.mapping();
        const auto& texture = map.reference ? *map.reference : source;
        if (!inputs.texture && !texturewrap::find_texture_bounds(texture, map.texture, cancel)) return false;
        return texturewrap::render(texturewrap::effect_shape, source, p, map, out, cancel);
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
