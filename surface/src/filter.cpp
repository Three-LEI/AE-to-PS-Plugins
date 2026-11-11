#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "surface_host.hpp"
#include "script_parameters.hpp"
#include "smart_trace.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=surface::Parameters;using Image=surface::Image;struct Inputs{Image map;surface::MapBinding binding;};
 static constexpr unsigned long magic=surface::effect_kind==surface::Kind::blob?0x5A424C31:0x5A474C31;
 static constexpr auto info=surface::plugin_info;static constexpr auto title=surface::title;
 static bool valid(const Parameters& p){return surface::valid(p)&&p.kind==surface::effect_kind;}
 static VRect input_bounds(FilterRecord* f){auto bounds=f->documentInfo->bounds;const auto area=psfilter::Runtime<Effect>::full_rect(f);if(area.right>area.left&&area.bottom>area.top){bounds.left=std::min(bounds.left,area.left);bounds.top=std::min(bounds.top,area.top);bounds.right=std::max(bounds.right,area.right);bounds.bottom=std::max(bounds.bottom,area.bottom);}return bounds;}
 static Parameters defaults(FilterRecord* f){const auto b=f->documentInfo->bounds;auto p=surface::defaults(surface::effect_kind,b.right-b.left,b.bottom-b.top,b.left,b.top);
#ifdef SURFACE_SMART_TRACE
  surface::smart_trace_seed(p);
#endif
  return p;}
 static Image from_host(psui::LayerPixels&& p){return surface::from_host(std::move(p));}
 static OSErr read_descriptor(FilterRecord* f,const std::vector<psui::HostLayer>&,Parameters& p,Inputs& in,bool& interactive){return surface::script::read(f,p,in.binding,interactive);}
 static OSErr write_descriptor(FilterRecord* f,const Parameters& p,const Inputs& in){return surface::script::write(f,p,in.binding);}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& in){
  if(!p.map_key){in.map={};return noErr;}
  const auto* layer=surface::script::resolve(f,layers,p,in.binding);if(!layer)return psfilter::CachedRuntime<Effect>::report(f,"The saved reference layer is missing or cannot be resolved. Reopen the effect and select its map.");
  psui::LayerPixels pixels;const auto error=psui::host_read_layer(f,*layer,input_bounds(f),0,pixels);if(!error)in.map=from_host(std::move(pixels));return error;
 }
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& in){return surface::show_dialog(m,owner,f,src,layers,p,in.map,&in.binding);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& in,const surface::Cancel& cancel){return surface::render(src,p,out,in.map.valid()?&in.map:nullptr,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){
#ifdef SURFACE_SMART_TRACE
 if(s==filterSelectorContinue)surface::smart_trace(s,f,data?*data:0,12345);
#endif
 psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);
#ifdef SURFACE_SMART_TRACE
 surface::smart_trace(s,f,data?*data:0,result?*result:0);
#endif
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
