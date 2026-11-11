#pragma once
#include "photoshop_host.hpp"
#include <algorithm>
#include <cmath>
namespace psui {
inline VRect filter_bounds(const FilterRecord *f) {
  return f->bigDocumentData ? f->bigDocumentData->filterRect32
                            : VRect{f->filterRect.top, f->filterRect.left,
                                    f->filterRect.bottom, f->filterRect.right};
}
// View-only state. Percent is actual screen pixels per document pixel.
// Never store it in filter parameters or use it to change final rendering.
struct PreviewView {
  VRect bounds{};
  double percent = 100;
  double pan_x = 0, pan_y = 0;
  float width = 0, height = 0;
  bool fitted = true, dirty = true;
  double limit_x() const { return std::max(0., (double(bounds.right)-bounds.left-width/(percent/100))*.5); }
  double limit_y() const { return std::max(0., (double(bounds.bottom)-bounds.top-height/(percent/100))*.5); }
  void constrain() { pan_x=std::clamp(pan_x,-limit_x(),limit_x());pan_y=std::clamp(pan_y,-limit_y(),limit_y()); }
  bool pan(double screen_dx,double screen_dy) {
    if(!std::isfinite(screen_dx)||!std::isfinite(screen_dy))return false;
    const auto x=pan_x,y=pan_y;
    pan_x-=screen_dx/(percent/100);pan_y-=screen_dy/(percent/100);constrain();
    if(x==pan_x&&y==pan_y)return false;
    fitted=false;dirty=true;return true;
  }
  void recenter() {pan_x=pan_y=0;dirty=true;}
  bool wheel(double delta,double local_x,double local_y) {
    if(!std::isfinite(delta)||!std::isfinite(local_x)||!std::isfinite(local_y)||delta==0)return false;
    const double old_zoom=percent/100;
    const double target=std::clamp(percent*std::pow(1.2,std::clamp(delta,-8.,8.)),
                                  std::min(.01,fit_percent()),std::max(800.,fit_percent()));
    if(target==percent)return false;
    const double anchor_x=center_x()+(local_x-width*.5)/old_zoom;
    const double anchor_y=center_y()+(local_y-height*.5)/old_zoom;
    zoom(target);
    pan_x=anchor_x-(local_x-width*.5)/(percent/100)-(double(bounds.left)+bounds.right)*.5;
    pan_y=anchor_y-(local_y-height*.5)/(percent/100)-(double(bounds.top)+bounds.bottom)*.5;
    constrain();return true;
  }
  double fit_percent() const {
    return 100 *
           std::min(width / std::max(1.0, double(bounds.right - bounds.left)),
                    height / std::max(1.0, double(bounds.bottom - bounds.top)));
  }
  void configure(VRect area, float w, float h) {
    if (bounds.left == area.left && bounds.top == area.top &&
        bounds.right == area.right && bounds.bottom == area.bottom &&
        width == w && height == h)
      return;
    bounds = area;
    width = w;
    height = h;
    dirty = true;
    if (fitted)
      percent = fit_percent();
    constrain();
  }
  void fit() {
    fitted = true;
    percent = fit_percent();
    pan_x=pan_y=0;
    dirty = true;
  }
  void zoom(double value) {
    if (!std::isfinite(value))
      return;
    fitted = false;
    percent = std::clamp(value, std::min(.01, fit_percent()),
                         std::max(800.0, fit_percent()));
    constrain();
    dirty = true;
  }
  double center_x() const { return (double(bounds.left) + bounds.right) * .5 + pan_x; }
  double center_y() const { return (double(bounds.top) + bounds.bottom) * .5 + pan_y; }
  VRect region() const {
    const double z = std::max(.000001, percent / 100);
    return {
        std::max(bounds.top,
                 static_cast<int32>(std::floor(center_y() - height / (2 * z)))),
        std::max(bounds.left,
                 static_cast<int32>(std::floor(center_x() - width / (2 * z)))),
        std::min(bounds.bottom,
                 static_cast<int32>(std::ceil(center_y() + height / (2 * z)))),
        std::min(bounds.right,
                 static_cast<int32>(std::ceil(center_x() + width / (2 * z))))};
  }
  int proxy_side() const {
    return std::clamp(int(std::ceil(2 * std::max(width, height))), 16, 512);
  }
};
} // namespace psui
