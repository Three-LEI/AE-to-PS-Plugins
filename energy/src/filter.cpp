#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "energy_host.hpp"
#include "energy_legacy.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=energy::Parameters;using Image=energy::Image;using Inputs=energy::Inputs;
 static constexpr auto magic=energy::magic;static constexpr auto info=energy::plugin_info;static constexpr auto title=energy::title;
 static bool valid(const Parameters& p){return energy::valid(p);}
 static bool upgrade(const void* data,std::size_t size,Parameters& p){return energy::legacy::upgrade(data,size,p);}
 static Parameters defaults(FilterRecord* f){auto b=f->documentInfo->bounds;return energy::unified_defaults(b.right-b.left,b.bottom-b.top,b.left,b.top);}
 static VRect input_bounds(FilterRecord* f){auto b=f->documentInfo->bounds;auto r=psfilter::Runtime<Effect>::full_rect(f);b.left=std::min(b.left,r.left);b.top=std::min(b.top,r.top);b.right=std::max(b.right,r.right);b.bottom=std::max(b.bottom,r.bottom);return b;}
 static Image from_host(psui::LayerPixels&& p){return energy::from_host(std::move(p));}
 static OSErr read_descriptor(FilterRecord* f,const std::vector<psui::HostLayer>&,Parameters& p,Inputs& in,bool& interactive){return energy::read_settings(f,p,in,interactive);}
 static OSErr write_descriptor(FilterRecord* f,const Parameters& p,const Inputs& in){return energy::write_settings(f,p,in);}
 static OSErr finish_descriptor(FilterRecord* f,const Parameters& p,const Inputs& in){return energy::write_settings(f,p,in);}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>&,Parameters& p,Inputs& in){
  if(p.input!=1||!in.path.curves.empty())return noErr;
  return psfilter::CachedRuntime<Effect>::report(f,"No saved geometry. Open Saber and select a Photoshop path.");
 }
 static INT_PTR dialog(HINSTANCE m,HWND w,FilterRecord* f,const Image& s,const std::vector<psui::HostLayer>&,Parameters& p,Inputs& in){return energy::show_dialog(m,w,f,s,p,in);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& in,const energy::Cancel& cancel){return energy::render(src,p,in.path,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
