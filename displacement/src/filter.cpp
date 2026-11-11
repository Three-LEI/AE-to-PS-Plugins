#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/filter_runtime.hpp"
#include "displacement_host.hpp"
#include "displacement_info.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#ifdef ZAYU_PORT_PROBE
#include <fstream>
#include <filesystem>
#endif
namespace {
HINSTANCE instance=nullptr;
struct Metadata {
 using Parameters=displacement::Parameters;
 static constexpr unsigned long magic=0x5A445031;
 static constexpr auto info=displacement::plugin_info;
 static constexpr auto title=L"Displacement Map 置换图";
 static bool valid(const Parameters& p){return displacement::valid(p);}
 static bool upgrade(const void* memory,int32 size,Parameters& p){
  struct PreviousParameters{std::uint32_t layer_id;displacement::Channel horizontal,vertical;double amount_x,amount_y;displacement::Placement placement;bool wrap;};
  struct PreviousSaved{unsigned long magic;PreviousParameters p;};
  if(size!=sizeof(PreviousSaved))return false;PreviousSaved old;std::memcpy(&old,memory,sizeof(old));
  if(old.magic!=magic)return false;
  p=Parameters{};p.horizontal=old.p.horizontal;p.vertical=old.p.vertical;p.amount_x=old.p.amount_x;p.amount_y=old.p.amount_y;p.placement=old.p.placement;p.wrap=old.p.wrap;
  return valid(p);
 }
};
using Store=psfilter::Runtime<Metadata>;
struct Session {Store::Saved saved;displacement::Image source,map;std::vector<float> tile;};
void error_message(FilterRecord* f,const char* message){
 if(f->errorString){const auto n=std::min<std::size_t>(255,std::strlen(message));(*f->errorString)[0]=std::uint8_t(n);std::memcpy(*f->errorString+1,message,n);}
}
void request(FilterRecord* f,VRect r){
 if(f->bigDocumentData){f->bigDocumentData->inRect32={};f->bigDocumentData->outRect32=r;f->bigDocumentData->maskRect32={};}
 else{f->inRect={};f->outRect={int16(r.top),int16(r.left),int16(r.bottom),int16(r.right)};f->maskRect={};}
 f->outLoPlane=0;f->outHiPlane=(f->outTransparencyMask&&
  (f->filterCase==filterCaseEditableTransparencyNoSelection||f->filterCase==filterCaseEditableTransparencyWithSelection))?3:2;
}
VRect tile_at(int x,int y,VRect all){return {y,x,y+std::min(64,all.bottom-y),x+std::min(512,all.right-x)};}
int16 start(FilterRecord* f,intptr_t* data){
 const bool interactive=*data==1;*data=0;
 if((f->depth!=8&&f->depth!=16&&f->depth!=32)||f->planes<3||
   (f->imageMode!=plugInModeRGBColor&&f->imageMode!=plugInModeRGB48&&f->imageMode!=plugInModeRGB96))return filterBadMode;
 auto layers=psui::host_layers(f);
 if(layers.empty()){
  error_message(f,!f->documentInfo?"Photoshop did not provide image document information.":
    (!f->channelPortProcs||!f->channelPortProcs->readPixelsProc)?"Photoshop did not provide the pixel reading interface.":
    "No readable RGB source was provided. Select an RGB pixel layer and reopen the filter.");
  return errReportString;
 }
 // Photoshop may provide distinct port handles for the same target pixels in
 // layersDescriptor and targetCompositeChannels. The latter is authoritative.
 psui::HostLayer target;auto* target_channel=f->documentInfo->targetCompositeChannels;
 for(int i=0;i<3&&target_channel;++i,target_channel=target_channel->next)target.rgb[i]=target_channel;
 target.alpha=f->documentInfo->targetTransparency;
 if(!target.rgb[2]){error_message(f,"Select an RGB layer before opening Displacement Map.");return errReportString;}
 auto session=std::make_unique<Session>();
 if(!Store::read_parameters(f,session->saved))return memFullErr;
 auto& p=session->saved.values;
 // SDK sheetID is a per-invocation layer index in Photoshop 27.7, not the
 // persistent Action Manager layer ID. Validate a saved reference by its name.
 auto map=layers.end();
 if(p.layer_name_key){
  int matches=0;for(auto it=layers.begin();it!=layers.end();++it)if(it->name_key==p.layer_name_key){map=it;++matches;}
  if(matches>1){
   if(!interactive){error_message(f,"Multiple layers have the saved map name. Reopen the filter and choose a layer.");return errReportString;}
   map=std::find_if(layers.begin(),layers.end(),[&](const auto& l){return l.id==p.layer_id&&l.name_key==p.layer_name_key;});
  }
 }else if(p.layer_id!=0xFFFFFFFFu)map=std::find_if(layers.begin(),layers.end(),[&](const auto& l){return l.id==p.layer_id;});
 if(map==layers.end()){
  if((p.layer_name_key||p.layer_id!=0xFFFFFFFFu)&&!interactive){error_message(f,"The saved displacement layer is missing. Reopen the filter and choose a map layer.");return errReportString;}
  map=std::find_if(layers.begin(),layers.end(),[](const auto& l){return !l.target;});
  if(map==layers.end())map=layers.begin();p.layer_id=map->id;
 }
 p.layer_id=map->id;p.layer_name_key=map->name_key;
 psui::LayerPixels source;auto error=psui::host_read_layer(f,target,f->documentInfo->bounds,0,source);
 if(error)return error;
 session->source=displacement::from_host(std::move(source));
 if(interactive){
  HWND parent=f->platformData?reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(f->platformData)->hwnd):nullptr;
  const auto answer=displacement::show_dialog(instance,parent,f,layers,session->source,p,session->map);
  if(answer!=IDOK)return answer==-1?filterBadParameters:userCanceledErr;
 }else{
  psui::LayerPixels pixels;error=psui::host_read_layer(f,*map,displacement::layer_bounds(*map),0,pixels);
  if(error)return error;session->map=displacement::from_host(std::move(pixels));
 }
 if(!Store::save_parameters(f,session->saved))return memFullErr;
 f->autoMask=true;f->wantLayout=0;f->inputRate=f->maskRate=65536;
 const auto all=Store::full_rect(f);
 request(f,all.right>all.left&&all.bottom>all.top?tile_at(all.left,all.top,all):VRect{});
 *data=reinterpret_cast<intptr_t>(session.release());return noErr;
}
int16 render(FilterRecord* f,Session& s){
 const auto r=f->bigDocumentData?f->bigDocumentData->outRect32:VRect{f->outRect.top,f->outRect.left,f->outRect.bottom,f->outRect.right};
 const int w=r.right-r.left,h=r.bottom-r.top,channels=f->outHiPlane+1,bytes=f->depth/8;
 if(w<=0||h<=0||!f->outData||f->outRowBytes<w*channels*bytes||
   (f->outColumnBytes&&f->outColumnBytes!=channels*bytes)||(f->outPlaneBytes&&f->outPlaneBytes!=bytes))return filterBadParameters;
 s.tile.resize(std::size_t(w)*h*4);
 if(!displacement::render(s.source,s.map,s.saved.values,{double(r.left),double(r.top),1,w,h},s.tile.data()))return filterBadParameters;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)for(int k=0;k<channels;++k){
  const float value=s.tile[(std::size_t(y)*w+x)*4+k];
  auto* out=static_cast<std::uint8_t*>(f->outData)+std::size_t(y)*f->outRowBytes+(x*channels+k)*bytes;
  if(bytes==1)*out=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));
  else if(bytes==2){const auto q=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(out,&q,2);}
  else std::memcpy(out,&value,4);
 }
 const auto all=Store::full_rect(f);int x=r.right,y=r.top;if(x>=all.right){x=all.left;y=r.bottom;}
 if(f->progressProc)f->progressProc(y-all.top,all.bottom-all.top);
 request(f,y>=all.bottom?VRect{}:tile_at(x,y,all));return noErr;
}
}
extern "C" __declspec(dllexport) void PluginMain(const int16 selector,FilterRecord* f,intptr_t* data,int16* result){
 if(!result)return;*result=noErr;
 if(selector==filterSelectorAbout){psui::show_host_about(Metadata::info,Metadata::title);return;}
 if(!f||!data){*result=filterBadParameters;return;}
#ifdef ZAYU_PORT_PROBE
 {std::ofstream log((std::filesystem::temp_directory_path()/"AE-to-PS-displacement-probe.log"),std::ios::app);
 log<<"selector="<<selector<<" doc="<<(f->documentInfo!=nullptr)<<" legacy="<<(f->channelPortProcs!=nullptr)<<" read="<<(f->channelPortProcs&&f->channelPortProcs->readPixelsProc)<<" basic="<<(f->sSPBasic!=nullptr)<<" depth="<<f->depth;
 if(auto* doc=f->documentInfo){log<<" min="<<doc->minVersion<<" max="<<doc->maxVersion<<" target="<<(doc->targetCompositeChannels!=nullptr);if(doc->maxVersion>=2)log<<" layers="<<(doc->layersDescriptor!=nullptr);}
 log<<'\n';}
#endif
 if(f->bigDocumentData)f->bigDocumentData->PluginUsing32BitCoordinates=true;
 try{switch(selector){
 case filterSelectorParameters:*data=1;break;
 case filterSelectorPrepare:f->bufferSpace=0;f->bufferSpace64=0;break;
 case filterSelectorStart:*result=start(f,data);break;
 case filterSelectorContinue:
  if(*data<=1){*result=filterBadParameters;break;}
  if(f->abortProc&&f->abortProc())*result=userCanceledErr;
  else *result=render(f,*reinterpret_cast<Session*>(*data));
  if(*result)request(f,{});break;
 case filterSelectorFinish:if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;request(f,{});break;
 default:*result=filterBadParameters;break;
 }}catch(const std::bad_alloc&){*result=memFullErr;}catch(...){*result=filterBadParameters;}
 if(*result&&selector!=filterSelectorParameters&&selector!=filterSelectorPrepare){
  if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;request(f,{});
 }
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=module;DisableThreadLibraryCalls(module);}return TRUE;}
