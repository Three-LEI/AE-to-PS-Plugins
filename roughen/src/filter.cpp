#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "roughen_host.hpp"
#include "script_parameters.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=roughen::Parameters;using Image=roughen::Image;struct Inputs{mutable roughen::Prepared prepared;};
 static constexpr auto magic=roughen::magic;static constexpr auto info=roughen::plugin_info;static constexpr auto title=roughen::title;
 static bool valid(const Parameters& p){return roughen::valid(p);}
 static VRect input_bounds(FilterRecord* f){auto b=f->documentInfo->bounds;const auto r=psfilter::Runtime<Effect>::full_rect(f);if(r.right>r.left&&r.bottom>r.top){b.left=std::min(b.left,r.left);b.top=std::min(b.top,r.top);b.right=std::max(b.right,r.right);b.bottom=std::max(b.bottom,r.bottom);}return b;}
 static Parameters defaults(FilterRecord*){return roughen::defaults();}
 static Image from_host(psui::LayerPixels&& p){return roughen::from_host(std::move(p));}
 static OSErr read_descriptor(FilterRecord* f,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&,bool& interactive){return roughen::script::read(f,p,interactive);}
 static OSErr write_descriptor(FilterRecord* f,const Parameters& p,const Inputs&){return roughen::script::write(f,p);}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs& in){return roughen::show_dialog(m,owner,f,src,p,in.prepared);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& in,const roughen::Cancel& cancel){return roughen::render(src,p,out,&in.prepared,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
