#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "longshadow_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=longshadow::Parameters;using Image=longshadow::Image;struct Inputs{};
 static constexpr auto magic=longshadow::magic;
 static constexpr auto info=longshadow::info;static constexpr auto title=longshadow::title;
 static bool valid(const Parameters& p){return longshadow::valid(p);}
 static Parameters defaults(FilterRecord*){return {};}
 static bool upgrade(const void* bytes,int32 size,Parameters& p){
  using Saved=psfilter::Runtime<Effect>::Saved;
  constexpr auto start=offsetof(Saved,values),v01=offsetof(Parameters,color_mode);
  if(size!=int32(start+v01))return false;
  unsigned long header=0;std::memcpy(&header,bytes,sizeof(header));
  if(header!=magic)return false;
  p={};std::memcpy(&p,static_cast<const char*>(bytes)+start,v01);
  return valid(p);
 }
 static Image from_host(psui::LayerPixels&& px){return longshadow::from_host(std::move(px));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return longshadow::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const longshadow::Cancel& cancel){return longshadow::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
