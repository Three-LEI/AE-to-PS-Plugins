#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "caustics_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=caustics::Parameters;using Image=caustics::Image;struct Inputs{Image map;};
 static constexpr auto magic=caustics::magic;static constexpr auto info=caustics::plugin_info;static constexpr auto title=caustics::title;
 static bool valid(const Parameters& p){return caustics::valid(p);}
 static Parameters defaults(FilterRecord*){return {};}
 static Image from_host(psui::LayerPixels&& p){return caustics::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const Parameters& p,Inputs& in){return p.surface==2?psfilter::CachedRuntime<Effect>::read_named(f,layers,p.map_key,in.map):noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& in){return caustics::show_dialog(m,owner,f,src,layers,p,in.map);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& in,const caustics::Cancel& cancel){return caustics::render(src,p,out,in.map.valid()?&in.map:nullptr,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
