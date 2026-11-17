#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "etching_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=etching::Parameters;using Image=etching::Image;struct Inputs{};
 static constexpr auto magic=etching::magic;static constexpr auto info=etching::plugin_info;static constexpr auto title=etching::title;
 static bool valid(const Parameters& p){return etching::valid(p);}
 static Parameters defaults(FilterRecord*){return {};}
 static Image from_host(psui::LayerPixels&& p){return etching::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return etching::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const etching::Cancel& cancel){return etching::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
