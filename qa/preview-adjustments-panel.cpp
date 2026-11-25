#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "../native/tests/library_qa.hpp"

int main(int argc, char**) {
    psui::PreviewView view;
    const VRect bounds{-150, -300, 350, 500};
    psui::ProxyImage proxy;
    proxy.origin_x = bounds.left;
    proxy.origin_y = bounds.top;
    proxy.width = 80;
    proxy.height = 50;
    proxy.step = 10;
    const std::vector<std::uint8_t> rgb(proxy.width * proxy.height * 3, 96);
    double x = 100, y = 100, percent_x = 50, percent_y = 50;
    bool uploaded = false, complete = false;
    ImVec4 start{}, pan_start{}, wheel_anchor{};
    constexpr auto primary = "preview.adjustment.0", second = "preview.adjustment.1";
    const auto close_to = [](double actual, double expected, const char* message) {
        libraryqa::require(std::abs(actual - expected) < .015, message);
    };

    psui::PanelTestHook hook;
    hook.software = true;
    hook.initial_scale = argc == 1 ? 1.f : 2.f;
    hook.host_locale = argc == 1 ? "en_US" : "zh_CN";
    hook.force_legacy_dpi = argc != 1;
    hook.before_frame = [&](psui::LibraryPanel& ui) {
        auto& io = ImGui::GetIO();
        io.ClearEventsQueue();
        io.ConfigInputTrickleEventQueue = false;
        io.AddFocusEvent(true);
        const auto frame = ui.frame();
        const float s = ui.scale();
        if (frame == 4) {
            start = ui.items.at(second);
            io.AddMousePosEvent(start.x, start.y);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
        }
        if (frame == 5) io.AddMousePosEvent(start.x + 10 * s, start.y + 6 * s);
        if (frame == 6) {
            io.AddMousePosEvent(start.x + 18 * s, start.y + 12 * s);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        }
        if (frame == 8) {
            pan_start = ui.items.at(primary);
            io.AddMousePosEvent(pan_start.x - 26 * s, pan_start.y - 20 * s);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
        }
        if (frame == 9) io.AddMousePosEvent(pan_start.x, pan_start.y);
        if (frame == 10) io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        if (frame == 13) {
            wheel_anchor = ui.items.at(primary);
            io.AddMousePosEvent(wheel_anchor.x, wheel_anchor.y);
            io.AddMouseWheelEvent(0, 1);
        }
    };
    hook.tick = [&](psui::LibraryPanel& ui) {
        const auto frame = ui.frame();
        const float s = ui.scale();
        if (frame == 3) {
            const auto a = ui.items.at(primary), b = ui.items.at(second);
            libraryqa::require(std::hypot(a.x - b.x, a.y - b.y) > 20 * s, "coincident handles are individually accessible");
        }
        if (frame == 5) libraryqa::require(ui.items.at("preview.adjustments").z == 1, "grab the second coincident handle");
        if (frame == 7 || frame == 11 || frame == 16) {
            close_to(percent_x, 50 + 18 * s / (1.25 * 8), "document-space X includes the last release movement");
            close_to(percent_y, 50 + 12 * s / (1.25 * 5), "document-space Y includes the last release movement");
            libraryqa::require(x == 100 && y == 100, "other coincident point remains independent");
        }
        if (frame == 7) libraryqa::require(view.pan_x == 0 && view.pan_y == 0, "handle drag does not pan");
        if (frame == 11) {
            close_to(view.pan_x, -26 * s / 1.25, "blank-space drag pans horizontally through a handle");
            close_to(view.pan_y, -20 * s / 1.25, "blank-space drag pans vertically through a handle");
        }
        if (frame == 16) {
            const auto marker = ui.items.at(primary);
            close_to(view.percent, 150, "wheel zoom works over a handle");
            close_to(marker.x, wheel_anchor.x, "wheel anchor keeps handle X aligned");
            close_to(marker.y, wheel_anchor.y, "wheel anchor keeps handle Y aligned");
            complete = true;
            ui.close(false);
        }
    };

    psui::set_panel_test_hook(&hook);
    const auto result = psui::run_library_panel(GetModuleHandleW(nullptr), nullptr, nullptr, L"Preview adjustment QA",
        640, 460, [&](psui::LibraryPanel& ui) {
            const auto scale = ui.scale();
            view.configure(bounds, 240 * scale, 180 * scale);
            if (!uploaded) {
                libraryqa::require(ui.preview(nullptr, rgb.data(), proxy.width, proxy.height), "upload the test preview");
                view.zoom(125);
                uploaded = true;
            }
            view.dirty = false;
            psui::PreviewAdjustments handles;
            handles.point("中心 / Center", x, y)
                   .point("范围中心 / Mask Center", percent_x, percent_y, psui::PreviewPointSpace::percent(bounds));
            ui.image_view(view, proxy, handles);
            ui.preview_zoom(view);
        }, "中心范围");
    psui::set_panel_test_hook(nullptr);
    if (result != IDCANCEL || !complete || !hook.error.empty()) {
        std::cerr << "FAIL preview adjustments: " << hook.error << '\n';
        return 1;
    }
    std::cout << "PASS " << (argc == 1 ? "en 100%" : "zh 200% legacy")
              << ": coincident handles, signed bounds, final release, blank-space pan, anchored zoom\n";
}
