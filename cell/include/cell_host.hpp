#pragma once
#include "cell_core.hpp"
#include "psui/host_layers.hpp"
#include "psui/plugin_info.hpp"
inline constexpr psui::PluginInfo cell_info{"Cell Pattern 单元格图案","0.3"};
inline constexpr auto cell_title=L"Cell Pattern 单元格图案";
namespace cell {
inline VRect document_bounds(FilterRecord* f){if(f->documentInfo)return f->documentInfo->bounds;return {0,0,f->bigDocumentData?f->bigDocumentData->imageSize32.v:f->imageSize.v,f->bigDocumentData?f->bigDocumentData->imageSize32.h:f->imageSize.h};}
inline Parameters defaults_for_host(FilterRecord* f){const auto r=document_bounds(f);return defaults(r.right-r.left,r.bottom-r.top,r.left,r.top);}
inline bool locked_alpha(FilterRecord* f){return f->filterCase==filterCaseProtectedTransparencyNoSelection||f->filterCase==filterCaseProtectedTransparencyWithSelection;}
inline OSErr read_source(FilterRecord* f,int max_side,Image& image){
 if(!f->documentInfo||!f->channelPortProcs||!f->channelPortProcs->readPixelsProc)return filterBadMode;
 psui::HostLayer target;auto* channel=f->documentInfo->targetCompositeChannels;
 for(int i=0;i<3&&channel;++i,channel=channel->next)target.rgb[i]=channel;
 target.alpha=f->documentInfo->targetTransparency;if(!target.rgb[2])return filterBadMode;
 psui::LayerPixels pixels;const auto error=psui::host_read_layer(f,target,document_bounds(f),max_side,pixels);
 if(!error)image={pixels.width,pixels.height,pixels.bounds.left,pixels.bounds.top,std::move(pixels.rgba)};return error;
}
inline Source source_view(const Image& image,VRect bounds){
 const double sx=double(bounds.right-bounds.left)/image.width,sy=double(bounds.bottom-bounds.top)/image.height;
 return {&image,bounds.left+(sx-1)*.5,bounds.top+(sy-1)*.5,sx,sy};
}
INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,Parameters&);
}
