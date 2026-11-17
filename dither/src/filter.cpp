#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "dither_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=dither::Parameters;using Image=dither::Image;struct Inputs{};
 static constexpr auto magic=dither::magic;static constexpr auto info=dither::plugin_info;static constexpr auto title=dither::title;
 static bool valid(const Parameters& p){return dither::valid(p)&&p.pixel_size>=1;}
 static Parameters defaults(FilterRecord*){return {};}
 static Image from_host(psui::LayerPixels&& p){return dither::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return dither::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const dither::Cancel& cancel){return dither::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
