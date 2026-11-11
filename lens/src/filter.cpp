#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "lens_host.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=lens::Parameters;using Image=lens::Image;struct Inputs{};
 static constexpr unsigned long magic=lens::effect_kind==lens::Kind::cc?0x5A4C4331:0x5A4C4F31;
 static constexpr auto info=lens::plugin_info;static constexpr auto title=lens::title;
 static bool valid(const Parameters& p){return lens::valid(p)&&p.kind==lens::effect_kind;}
 static Parameters defaults(FilterRecord* f){auto b=f->documentInfo->bounds;return lens::defaults(lens::effect_kind,b.right-b.left,b.bottom-b.top,b.left,b.top);}
 static Image from_host(psui::LayerPixels&& p){return lens::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return lens::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const lens::Cancel& cancel){return lens::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
