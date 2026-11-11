#pragma once
#include "cached_filter_runtime.hpp"
namespace psfilter {
// Pointwise generators do not need a full-document snapshot. Only the current
// output tile is held; optional source blending is read for that same tile.
// Effect::generate runs on the host thread, so Channel Ports/abortProc remain safe.
template<class Effect> struct GeneratorRuntime {
 using Base=CachedRuntime<Effect>;
 using Store=Runtime<Effect>;
 using Session=typename Base::Session;
 static int16 start(HINSTANCE module,FilterRecord* f,intptr_t* data){
  const bool interactive=*data==1;*data=0;
  if(f->abortProc&&f->abortProc())return userCanceledErr;
  if((f->depth!=8&&f->depth!=16&&f->depth!=32)||f->planes<3||
   (f->imageMode!=plugInModeRGBColor&&f->imageMode!=plugInModeRGB48&&f->imageMode!=plugInModeRGB96))return filterBadMode;
  auto session=std::make_unique<Session>();session->saved.values=Effect::defaults(f);
  if(!Store::read_parameters(f,session->saved))return filterBadParameters;
  if(interactive){
   HWND owner=f->platformData?reinterpret_cast<HWND>(reinterpret_cast<PlatformData*>(f->platformData)->hwnd):nullptr;
   const auto answer=Effect::dialog(module,owner,f,session->saved.values);
   if(answer!=IDOK)return answer<0?int16(answer):userCanceledErr;
  }
  if(!Effect::valid(session->saved.values))return filterBadParameters;
  if(!Store::save_parameters(f,session->saved))return memFullErr;
  f->autoMask=true;f->wantLayout=0;f->inputRate=f->maskRate=65536;
  const auto all=Store::full_rect(f);Base::request(f,all.right>all.left&&all.bottom>all.top?Store::tile(all.left,all.top,all):VRect{});
  if(f->progressProc)f->progressProc(0,100);
  *data=reinterpret_cast<intptr_t>(session.release());return noErr;
 }
 static int16 next(FilterRecord* f,Session& session){
  const auto r=f->bigDocumentData?f->bigDocumentData->outRect32:VRect{f->outRect.top,f->outRect.left,f->outRect.bottom,f->outRect.right};
  if(r.right<=r.left||r.bottom<=r.top)return filterBadParameters;
  auto error=Effect::generate(f,r,session.saved.values,session.output);if(error)return error;
  error=Base::write_tile(f,session);
  if(!error&&f->progressProc){const auto all=Store::full_rect(f);f->progressProc(r.bottom-all.top,std::max(1,all.bottom-all.top));}
  return error;
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
    else *result=next(f,*reinterpret_cast<Session*>(*data));break;
   case filterSelectorFinish:if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;Base::request(f,{});break;
   default:*result=filterBadParameters;break;
  }}catch(const std::bad_alloc&){*result=memFullErr;}catch(...){*result=filterBadParameters;}
  if(*result&&selector!=filterSelectorParameters&&selector!=filterSelectorPrepare){if(*data>1)delete reinterpret_cast<Session*>(*data);*data=0;Base::request(f,{});}
 }
};
}
