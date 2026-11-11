#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/filter_runtime.hpp"
#include "glow_host.hpp"
#include <cmath>
#include <memory>
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Metadata {
 using Parameters=glow::Parameters;
 static constexpr unsigned long magic=glow::effect_kind==glow::Kind::deep?0x5A444731:0x5A474C31;
 static bool valid(const Parameters& p){return p.kind==glow::effect_kind&&glow::valid(p);}
};
using Store=psfilter::Runtime<Metadata>;
struct Session {Store::Saved saved;glow::Image output;};
void request(FilterRecord* f,VRect r){
 if(f->bigDocumentData){f->bigDocumentData->inRect32={};f->bigDocumentData->outRect32=r;f->bigDocumentData->maskRect32={};}
 else{f->inRect={};f->outRect={int16(r.top),int16(r.left),int16(r.bottom),int16(r.right)};f->maskRect={};}
 f->outLoPlane=0;f->outHiPlane=f->outTransparencyMask&&
  (f->filterCase==filterCaseEditableTransparencyNoSelection||f->filterCase==filterCaseEditableTransparencyWithSelection)?3:2;
}
int16 message(FilterRecord* f,const char* text){
 if(f->errorString){const auto n=std::min<std::size_t>(255,std::strlen(text));(*f->errorString)[0]=std::uint8_t(n);std::memcpy(*f->errorString+1,text,n);}return errReportString;
}
OSErr read_optional(FilterRecord* f,const std::vector<psui::HostLayer>& layers,std::uint64_t key,glow::Image& image){
 if(!key)return noErr;const psui::HostLayer* found=nullptr;
 for(const auto& l:layers)if(l.name_key==key){if(found)return message(f,"Multiple layers have the saved input name. Reopen Glow and select the mask/texture again.");found=&l;}
 if(!found)return message(f,"A saved glow mask/texture layer is missing. Reopen the filter and choose its input.");
 psui::LayerPixels pixels;const auto error=psui::host_read_layer(f,*found,f->documentInfo->bounds,0,pixels);
 if(!error)image=glow::from_host(std::move(pixels));return error;
}
int16 start(FilterRecord* f,intptr_t* data){
 bool interactive=*data==1;*data=0;
 if((f->depth!=8&&f->depth!=16&&f->depth!=32)||f->planes<3||
  (f->imageMode!=plugInModeRGBColor&&f->imageMode!=plugInModeRGB48&&f->imageMode!=plugInModeRGB96))return filterBadMode;
 if(!f->documentInfo||!f->channelPortProcs||!f->channelPortProcs->readPixelsProc)
  return message(f,"Photoshop did not expose readable RGB target channels. Select an RGB pixel layer.");
 psui::HostLayer target;auto* channel=f->documentInfo->targetCompositeChannels;
 for(int i=0;i<3&&channel;++i,channel=channel->next)target.rgb[i]=channel;
 target.alpha=f->documentInfo->targetTransparency;
 if(!target.rgb[2])return message(f,"Select an RGB pixel layer before opening Glow.");
 auto s=std::make_unique<Session>();s->saved.values=glow::defaults(glow::effect_kind);
 if(!Store::read_parameters(f,s->saved))return filterBadParameters;
 psui::LayerPixels pixels;auto error=psui::host_read_layer(f,target,f->documentInfo->bounds,0,pixels);if(error)return error;
 auto source=glow::from_host(std::move(pixels));glow::Image mask,texture;
 const auto layers=psui::host_layers(f);
 if((error=Store::read_descriptor(f,layers,s->saved.values,interactive)))return error;
 if(interactive){
  HWND parent=f->platformData?reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(f->platformData)->hwnd):nullptr;
  const auto answer=glow::show_dialog(instance,parent,f,source,layers,s->saved.values,mask,texture);
  if(answer!=IDOK)return answer<0?int16(answer):userCanceledErr;
 }else{
  if((error=read_optional(f,layers,s->saved.values.mask_key,mask)))return error;
  if((error=read_optional(f,layers,s->saved.values.texture_key,texture)))return error;
 }
 auto cancel=[&](){return f->abortProc&&f->abortProc();};
 if(f->progressProc)f->progressProc(0,100);
 if(!glow::render(source,s->saved.values,s->output,mask.valid()?&mask:nullptr,texture.valid()?&texture:nullptr,cancel))
  return cancel()?userCanceledErr:filterBadParameters;
 if((error=Store::write_descriptor(f,layers,s->saved.values)))return error;
 if(!Store::save_parameters(f,s->saved))return memFullErr;
 f->autoMask=true;f->wantLayout=0;f->inputRate=f->maskRate=65536;
 const auto all=Store::full_rect(f);request(f,all.right>all.left&&all.bottom>all.top?Store::tile(all.left,all.top,all):VRect{});
 if(f->progressProc)f->progressProc(90,100);
 *data=reinterpret_cast<intptr_t>(s.release());return noErr;
}
int16 write_tile(FilterRecord* f,Session& s){
 const auto r=f->bigDocumentData?f->bigDocumentData->outRect32:VRect{f->outRect.top,f->outRect.left,f->outRect.bottom,f->outRect.right};
 const int w=r.right-r.left,h=r.bottom-r.top,n=f->outHiPlane+1,bytes=f->depth/8;
 if(w<=0||h<=0||!f->outData||f->outRowBytes<w*n*bytes||
  (f->outColumnBytes&&f->outColumnBytes!=n*bytes)||(f->outPlaneBytes&&f->outPlaneBytes!=bytes))return filterBadParameters;
 const auto& out=s.output;
 if(r.left<out.left||r.top<out.top||r.right>out.left+out.width||r.bottom>out.top+out.height)return filterBadParameters;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x)for(int k=0;k<n;++k){
  const auto i=(std::size_t(r.top+y-out.top)*out.width+(r.left+x-out.left))*4+k;const float value=out.rgba[i];
  auto* dest=static_cast<std::uint8_t*>(f->outData)+std::size_t(y)*f->outRowBytes+(x*n+k)*bytes;
  if(bytes==1)*dest=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));
  else if(bytes==2){const auto v=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(dest,&v,2);}
  else std::memcpy(dest,&value,4);
 }
 const auto all=Store::full_rect(f);int x=r.right,y=r.top;if(x>=all.right){x=all.left;y=r.bottom;}
 request(f,y>=all.bottom?VRect{}:Store::tile(x,y,all));return noErr;
}
}
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* f,intptr_t* data,int16* result){
 if(!result)return;*result=noErr;
 if(selector==filterSelectorAbout){psui::show_host_about(glow::plugin_info,glow::title);return;}
 if(!f||!data){*result=filterBadParameters;return;}
 if(f->bigDocumentData)f->bigDocumentData->PluginUsing32BitCoordinates=true;
 try{switch(selector){
 case filterSelectorParameters:*data=1;break;
 case filterSelectorPrepare:f->bufferSpace=0;f->bufferSpace64=0;break;
 case filterSelectorStart:*result=start(f,data);break;
 case filterSelectorContinue:
  if(*data<=1){*result=filterBadParameters;break;}
  if(f->abortProc&&f->abortProc())*result=userCanceledErr;
  else *result=write_tile(f,*reinterpret_cast<Session*>(*data));break;
 case filterSelectorFinish:if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;request(f,{});break;
 default:*result=filterBadParameters;break;
 }}catch(const std::bad_alloc&){*result=memFullErr;}catch(...){*result=filterBadParameters;}
 if(*result&&selector!=filterSelectorParameters&&selector!=filterSelectorPrepare){if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;request(f,{});}
}
BOOL WINAPI DllMain(HINSTANCE m,DWORD r,LPVOID){if(r==DLL_PROCESS_ATTACH){instance=m;DisableThreadLibraryCalls(m);}return TRUE;}
