#pragma once
#include "preview_background.hpp"
namespace psui {
// Bounded native-depth target/selection snapshot. Host calls stay on the UI thread.
struct NativePreview {
 std::vector<float> rgba,selection;OSErr source_error=noErr;bool selection_approximate=false;
 void read(FilterRecord* f,const ProxyImage& geometry){
  const auto n=std::size_t(geometry.width)*geometry.height;rgba.assign(n*4,0);selection.clear();source_error=noErr;selection_approximate=false;
  if(!f->documentInfo){source_error=filterBadParameters;return;}
  auto* rgb=f->documentInfo->targetCompositeChannels;
  for(int k=0;k<4;++k){auto* channel=k==3?f->documentInfo->targetTransparency:rgb;
   if(k<3&&rgb)rgb=rgb->next;
   if(!channel){if(k==3)for(std::size_t i=0;i<n;++i)rgba[i*4+3]=1;else source_error=filterBadMode;continue;}
   std::vector<float> plane;const auto error=host_read_preview_channel(f,channel,geometry,plane);
   if(error){source_error=error;continue;}for(std::size_t i=0;i<n;++i)rgba[i*4+k]=plane[i];
  }
  if(f->haveMask){auto* mask=f->documentInfo->selection;if(!mask||host_read_preview_channel(f,mask,geometry,selection)){selection.clear();selection_approximate=true;}}
 }
};
}
