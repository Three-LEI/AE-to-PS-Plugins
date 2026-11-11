#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "vector_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=vectorblur::Parameters;using Image=vectorblur::Image;
 struct Inputs {Image map;};
 static constexpr unsigned long magic=0x5A564231;
 static constexpr auto info=vectorblur::plugin_info;
 static constexpr auto title=vectorblur::title;
 static bool valid(const Parameters& p){return vectorblur::valid(p);}
 static bool upgrade(const void* bytes,int32 size,Parameters& p){
  using Saved=psfilter::Runtime<Effect>::Saved;
  constexpr auto start=offsetof(Saved,values),prefix=offsetof(Parameters,kernel_version);
  static_assert(start+prefix+sizeof(p.kernel_version)==sizeof(Saved));
  unsigned long saved_magic=0;if(size!=int32(start+prefix))return false;
  std::memcpy(&saved_magic,bytes,sizeof(saved_magic));if(saved_magic!=magic)return false;
  p=Parameters{};std::memcpy(&p,static_cast<const char*>(bytes)+start,prefix);
  // v0.1 defaulted to curved bilateral averaging. Migrate to the calibrated baseline.
  p.follow=false;p.kernel_version=2;return valid(p);
 }
 static Parameters defaults(FilterRecord*){return {};}
 static Image from_host(psui::LayerPixels&& p){return vectorblur::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const Parameters& p,Inputs& inputs){return psfilter::CachedRuntime<Effect>::read_named(f,layers,p.map_key,inputs.map);}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Inputs& inputs){return vectorblur::show_dialog(m,owner,f,src,layers,p,inputs.map);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs& inputs,const vectorblur::Cancel& cancel){return vectorblur::render(src,p,out,inputs.map.valid()?&inputs.map:nullptr,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 selector,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,selector,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE m,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=m;DisableThreadLibraryCalls(m);}return TRUE;}
