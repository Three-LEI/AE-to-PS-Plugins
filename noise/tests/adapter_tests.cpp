#include "../../displacement/tests/mock_host.hpp"
#include "noise_host.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {
using Entry = void(*)(int16, FilterRecord*, intptr_t*, int16*);
struct Saved { unsigned long magic = noise::magic; noise::Parameters values; };
int checks = 0;

void require(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void run(Entry entry, int depth, bool modern, bool flat, bool locked, bool legacy_handle = false) {
    constexpr int width = 530, height = 75;
    mock::Host host(width, height, depth);
    if (!modern) host.f.bigDocumentData = nullptr;
    if (flat) {
        host.f.outTransparencyMask = 0;
        host.f.filterCase = filterCaseFlatImageWithSelection;
        host.doc.targetTransparency = nullptr;
    }
    if (locked) host.f.filterCase = filterCaseProtectedTransparencyWithSelection;
    Saved saved;
    saved.values = noise::preset(noise::Preset::FilmAndVHS);
    saved.values.basic = true;
    saved.values.hue = 4;
    saved.values.seed = 82;
    saved.values.phase = 9;
    if (!legacy_handle) {
        saved.values.grain_midpoint = 43;
        saved.values.grain_roughness = 57;
        saved.values.mix = 62;
    }
    constexpr int32 version_01_bytes = 176;
    const auto parameter_bytes = legacy_handle ? version_01_bytes : int32(sizeof(saved));
    host.f.parameters = mock::allocate(parameter_bytes);
    std::memcpy(mock::lock(host.f.parameters, true), &saved, parameter_bytes);

    psui::HostLayer target;
    auto* channel = host.doc.targetCompositeChannels;
    for (int k = 0; k < 3; ++k, channel = channel->next) target.rgb[k] = channel;
    target.alpha = host.doc.targetTransparency;
    psui::LayerPixels pixels;
    require(psui::host_read_layer(&host.f, target, host.doc.bounds, 0, pixels) == noErr, "native source");
    const auto input = noise::from_host(std::move(pixels));
    noise::Image expected;
    require(noise::render(input, saved.values, expected), "full-resolution reference");

    host.big.filterRect32 = {3, 4, 72, 527};
    host.f.filterRect = {3, 4, 72, 527};
    intptr_t state = 0;
    auto call = [&](int selector) {
        int16 result = 999;
        entry(int16(selector), &host.f, &state, &result);
        return result;
    };
    require(call(filterSelectorPrepare) == noErr && call(filterSelectorStart) == noErr && state > 1, "DLL starts");
    require(host.f.handleProcs->getSizeProc(host.f.parameters) == sizeof(Saved), "0.1 or 0.2 handle fits current layout");
    const int channels = flat || locked ? 3 : 4, bytes = depth / 8, row_bytes = width * channels * bytes + 8;
    std::vector<unsigned char> output(std::size_t(row_bytes) * height, 0xCD);
    int tiles = 0;
    while (true) {
        const auto r = modern ? host.big.outRect32
                              : VRect{host.f.outRect.top, host.f.outRect.left, host.f.outRect.bottom, host.f.outRect.right};
        if (r.bottom <= r.top) break;
        require(++tiles <= 4, "bounded tile iteration");
        host.f.outData = output.data() + std::size_t(r.top) * row_bytes + r.left * channels * bytes;
        host.f.outRowBytes = row_bytes;
        host.f.outColumnBytes = channels * bytes;
        host.f.outPlaneBytes = bytes;
        require(call(filterSelectorContinue) == noErr, "tile write");
    }
    require(tiles == 4, "partial tiles");
    for (int y = 0; y < height; ++y) {
        for (int b = 0; b < row_bytes; ++b) {
            const int x = b / (channels * bytes), k = (b / bytes) % channels;
            const auto actual = output[std::size_t(y) * row_bytes + b];
            if (y < 3 || y >= 72 || x < 4 || x >= 527) {
                require(actual == 0xCD, "selection and padding guard");
                continue;
            }
            const auto i = (std::size_t(y) * width + x) * 4;
            const float value = expected.rgba[i + k] * (flat && k < 3 ? expected.rgba[i + 3] : 1);
            unsigned char wanted[4]{};
            if (bytes == 1) wanted[0] = std::uint8_t(std::lround(std::clamp(value, 0.f, 1.f) * 255));
            else if (bytes == 2) {
                const auto encoded = std::uint16_t(std::lround(std::clamp(value, 0.f, 1.f) * 32768));
                std::memcpy(wanted, &encoded, 2);
            } else std::memcpy(wanted, &value, 4);
            require(actual == wanted[b % bytes], "tile matches full output");
        }
    }
    require(call(filterSelectorFinish) == noErr && state == 0, "session released");
    mock::cancel = true;
    require(call(filterSelectorStart) == userCanceledErr && state == 0, "cancel cleanup");
    mock::cancel = false;
    mock::fail_read = true;
    require(call(filterSelectorStart) != noErr && state == 0, "read failure cleanup");
    mock::fail_read = false;
}
}

int main() {
    wchar_t executable[32768];
    GetModuleFileNameW(nullptr, executable, 32768);
    const auto file = std::filesystem::path(executable).parent_path() / noise::binary_name;
    const auto dll = LoadLibraryW(file.c_str());
    require(bool(dll), "actual DLL loads");
    const auto entry = reinterpret_cast<Entry>(GetProcAddress(dll, "PluginMain"));
    require(bool(entry), "PluginMain exported");
    for (int depth : {8, 16, 32}) {
        for (bool modern : {false, true}) {
            for (bool flat : {false, true}) run(entry, depth, modern, flat, false);
        }
        run(entry, depth, true, false, true);
        run(entry, depth, true, false, false, true);
    }
    FreeLibrary(dll);
    std::cout << "PASS: " << checks << " actual DLL assertions across RGB8/16/32, 0.1 handle migration, tiled selection and alpha\n";
}
