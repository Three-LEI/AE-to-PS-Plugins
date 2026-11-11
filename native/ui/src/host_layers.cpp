#include "psui/host_layers.hpp"
#include "PIProperties.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
namespace psui {
std::vector<HostLayer> host_layers(FilterRecord* f){
  std::vector<HostLayer> result;
  if(!f||!f->documentInfo||
     f->documentInfo->minVersion>kCurrentMaxVersReadImageDocDesc||
     !f->channelPortProcs||!f->channelPortProcs->readPixelsProc)return result;
  auto* target=f->documentInfo->targetCompositeChannels;
  intptr_t target_index=-1;
  if(f->propertyProcs&&f->propertyProcs->getPropertyProc)
    if(f->propertyProcs->getPropertyProc(0x3842494d,propTargetLayerIndex,0,&target_index,nullptr)!=noErr)target_index=-1;
  int count=0;
  for(auto* l=f->documentInfo->maxVersion>=2?f->documentInfo->layersDescriptor:nullptr;l&&count++<16384;l=l->next){
    if(l->minVersion>kCurrentMaxVersReadLayerDesc)continue;
    HostLayer item;item.id=l->sheetID;item.alpha=l->transparency;
    auto* ch=l->compositeChannelsList;
    for(int i=0;i<3&&ch;++i,ch=ch->next)item.rgb[i]=ch;
    if(!item.rgb[2]||!item.rgb[0]->port||!item.rgb[1]->port||!item.rgb[2]->port)continue;
    item.target=(target_index>=0&&intptr_t(item.id)==target_index)||item.rgb[0]->target || (target&&item.rgb[0]->port==target->port);
    if(l->maxVersion>=1&&l->unicodeName){
      int n=0;while(n<2048&&l->unicodeName[n])++n;
      const auto* wide=reinterpret_cast<const wchar_t*>(l->unicodeName);
      const int bytes=WideCharToMultiByte(CP_UTF8,0,wide,n,nullptr,0,nullptr,nullptr);
      item.name.resize(bytes);WideCharToMultiByte(CP_UTF8,0,wide,n,item.name.data(),bytes,nullptr,nullptr);
    }else if(l->name){
      const int n=MultiByteToWideChar(CP_ACP,0,l->name,-1,nullptr,0);
      std::wstring wide(n,L'\0');MultiByteToWideChar(CP_ACP,0,l->name,-1,wide.data(),n);
      const int bytes=WideCharToMultiByte(CP_UTF8,0,wide.data(),n? n-1:0,nullptr,0,nullptr,nullptr);
      item.name.resize(bytes);WideCharToMultiByte(CP_UTF8,0,wide.data(),n? n-1:0,item.name.data(),bytes,nullptr,nullptr);
    }
    if(item.name.empty())item.name="Layer "+std::to_string(item.id);
    item.name_key=layer_name_key(item.name);
    result.push_back(std::move(item));
  }
  // Flat/background-only images can have target channels but no layer list.
  // This is a valid readable image, not an absent Channel Ports interface.
  if(result.empty() && (f->documentInfo->maxVersion<2 || !f->documentInfo->layersDescriptor)){
    HostLayer current;current.target=true;current.id=target_index>=0?unsigned32(target_index):0;
    current.name="当前图层 / Current Layer";current.name_key=layer_name_key(current.name);
    current.alpha=f->documentInfo->targetTransparency;
    auto* channel=target;for(int i=0;i<3&&channel;++i,channel=channel->next)current.rgb[i]=channel;
    if(current.rgb[2]&&current.rgb[0]->port&&current.rgb[1]->port&&current.rgb[2]->port)
      result.push_back(std::move(current));
  }
  return result;
}
OSErr host_read_layer(FilterRecord* f,const HostLayer& layer,VRect bounds,int max_side,LayerPixels& output){
  if(!f||!f->channelPortProcs||!f->channelPortProcs->readPixelsProc)return filterBadParameters;
  const auto w=std::int64_t(bounds.right)-bounds.left,h=std::int64_t(bounds.bottom)-bounds.top;
  if(w<=0||h<=0||w>2000000||h>2000000)return filterBadParameters;
  const double ratio=max_side>0?std::min(1.,double(max_side)/std::max(w,h)):1.;
  const int width=std::max(1,int(std::ceil(w*ratio))),height=std::max(1,int(std::ceil(h*ratio)));
  const auto size=std::size_t(width)*height;
  // Two native snapshots plus scratch are bounded; no silent full-resolution downsampling.
  if(size>std::size_t(256)*1024*1024/sizeof(float)/4||width>std::numeric_limits<int32>::max()/128)return memFullErr;
  LayerPixels next;next.bounds=bounds;next.width=width;next.height=height;
  next.depth=layer.rgb[0]->depth;next.rgba.resize(size*4,0);
  PSScaling scaling{bounds,{0,0,height,width}};
  VRect full{0,0,height,width};
  for(int c=0;c<4;++c){
    const auto* channel=c<3?layer.rgb[c]:layer.alpha;
    if(!channel){if(c<3)return filterBadParameters;continue;}
    if(channel->depth!=8&&channel->depth!=16&&channel->depth!=32)return filterBadMode;
    const int bytes=channel->depth/8;
    std::vector<std::uint8_t> plane(size*bytes,0);
    PixelMemoryDesc memory{plane.data(),width*bytes*8,bytes*8,0,channel->depth};
    VRect wrote{};
    const auto error=f->channelPortProcs->readPixelsProc(channel->port,&scaling,&full,&memory,&wrote);
    if(error)return error;
    if(wrote.top<0||wrote.left<0||wrote.bottom>height||wrote.right>width)return filterBadParameters;
    for(int y=wrote.top;y<wrote.bottom;++y)for(int x=wrote.left;x<wrote.right;++x){
      const auto i=std::size_t(y)*width+x;
      double value=0;
      if(bytes==1)value=plane[i]/255.;
      else if(bytes==2){std::uint16_t v;std::memcpy(&v,plane.data()+i*2,2);value=v/32768.;}
      else{float v;std::memcpy(&v,plane.data()+i*4,4);value=std::isfinite(v)?v:0;}
      next.rgba[i*4+c]=float(c==3?std::clamp(value,0.,1.):value);
      if(c==0&&!layer.alpha)next.rgba[i*4+3]=1;
    }
    if(f->abortProc&&f->abortProc())return userCanceledErr;
  }
  output=std::move(next);return noErr;
}
}
