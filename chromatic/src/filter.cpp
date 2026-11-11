#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "chromatic_host.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=chromatic::Parameters;using Image=chromatic::Image;
 struct Inputs {Image mask;};
 static constexpr unsigned long magic=0x5A434431;
 static constexpr auto info=chromatic::plugin_info;
 static constexpr auto title=chromatic::title;
 static bool valid(const Parameters& p){return chromatic::valid(p);}
 static Parameters defaults(FilterRecord*){return chromatic::preset(2);}
 static Image from_host(psui::LayerPixels&& pixels){return chromatic::from_host(std::move(pixels));}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const Parameters& p,Inputs& inputs){return psfilter::CachedRuntime<Effect>::read_named(f,layers,p.mask_key,inputs.mask);}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& inputs){return chromatic::show_dialog(m,owner,f,src,layers,p,inputs.mask);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& inputs,const chromatic::Cancel& cancel){return chromatic::render(src,p,out,inputs.mask.valid()?&inputs.mask:nullptr,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,selector,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE m,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=m;DisableThreadLibraryCalls(m);}return TRUE;}
