#pragma once
#include "host_layers.hpp"
#include "preview_view.hpp"
namespace psui {
inline bool has_pixels(VRect r){return r.right>r.left&&r.bottom>r.top;}
inline VRect union_bounds(VRect a,VRect b){
 if(!has_pixels(b))return a;if(!has_pixels(a))return b;
 return {std::min(a.top,b.top),std::min(a.left,b.left),std::max(a.bottom,b.bottom),std::max(a.right,b.right)};
}
// The document rectangle is the canvas, not necessarily the raster layer.
// A normal layer may retain pixels beyond any canvas edge. Read those pixels
// before the host starts writing output tiles, including when a selection is
// inside the canvas and neighbouring samples come from outside it.
inline VRect target_read_bounds(const FilterRecord* f){
 VRect area{};if(!f)return area;
 if(f->documentInfo){
  area=f->documentInfo->bounds;
  auto* channel=f->documentInfo->targetCompositeChannels;
  for(int i=0;i<3&&channel;i++,channel=channel->next)area=union_bounds(area,channel->bounds);
  if(f->documentInfo->targetTransparency)area=union_bounds(area,f->documentInfo->targetTransparency->bounds);
 }
 return union_bounds(area,filter_document_bounds(f));
}
// Reference layers keep their own document origin and bounds. In particular,
// the canvas and the current selection do not clip an external texture.
inline VRect reference_layer_bounds(const HostLayer& layer){
 VRect bounds{};
 for(const auto* channel:layer.rgb)if(channel)bounds=union_bounds(bounds,channel->bounds);
 if(layer.alpha)bounds=union_bounds(bounds,layer.alpha->bounds);
 return bounds;
}
inline OSErr host_read_reference(FilterRecord* f,const HostLayer& layer,LayerPixels& pixels){
 const auto bounds=reference_layer_bounds(layer);
 return host_read_layer(f,layer,has_pixels(bounds)?bounds:target_read_bounds(f),0,pixels);
}
}
