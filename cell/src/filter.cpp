#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/generator_filter_runtime.hpp"
#include "cell_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=cell::Parameters;using Image=cell::Image;
 static constexpr unsigned long magic=0x5A434531;
 static bool valid(const Parameters& p){return cell::valid(p);}
};
// Procedural modes stay tile-sized. Only source-dependent modes (or protected
// alpha) freeze the target once, before any destination tile can overwrite it.
struct Runtime {
 using Base=psfilter::CachedRuntime<Effect>;using Store=psfilter::Runtime<Effect>;
 struct Session:Base::Session {cell::Image source;};
 static int16 start(FilterRecord* f,intptr_t* data){
  const bool interactive=*data==1;*data=0;if(f->abortProc&&f->abortProc())return userCanceledErr;
  if((f->depth!=8&&f->depth!=16&&f->depth!=32)||f->planes<3||(f->imageMode!=plugInModeRGBColor&&f->imageMode!=plugInModeRGB48&&f->imageMode!=plugInModeRGB96))return filterBadMode;
  auto s=std::make_unique<Session>();s->saved.values=cell::defaults_for_host(f);
  if(!Store::read_parameters(f,s->saved))return filterBadParameters;
  if(interactive){HWND owner=f->platformData?reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(f->platformData)->hwnd):nullptr;
   const auto answer=cell::show_dialog(instance,owner,f,s->saved.values);if(answer!=IDOK)return answer<0?int16(answer):userCanceledErr;}
  if(!Effect::valid(s->saved.values))return filterBadParameters;
  if(cell::samples_source(s->saved.values.mode)||cell::locked_alpha(f)){
   const auto error=cell::read_source(f,0,s->source);if(error)return error;
  }
  if(!Store::save_parameters(f,s->saved))return memFullErr;
  f->autoMask=true;f->wantLayout=0;f->inputRate=f->maskRate=65536;
  const auto r=Store::full_rect(f);Base::request(f,r.right>r.left&&r.bottom>r.top?Store::tile(r.left,r.top,r):VRect{});
  *data=reinterpret_cast<intptr_t>(s.release());return noErr;
 }
 static int16 next(FilterRecord* f,Session& s){
  const auto r=f->bigDocumentData?f->bigDocumentData->outRect32:VRect{f->outRect.top,f->outRect.left,f->outRect.bottom,f->outRect.right};
  cell::Source source;if(s.source.valid())source=cell::source_view(s.source,cell::document_bounds(f));
  const auto cancel=[&]{return f->abortProc&&f->abortProc();};
  if(!cell::render(s.saved.values,{double(r.left),double(r.top),1},r.right-r.left,r.bottom-r.top,s.output,source,cancel,cell::locked_alpha(f)))return cancel()?userCanceledErr:filterBadParameters;
  const auto error=Base::write_tile(f,s);if(!error&&f->progressProc){const auto all=Store::full_rect(f);f->progressProc(r.bottom-all.top,std::max(1,all.bottom-all.top));}return error;
 }
 static void entry(int16 selector,FilterRecord* f,intptr_t* data,int16* result){
  if(!result)return;*result=noErr;if(selector==filterSelectorAbout){psui::show_host_about(cell_info,cell_title);return;}
  if(!f||!data){*result=filterBadParameters;return;}if(f->bigDocumentData)f->bigDocumentData->PluginUsing32BitCoordinates=true;
  try{switch(selector){
   case filterSelectorParameters:*data=1;break;
   case filterSelectorPrepare:f->bufferSpace=0;f->bufferSpace64=0;break;
   case filterSelectorStart:*result=start(f,data);break;
   case filterSelectorContinue:if(*data<=1)*result=filterBadParameters;else if(f->abortProc&&f->abortProc())*result=userCanceledErr;else *result=next(f,*reinterpret_cast<Session*>(*data));break;
   case filterSelectorFinish:if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;Base::request(f,{});break;
   default:*result=filterBadParameters;break;
  }}catch(const std::bad_alloc&){*result=memFullErr;}catch(...){*result=filterBadParameters;}
  if(*result&&selector!=filterSelectorPrepare&&selector!=filterSelectorParameters){if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;Base::request(f,{});}
 }
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* f,intptr_t* data,int16* result){Runtime::entry(selector,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE m,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=m;DisableThreadLibraryCalls(m);}return TRUE;}
