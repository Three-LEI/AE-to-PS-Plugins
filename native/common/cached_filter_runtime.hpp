#pragma once
#include "filter_runtime.hpp"
#include "psui/host_layers.hpp"
#include <cmath>
#include <memory>
namespace psfilter {
// Reusable host lifecycle for spatial effects that snapshot native RGBA once,
// then process a complete image and deliver non-overlapping output tiles.
template<class Effect> struct CachedRuntime {
 using Store=Runtime<Effect>;
 struct Session {typename Store::Saved saved;typename Effect::Image output;};
 static int16 report(FilterRecord* f,const char* text){
  if(f->errorString){const auto n=std::min<std::size_t>(255,std::strlen(text));(*f->errorString)[0]=std::uint8_t(n);std::memcpy(*f->errorString+1,text,n);}return errReportString;
 }
 static void request(FilterRecord* f,VRect r){
  if(f->bigDocumentData){f->bigDocumentData->inRect32={};f->bigDocumentData->outRect32=r;f->bigDocumentData->maskRect32={};}
  else{f->inRect={};f->outRect={int16(r.top),int16(r.left),int16(r.bottom),int16(r.right)};f->maskRect={};}
  f->outLoPlane=0;f->outHiPlane=f->outTransparencyMask&&
   (f->filterCase==filterCaseEditableTransparencyNoSelection||f->filterCase==filterCaseEditableTransparencyWithSelection)?3:2;
 }
 static OSErr read_named(FilterRecord* f,const std::vector<psui::HostLayer>& layers,std::uint64_t key,typename Effect::Image& image){
  image={};if(!key)return noErr;const psui::HostLayer* found=nullptr;
  for(const auto& layer:layers)if(layer.name_key==key){if(found)return report(f,"Multiple layers have the saved input name. Reopen the filter and choose its input again.");found=&layer;}
  if(!found)return report(f,"The saved input layer is missing. Reopen the filter and choose its input again.");
  psui::LayerPixels pixels;const auto error=psui::host_read_layer(f,*found,f->documentInfo->bounds,0,pixels);
  if(!error)image=Effect::from_host(std::move(pixels));return error;
 }
 static int16 start(HINSTANCE module,FilterRecord* f,intptr_t* data){
  bool interactive=*data==1;*data=0;
  if((f->depth!=8&&f->depth!=16&&f->depth!=32)||f->planes<3||
   (f->imageMode!=plugInModeRGBColor&&f->imageMode!=plugInModeRGB48&&f->imageMode!=plugInModeRGB96))return filterBadMode;
  if(!f->documentInfo||!f->channelPortProcs||!f->channelPortProcs->readPixelsProc)
   return report(f,"Photoshop did not expose readable RGB target channels. Select an RGB pixel layer.");
  psui::HostLayer target;auto* channel=f->documentInfo->targetCompositeChannels;
  for(int i=0;i<3&&channel;++i,channel=channel->next)target.rgb[i]=channel;
  target.alpha=f->documentInfo->targetTransparency;if(!target.rgb[2])return report(f,"Select a readable RGB pixel layer before opening this filter.");
  auto session=std::make_unique<Session>();session->saved.values=Effect::defaults(f);
  if(!Store::read_parameters(f,session->saved))return filterBadParameters;
  auto input_bounds=f->documentInfo->bounds;
  if constexpr(requires{Effect::input_bounds(f);})input_bounds=Effect::input_bounds(f);
  psui::LayerPixels pixels;auto error=psui::host_read_layer(f,target,input_bounds,0,pixels);if(error)return error;
  auto source=Effect::from_host(std::move(pixels));auto layers=psui::host_layers(f);typename Effect::Inputs inputs;
  if constexpr(requires{Effect::read_descriptor(f,layers,session->saved.values,inputs,interactive);})
   if((error=Effect::read_descriptor(f,layers,session->saved.values,inputs,interactive)))return error;
  if(interactive){
   HWND owner=f->platformData?reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(f->platformData)->hwnd):nullptr;
   const auto answer=Effect::dialog(module,owner,f,source,layers,session->saved.values,inputs);
   if(answer!=IDOK)return answer<0?int16(answer):userCanceledErr;
  }else if((error=Effect::read_inputs(f,layers,session->saved.values,inputs)))return error;
  if(f->progressProc)f->progressProc(0,100);
  auto cancel=[&]{return f->abortProc&&f->abortProc();};
  if(!Effect::process(source,session->saved.values,session->output,inputs,cancel))return cancel()?userCanceledErr:filterBadParameters;
  if constexpr(requires{Effect::write_descriptor(f,session->saved.values,inputs);})
   if((error=Effect::write_descriptor(f,session->saved.values,inputs)))return error;
  if(!Store::save_parameters(f,session->saved))return memFullErr;
  f->autoMask=true;f->wantLayout=0;f->inputRate=f->maskRate=65536;
  const auto all=Store::full_rect(f);request(f,all.right>all.left&&all.bottom>all.top?Store::tile(all.left,all.top,all):VRect{});
  if(f->progressProc)f->progressProc(90,100);
  *data=reinterpret_cast<intptr_t>(session.release());return noErr;
 }
 static int16 write_tile(FilterRecord* f,Session& session){
  const auto r=f->bigDocumentData?f->bigDocumentData->outRect32:VRect{f->outRect.top,f->outRect.left,f->outRect.bottom,f->outRect.right};
  const int w=r.right-r.left,h=r.bottom-r.top,n=f->outHiPlane+1,bytes=f->depth/8;
  if(w<=0||h<=0||!f->outData||f->outRowBytes<w*n*bytes||
   (f->outColumnBytes&&f->outColumnBytes!=n*bytes)||(f->outPlaneBytes&&f->outPlaneBytes!=bytes))return filterBadParameters;
  const auto& out=session.output;
  if(r.left<out.left||r.top<out.top||r.right>out.left+out.width||r.bottom>out.top+out.height)return filterBadParameters;
  const bool flat=f->filterCase==filterCaseFlatImageNoSelection||f->filterCase==filterCaseFlatImageWithSelection;
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){const auto i=(std::size_t(r.top+y-out.top)*out.width+r.left+x-out.left)*4;
   for(int k=0;k<n;++k){const float value=out.rgba[i+k]*(flat&&k<3?out.rgba[i+3]:1);
    auto* dest=static_cast<std::uint8_t*>(f->outData)+std::size_t(y)*f->outRowBytes+(x*n+k)*bytes;
    if(bytes==1)*dest=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));
    else if(bytes==2){const auto v=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(dest,&v,2);}
    else std::memcpy(dest,&value,4);
   }
  }
  const auto all=Store::full_rect(f);int x=r.right,y=r.top;if(x>=all.right){x=all.left;y=r.bottom;}
  request(f,y>=all.bottom?VRect{}:Store::tile(x,y,all));return noErr;
 }
 static void entry(HINSTANCE module,int16 selector,FilterRecord* f,intptr_t* data,int16* result){
  if(!result)return;*result=noErr;
  if(selector==filterSelectorAbout){psui::show_host_about(Effect::info,Effect::title);return;}
  if(!f||!data){*result=filterBadParameters;return;}
  if(f->bigDocumentData)f->bigDocumentData->PluginUsing32BitCoordinates=true;
  try{switch(selector){
   case filterSelectorParameters:*data=1;break;
   case filterSelectorPrepare:f->bufferSpace=0;f->bufferSpace64=0;break;
   case filterSelectorStart:*result=start(module,f,data);break;
   case filterSelectorContinue:
    if(*data<=1)*result=filterBadParameters;
    else if(f->abortProc&&f->abortProc())*result=userCanceledErr;
    else *result=write_tile(f,*reinterpret_cast<Session*>(*data));break;
   case filterSelectorFinish:if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;request(f,{});break;
   default:*result=filterBadParameters;break;
  }}catch(const std::bad_alloc&){*result=memFullErr;}catch(...){*result=filterBadParameters;}
  if(*result&&selector!=filterSelectorParameters&&selector!=filterSelectorPrepare){if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;request(f,{});}
 }
};
}
