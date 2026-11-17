#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "ascii_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=asciifx::Parameters;using Image=asciifx::Image;struct Inputs{};
 static constexpr auto magic=asciifx::magic;static constexpr auto info=asciifx::plugin_info;static constexpr auto title=asciifx::title;
 static bool valid(const Parameters& p){return asciifx::valid(p)&&p.cell_width>=4;}
 static Parameters defaults(FilterRecord*){return {};}
 static Image from_host(psui::LayerPixels&& p){return asciifx::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return asciifx::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const asciifx::Cancel& cancel){auto atlas=asciifx::make_atlas(p,cancel);return asciifx::render(src,p,atlas,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
