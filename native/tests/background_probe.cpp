#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "psui/preview_background.hpp"
#include "psui/host_layers.hpp"
#include "PIProperties.h"
#include <fstream>
#include <filesystem>
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* f,intptr_t*,int16* result){
 *result=noErr;if(selector!=filterSelectorStart)return;
 try{
  std::ofstream log((std::filesystem::temp_directory_path()/"AE-to-PS-background-probe.txt"));
  intptr_t index=-1;if(f->propertyProcs)f->propertyProcs->getPropertyProc(0x3842494d,propTargetLayerIndex,0,&index,nullptr);log<<"target="<<index<<'\n';
  for(auto* l=f->documentInfo->layersDescriptor;l;l=l->next)log<<"layer="<<l->sheetID<<" visible="<<int(l->isVisible)<<" pixel="<<int(l->isPixelBased)<<" adjust="<<int(l->isAdjustor)<<" generator="<<int(l->isContentGenerator)<<" rendered="<<int(l->isRendered)<<" opacity="<<int(l->opacity)<<" fill="<<int(l->fillOpacity)<<" mode="<<std::hex<<l->blendMode<<std::dec<<" rgb="<<(l->compositeChannelsList!=nullptr)<<'\n';
  const auto r=f->documentInfo->bounds;
  psui::ProxyImage geometry;geometry.width=r.right-r.left;geometry.height=r.bottom-r.top;geometry.origin_x=r.left;geometry.origin_y=r.top;
  psui::PreviewBackground bg;bg.enabled=true;bg.prepare(f,geometry);log<<"status="<<int(bg.status)<<" error="<<bg.error<<'\n';
  psui::HostLayer target;auto* c=f->documentInfo->targetCompositeChannels;for(int k=0;k<3&&c;++k,c=c->next)target.rgb[k]=c;target.alpha=f->documentInfo->targetTransparency;
  psui::LayerPixels source;auto error=psui::host_read_layer(f,target,r,0,source);log<<"source="<<error<<'\n';
  if(!error){
   std::ofstream out((std::filesystem::temp_directory_path()/"AE-to-PS-background-preview.ppm"),std::ios::binary);
   out<<"P6\n"<<geometry.width<<' '<<geometry.height<<"\n255\n";
   for(int y=0;y<geometry.height;++y)for(int x=0;x<geometry.width;++x){const auto i=(std::size_t(y)*geometry.width+x)*4;const std::array<float,4> pixel{source.rgba[i],source.rgba[i+1],source.rgba[i+2],source.rgba[i+3]};const auto rgb=bg.pixel(x,y,pixel,pixel);out.write(reinterpret_cast<const char*>(rgb.data()),3);}
  }
 }catch(...){*result=filterBadParameters;return;}
 *result=userCanceledErr;
}
