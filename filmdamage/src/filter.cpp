#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../../native/common/cached_filter_runtime.hpp"
#include "filmdamage_host.hpp"
#include "parameter_schema.hpp"
namespace {
HINSTANCE instance=nullptr;
struct Effect {
 using Parameters=filmdamage::Parameters;using Image=filmdamage::Image;struct Inputs{};
 static constexpr auto magic=filmdamage::magic;static constexpr auto info=filmdamage::plugin_info;static constexpr auto title=filmdamage::title;
 static bool valid(const Parameters& p){return filmdamage::valid(p);}
 static Parameters defaults(FilterRecord*){return {};}
 static bool upgrade(const void* bytes,int32 size,Parameters& p){
  using Saved=psfilter::Runtime<Effect>::Saved;
  constexpr auto start=offsetof(Saved,values),v01=offsetof(Parameters,grain_softness),v02=offsetof(Parameters,stain_texture),v03=offsetof(Parameters,stain_edge_roughness);
  constexpr auto v04=offsetof(Parameters,stain_edge_size);
  if(size!=int32(start+v01)&&size!=int32(start+v02)&&size!=int32(start+v03)&&size!=int32(start+v04))return false;
  unsigned long saved_magic=0;std::memcpy(&saved_magic,bytes,sizeof(saved_magic));
  if(saved_magic!=magic)return false;
  p=Parameters{};std::memcpy(&p,static_cast<const char*>(bytes)+start,std::size_t(size)-start);
  return valid(p);
 }
 static Image from_host(psui::LayerPixels&& p){return filmdamage::from_host(std::move(p));}
 static OSErr read_inputs(FilterRecord*,const std::vector<psui::HostLayer>&,const Parameters&,Inputs&){return noErr;}
 static INT_PTR dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>&,Parameters& p,Inputs&){return filmdamage::show_dialog(m,owner,f,src,p);}
 static bool process(const Image& src,const Parameters& p,Image& out,const Inputs&,const filmdamage::Cancel& cancel){return filmdamage::render(src,p,out,cancel);}
};
}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* data,int16* result){psfilter::CachedRuntime<Effect>::entry(instance,s,f,data,result);}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=h;DisableThreadLibraryCalls(h);}return TRUE;}
