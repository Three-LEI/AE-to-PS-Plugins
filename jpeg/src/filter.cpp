#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "jpeg_host.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=jpegfx::Parameters;using Image=jpegfx::Image;struct Inputs{Image mask;};
 static constexpr unsigned long magic=0x5A4A4431;
 static constexpr auto info=jpegfx::plugin_info;static constexpr auto title=jpegfx::title;
 static bool valid(const Parameters& p){return jpegfx::valid(p);}
 static Parameters defaults(FilterRecord*){return {};}
 static Image from_host(psui::LayerPixels&& p){return jpegfx::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const Parameters& p,Inputs& in){return psfilter::CachedRuntime<Effect>::read_named(f,layers,p.mask_key,in.mask);}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& in){return jpegfx::show_dialog(m,owner,f,src,layers,p,in.mask);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& in,const jpegfx::Cancel& cancel){return jpegfx::render(src,p,out,in.mask.valid()?&in.mask:nullptr,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
