#pragma once
#include "dither_area.hpp"
namespace dither::preview {
inline RGBA area_pixel(const Image& image,const psui::PreviewSpace& space,double left,double top,double size,bool premultiplied=true){
 return sample_area(image,space.x(left),space.y(top),space.x(left+size),space.y(top+size),premultiplied);
}
struct SourceAreaCache {
 psui::ProxyImage geometry;bool premultiplied=true;std::vector<RGBA> pixels;
 void prepare(const Image& source,const psui::ProxyImage& g,bool premult){
  if(!pixels.empty()&&g.width==geometry.width&&g.height==geometry.height&&g.origin_x==geometry.origin_x&&g.origin_y==geometry.origin_y&&g.step==geometry.step&&premult==premultiplied)return;
  geometry=g;premultiplied=premult;pixels.resize(std::size_t(g.width)*g.height);const psui::PreviewSpace s{double(source.left),double(source.top),1};
  for(int y=0;y<g.height;y++)for(int x=0;x<g.width;x++)pixels[std::size_t(y)*g.width+x]=area_pixel(source,s,g.origin_x+x*g.step,g.origin_y+y*g.step,g.step,premult);
 }
};
// Display-only area filtering keeps 1-bit pixels from aliasing at Fit zoom.
// Before pixels are immutable and cached across parameter changes.
inline bool compose(psui::LibraryPanel& ui,FilterRecord* host,psui::PreviewView& view,psui::ProxyImage& proxy,psui::PreviewBackground& background,std::vector<float>& selection,psui::ProxyImage& selection_geometry,std::vector<std::uint8_t>& display,SourceAreaCache& before_cache,const Image& source,const Image& last,const psui::PreviewSpace& mapping,bool enabled,bool& dirty,OSErr& error){
 if(!dirty&&!view.dirty)return true;const auto roi=view.region();const int step=std::max(1,int(std::ceil(double(std::max(roi.right-roi.left,roi.bottom-roi.top))/view.proxy_side())));
 const int w=std::max(1,(roi.right-roi.left+step-1)/step),h=std::max(1,(roi.bottom-roi.top+step-1)/step);proxy.width=w;proxy.height=h;proxy.origin_x=roi.left;proxy.origin_y=roi.top;proxy.step=step;
 if(host->haveMask&&(selection.empty()||selection_geometry.width!=w||selection_geometry.height!=h||selection_geometry.origin_x!=roi.left||selection_geometry.origin_y!=roi.top||selection_geometry.step!=step)){
  auto* channel=host->documentInfo?host->documentInfo->selection:nullptr;if(channel&&!psui::host_read_preview_channel(host,channel,proxy,selection))selection_geometry=proxy;else selection.clear();
 }
 const bool locked=host->filterCase==filterCaseProtectedTransparencyNoSelection||host->filterCase==filterCaseProtectedTransparencyWithSelection;
 before_cache.prepare(source,proxy,!locked);background.prepare(host,proxy);display.resize(std::size_t(w)*h*3);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  const auto at=std::size_t(y)*w+x;auto before=before_cache.pixels[at],after=enabled&&last.valid()?area_pixel(last,mapping,roi.left+x*step,roi.top+y*step,step,!locked):before;
  if(locked)after[3]=before[3];if(!host->outTransparencyMask){for(int k=0;k<3;k++){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto color=background.pixel(x,y,before,after,selection.empty()?1:selection[at]);for(int k=0;k<3;k++)display[at*3+k]=color[k];
 }
 if(!ui.preview(host,display.data(),w,h)){error=memFullErr;return false;}dirty=view.dirty=false;return true;
}
}
