#pragma once
#include "photoshop_host.hpp"
#include <array>
namespace psui {
class LibraryPanel;
// Bounded, read-only native channel access for selections and preview masks.
OSErr host_read_preview_channel(FilterRecord*,ReadChannelDesc*,const ProxyImage&,std::vector<float>&);
// View-only state. Never serialized into effect parameters or written to PS.
// The cached background includes ONLY visible layers below the current target.
struct PreviewBackground {
  bool enabled=false;
  enum class Status {ready,unavailable,approximate};
  Status status=Status::ready;
  OSErr error=noErr;
  bool cached=false;
  int width=0,height=0;
  double left=0,top=0,step=1;
  std::vector<float> rgba,coverage;
  PIType target_blend=PIBlendNormal;
  float target_opacity=1;
  // No reads when disabled. Matching geometry reuses the read-only cache.
  void prepare(FilterRecord*,const ProxyImage&);
  // RGBA inputs are unassociated, native-document values normalized to 0..1.
  // Selection is blended before the target's masks/opacity/blend mode.
  std::array<std::uint8_t,3> pixel(int x,int y,
      std::array<float,4> original,std::array<float,4> processed,
      float selection=1) const;
};
// Shared checkbox plus status/tooltip. Returns only a view change.
bool preview_background_control(LibraryPanel&,PreviewBackground&);
}
