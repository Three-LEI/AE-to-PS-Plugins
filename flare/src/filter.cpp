#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "flare_host.hpp"
#include "flare_settings.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=flare::Parameters;using Image=flare::Image;struct Inputs{Image mask;flare::Design design;};
 static constexpr auto magic=flare::magic;static constexpr auto info=flare::plugin_info;static constexpr auto title=flare::title;
 static bool valid(const Parameters& p){return flare::valid(p);}
 static Parameters defaults(FilterRecord* f){const auto b=f->documentInfo->bounds;return flare::defaults(b.right-b.left,b.bottom-b.top,b.left,b.top);}
 static Image from_host(psui::LayerPixels&& p){return flare::from_host(std::move(p));}
 static OSErr read_descriptor(FilterRecord* f,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& in,bool& interactive){return flare::read_settings(f,layers,p,in.design,interactive);}
 static OSErr write_descriptor(FilterRecord* f,const Parameters& p,const Inputs& in){return flare::write_settings(f,psui::host_layers(f),p,in.design);}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const Parameters& p,Inputs& in){return p.occlusion?psfilter::CachedRuntime<Effect>::read_named(f,layers,p.map_key,in.mask):noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& in){return flare::show_dialog(m,owner,f,src,layers,p,in.mask,&in.design);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& in,const flare::Cancel& cancel){return flare::render(src,p,out,in.mask.valid()?&in.mask:nullptr,cancel,{},&in.design);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
