#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "channelblur_host.hpp"
#include "psui/raster_filter_dialog.hpp"

namespace channelblur {
namespace {

bool draw(psui::LibraryPanel& ui, Parameters& p) {
    constexpr const char* modes[] = {"RGB 通道 / RGB Channels", "亮度与色度 / Luminance and Chroma"};
    constexpr const char* axes[] = {"水平与垂直 / Both", "仅水平 / Horizontal", "仅垂直 / Vertical"};
    bool changed = ui.choice("模式 / Mode", p.mode, modes, 2);
    changed |= ui.choice("模糊方向 / Blur Direction", p.axis, axes, 3);
    if (ui.section("模糊半径 / Blur Radius", true)) {
        const auto radius = [&](const char* label, double& v) { changed |= ui.number(label, v, 0, 256, 0, 100); };
        if (p.mode == Channels) {
            radius("红（像素） / Red (px)", p.red);
            radius("绿（像素） / Green (px)", p.green);
            radius("蓝（像素） / Blue (px)", p.blue);
        } else {
            radius("亮度（像素） / Luminance (px)", p.luminance);
            radius("色度（像素） / Chroma (px)", p.chroma);
        }
        radius("透明度（像素） / Alpha (px)", p.alpha);
    }
    changed |= ui.number("效果混合（%） / Effect Mix (%)", p.mix, 0, 100, 0, 100);
    return changed;
}

} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source, Parameters& p) {
    constexpr auto glyphs = "通道模糊 模式 亮度与色度 模糊方向 水平与垂直 仅水平仅垂直 "
                            "模糊半径 红绿蓝透明度 像素 效果混合 预览失败请重置或取消 确定全部重置";
    return psui::run_raster_filter_dialog(module, owner, host, title, plugin_info, glyphs, source, p, draw, render_region);
}

} // namespace channelblur
