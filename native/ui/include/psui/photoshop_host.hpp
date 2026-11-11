#pragma once
#include "psui/widgets.hpp"
#include "PIFilter.h"
#include <cstdint>
#include <string>
#include <vector>
namespace psui {
int host_brightness(FilterRecord* host) noexcept;
// Reads propLocale using host-owned Handle callbacks; releases the returned handle.
std::string host_locale(FilterRecord* host);
// Calls Photoshop's preferred picker. RGB values are normalized 0..1.
OSErr host_pick_rgb(FilterRecord* host,double color[3]);
// Photoshop image-window sample in physical screen coordinates. On failure the
// colour is untouched. This is not a screen/GDI pixel or a foreground-colour edit.
// Low-level only: non-image points may start host modal sampling. The shared
// eyedropper deliberately never calls this service; it reads the visible screen.
OSErr host_sample_rgb(FilterRecord* host,POINT screen,double color[3]);
bool screen_sample_rgb(POINT screen,double color[3]);
// Borrowed Photoshop cursor; never DestroyCursor. Falls back to the OS crosshair.
HCURSOR host_eyedropper_cursor(FilterRecord* host);
struct ProxyImage {
    int width=0,height=0,channels=3;
    double origin_x=0,origin_y=0,step=1;
    std::vector<std::uint8_t> pixels,mask;
};
OSErr host_read_proxy(FilterRecord* host,int max_side,ProxyImage& image);
OSErr host_read_proxy_area(FilterRecord*,int max_side,VRect area,ProxyImage& image);
bool host_display_rgb(FilterRecord* host,HDC dc,const RECT& bounds,const std::uint8_t* rgb,int width,int height);
}
