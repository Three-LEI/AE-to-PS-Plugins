#pragma once
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
namespace psui {
// RGB before display-profile conversion, kept with the last uploaded texture.
// Sampling follows the same linear texture interpolation, zoom, pan and clip.
// It never reads monitor pixels (which would apply the monitor profile twice).
struct PreviewColorSampler {
  std::vector<std::uint8_t> rgb;
  int width = 0, height = 0;
  ImVec2 lo{}, hi{}, clip_lo{}, clip_hi{};
  bool visible = false;
  void pixels(const std::uint8_t *data, int w, int h) {
    rgb.assign(data, data + std::size_t(w) * h * 3);
    width = w; height = h;
  }
  void geometry(ImVec2 a, ImVec2 b, ImVec2 ca, ImVec2 cb) {
    lo=a; hi=b; clip_lo=ca; clip_hi=cb; visible=true;
  }
  bool sample(float x, float y, double output[3]) const {
    if (!visible || width <= 0 || height <= 0 ||
        rgb.size() != std::size_t(width)*height*3 ||
        !std::isfinite(x) || !std::isfinite(y) ||
        hi.x<=lo.x || hi.y<=lo.y || x<lo.x || x>=hi.x || y<lo.y || y>=hi.y ||
        x<clip_lo.x || x>=clip_hi.x || y<clip_lo.y || y>=clip_hi.y)
      return false;
    const double px=std::clamp(double((x-lo.x)/(hi.x-lo.x))*width-.5,0.,double(width-1));
    const double py=std::clamp(double((y-lo.y)/(hi.y-lo.y))*height-.5,0.,double(height-1));
    const int x0=int(px), y0=int(py), x1=std::min(x0+1,width-1), y1=std::min(y0+1,height-1);
    const double fx=px-x0,fy=py-y0;
    for(int k=0;k<3;++k) {
      const auto at=[&](int sx,int sy){return rgb[(std::size_t(sy)*width+sx)*3+k]/255.;};
      output[k]=(at(x0,y0)*(1-fx)+at(x1,y0)*fx)*(1-fy)+
                (at(x0,y1)*(1-fx)+at(x1,y1)*fx)*fy;
    }
    return true;
  }
};
} // namespace psui
