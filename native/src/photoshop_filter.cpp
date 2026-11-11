#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../common/filter_runtime.hpp"
#include "tint_core.hpp"
#include "tint_dialog.hpp"
#include "tint_info.hpp"
namespace {
HINSTANCE module_instance=nullptr;
struct TintEffect {
 using Parameters=tint::Parameters;
 static constexpr unsigned long magic=0x54494e31;
 static constexpr auto title=L"Tint 色调";
 static constexpr auto info=tint::plugin_info;
 static Parameters defaults(FilterRecord*) { return {}; }
 static bool valid(const Parameters& p) { tint::ConstImage a; tint::Image b;return tint::process(a,b,p)==tint::Status::ok; }
 static INT_PTR dialog(HINSTANCE m,HWND w,FilterRecord* f,Parameters& p) {return tint::show_dialog(m,w,f,p);}
 static int16 render(FilterRecord* f,VRect r,const Parameters& p) {
  const auto w=static_cast<std::size_t>(r.right-r.left),h=static_cast<std::size_t>(r.bottom-r.top);
  tint::ConstImage in{f->inData,(h-1)*f->inRowBytes+w*3,w,h,static_cast<std::size_t>(f->inRowBytes),3,tint::SampleType::u8};
  tint::Image out{f->outData,(h-1)*f->outRowBytes+w*3,w,h,static_cast<std::size_t>(f->outRowBytes),3,tint::SampleType::u8};
  return tint::process(in,out,p)==tint::Status::ok ? noErr : filterBadParameters;
 }
};
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH)module_instance=instance;return TRUE;}
extern "C" __declspec(dllexport) void PluginMain(int16 s,FilterRecord* f,intptr_t* d,int16* r) {psfilter::Runtime<TintEffect>::entry(module_instance,s,f,d,r);}
