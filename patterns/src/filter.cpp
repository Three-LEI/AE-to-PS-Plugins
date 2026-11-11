#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/generator_filter_runtime.hpp"
#include "pattern_host.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=patterns::Parameters;using Image=patterns::Image;
 static constexpr unsigned long magic=pattern_kind==patterns::Kind::grid?0x5A475231:0x5A434B31;
 static constexpr auto info=pattern_info;
 static constexpr auto title=pattern_title;
 static bool valid(const Parameters& p){return patterns::valid(p);}
 static Parameters defaults(FilterRecord* f){return patterns::defaults_for_host(f);}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,Parameters& p){return patterns::show_dialog(m,owner,f,p);}
 static OSErr generate(FilterRecord* f,VRect rect,const Parameters& p,Image& output){
  psui::LayerPixels pixels;const std::vector<float>* source=nullptr;
  if(p.blend!=patterns::Blend::none){
   if(!f->documentInfo||!f->channelPortProcs)return psfilter::CachedRuntime<Effect>::report(f,"Photoshop did not expose readable source channels for compositing.");
   psui::HostLayer target;auto* channel=f->documentInfo->targetCompositeChannels;
   for(int k=0;k<3&&channel;++k,channel=channel->next)target.rgb[k]=channel;target.alpha=f->documentInfo->targetTransparency;
   if(!target.rgb[2])return filterBadMode;
   const auto error=psui::host_read_layer(f,target,rect,0,pixels);if(error)return error;source=&pixels.rgba;
  }
  const auto cancel=[&]{return f->abortProc&&f->abortProc();};
  return patterns::render(pattern_kind,p,{double(rect.left),double(rect.top),1},rect.right-rect.left,rect.bottom-rect.top,output,source,cancel,patterns::locked_alpha(f))?noErr:cancel()?userCanceledErr:filterBadParameters;
 }
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* f,intptr_t* data,int16* result){psfilter::GeneratorRuntime<Effect>::entry(instance,selector,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE m,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=m;DisableThreadLibraryCalls(m);}return TRUE;}
