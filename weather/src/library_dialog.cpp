#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "weather_host.hpp"
#include "psui/generator_controls.hpp"
#include "psui/raster_filter_dialog.hpp"

namespace weather {
namespace {

bool draw(psui::LibraryPanel& ui, FilterRecord* host, Parameters& p) {
    constexpr const char* modes[] = {"雨 / Rain", "雪 / Snow"};
    constexpr const char* outputs[] = {"与原图合成 / Composite", "仅粒子 / Particles Only"};
    bool changed = ui.choice("天气 / Weather", p.mode, modes, 2);
    changed |= ui.number("密度（%） / Density (%)", p.density, 0, 100, 0, 100);
    changed |= ui.number("不透明度（%） / Opacity (%)", p.opacity, 0, 100, 0, 100);
    if (p.mode == Rain) changed |= psui::color_control(ui, host, "雨线颜色 / Rain Color", p.rain_color);
    else changed |= psui::color_control(ui, host, "雪花颜色 / Snow Color", p.snow_color);
    changed |= ui.number("风向（度） / Wind Angle (deg)", p.wind, -90, 90, -90, 90);
    if (p.mode == Rain) {
        changed |= ui.number("雨线长度（像素） / Streak Length (px)", p.rain_length, 1, 400, 1, 100);
        changed |= ui.number("雨线宽度（像素） / Streak Width (px)", p.rain_width, .25, 14, .25, 6);
    } else {
        changed |= ui.number("雪花大小（像素） / Flake Size (px)", p.snow_size, .5, 40, .5, 20);
    }
    if (ui.section("虚化 / Defocus", true)) {
        changed |= ui.number("近景虚化（像素） / Near Defocus (px)", p.near_blur, 0, 24, 0, 12);
        changed |= ui.number("远景虚化（像素） / Far Defocus (px)", p.far_blur, 0, 24, 0, 12);
    }
    ui.inherit_number_default("随机种子 / Seed", p.seed);
    double seed = p.seed;
    if (ui.number("随机种子 / Seed", seed, 0, 999999, 0, 999, true, true)) {
        p.seed = int(seed);
        changed = true;
    }
    changed |= ui.choice("输出 / Output", p.output, outputs, 2);
    return changed;
}

} // namespace

INT_PTR show_dialog(HINSTANCE module, HWND owner, FilterRecord* host, const Image& source, Parameters& p) {
    constexpr auto glyphs = "雨雪模拟 天气 密度 不透明度 雨线颜色雪花颜色 风向 度 雨线长度宽度 像素 雪花大小 虚化 近景远景 "
                            "随机种子 输出 与原图合成 仅粒子 预览失败请重置或取消 确定全部重置";
    return psui::run_raster_filter_dialog(module, owner, host, title, plugin_info, glyphs, source, p,
        [host](psui::LibraryPanel& ui, Parameters& values) { return draw(ui, host, values); }, render_region,
        [](Parameters& values, const psui::PreviewView& view) {
            psui::PreviewAdjustments handles;
            const auto bounds = view.bounds;
            handles.angle("风向 / Wind Angle", values.wind, .5 * (double(bounds.left) + bounds.right),
                          .5 * (double(bounds.top) + bounds.bottom), -90, 90, 90, -1);
            return handles;
        });
}

} // namespace weather
