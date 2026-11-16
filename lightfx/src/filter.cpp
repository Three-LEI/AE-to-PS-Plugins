#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "lightfx_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=lightfx::Parameters;using Image=lightfx::Image;struct Inputs{};
 static constexpr auto magic=lightfx::magic;static constexpr auto info=lightfx::plugin_info;static constexpr auto title=lightfx::title;
 static bool valid(const Parameters& p){return lightfx::valid(p)&&p.kind==lightfx::effect_kind;}
 static Parameters defaults(FilterRecord* f){const auto b=f->documentInfo->bounds;return lightfx::defaults(lightfx::effect_kind,b.right-b.left,b.bottom-b.top,b.left,b.top);}
 static VRect input_bounds(FilterRecord* f){auto b=f->documentInfo->bounds;const auto r=psfilter::Runtime<Effect>::full_rect(f);if(r.right>r.left&&r.bottom>r.top){b.left=std::min(b.left,r.left);b.top=std::min(b.top,r.top);b.right=std::max(b.right,r.right);b.bottom=std::max(b.bottom,r.bottom);}return b;}
 static Image from_host(psui::LayerPixels&& p){return lightfx::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND w,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return lightfx::show_dialog(m,w,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const lightfx::Cancel& cancel){return lightfx::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
