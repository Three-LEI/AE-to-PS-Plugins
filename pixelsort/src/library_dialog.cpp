#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "pixelsort_host.hpp"
#include "psui/raster_filter_dialog.hpp"

namespace pixelsort {
namespace {

bool draw(psui::LibraryPanel& ui, Parameters& p) {
    constexpr const char* directions[] = {"水平 / Horizontal", "垂直 / Vertical"};
    constexpr const char* orders[] = {"暗到亮 / Dark to Light", "亮到暗 / Light to Dark"};
    bool changed = ui.choice("条带方向 / Stripe Direction", p.direction, directions, 2);
    changed |= ui.choice("排序顺序 / Sort Order", p.order, orders, 2);
    if (ui.number("亮度下限（%） / Lower Luminance (%)", p.low, 0, 100, 0, 100)) {
        p.high = std::max(p.low, p.high);
        changed = true;
    }
    if (ui.number("亮度上限（%） / Upper Luminance (%)", p.high, 0, 100, 0, 100)) {
        p.low = std::min(p.low, p.high);
        changed = true;
    }
    changed |= ui.number("最大条带长度（像素） / Max Stripe Length (px)", p.length, 1, 8192, 1, 512, true, true);
    changed |= ui.number("效果混合（%） / Effect Mix (%)", p.mix, 0, 100, 0, 100);
    return changed;
}

} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source, Parameters& p) {
    constexpr const char* glyphs = "像素排序 条带方向 水平垂直 顺序 暗到亮 亮到暗 亮度下限上限 最大条带长度像素 效果混合 预览失败请重置或取消 确定全部重置";
    return psui::run_raster_filter_dialog(module, owner, host, title, plugin_info, glyphs, source, p, draw, render_region);
}

} // namespace pixelsort
