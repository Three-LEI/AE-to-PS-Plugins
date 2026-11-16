#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "blinds_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=blinds::Parameters;using Image=blinds::Image;struct Inputs{};
 static constexpr auto magic=blinds::magic;static constexpr auto info=blinds::plugin_info;static constexpr auto title=blinds::title;
 static bool valid(const Parameters& p){return blinds::valid(p)&&p.width>=1;}
 static Parameters defaults(FilterRecord* f){const auto r=f->documentInfo->bounds;return blinds::defaults(r.right-r.left,r.bottom-r.top,r.left,r.top);}
 static Image from_host(psui::LayerPixels&& p){return blinds::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return blinds::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const blinds::Cancel& cancel){return blinds::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
