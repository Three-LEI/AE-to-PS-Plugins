#pragma once
#include <windows.h>
namespace psui::wincompat {
// Keep newer DPI entry points out of the DLL import table. A Photoshop process
// cannot load a DLL on Windows 7 if any mandatory USER32 import is missing.
struct DpiApi {
 using SetThread=HANDLE(WINAPI*)(HANDLE);
 using GetWindow=UINT(WINAPI*)(HWND);
 using Adjust=BOOL(WINAPI*)(LPRECT,DWORD,BOOL,DWORD,UINT);
 SetThread set_thread=nullptr;GetWindow get_window=nullptr;Adjust adjust=nullptr;
};
inline const DpiApi& dpi_api(){
 static const DpiApi api=[](){auto module=GetModuleHandleW(L"user32.dll");DpiApi value;
  if(module){value.set_thread=reinterpret_cast<DpiApi::SetThread>(GetProcAddress(module,"SetThreadDpiAwarenessContext"));value.get_window=reinterpret_cast<DpiApi::GetWindow>(GetProcAddress(module,"GetDpiForWindow"));value.adjust=reinterpret_cast<DpiApi::Adjust>(GetProcAddress(module,"AdjustWindowRectExForDpi"));}return value;}();
 return api;
}
inline thread_local bool force_legacy_dpi=false;
struct ScopedLegacyDpi {
 bool previous;explicit ScopedLegacyDpi(bool enabled):previous(force_legacy_dpi){force_legacy_dpi=previous||enabled;}
 ~ScopedLegacyDpi(){force_legacy_dpi=previous;}
};
inline HANDLE set_thread_dpi(HANDLE context){return !force_legacy_dpi&&dpi_api().set_thread?dpi_api().set_thread(context):nullptr;}
inline UINT window_dpi(HWND window){
 if(!force_legacy_dpi&&dpi_api().get_window){const auto value=dpi_api().get_window(window);if(value)return value;}
 const auto dc=GetDC(window);const int value=dc?GetDeviceCaps(dc,LOGPIXELSX):96;if(dc)ReleaseDC(window,dc);return value>0?UINT(value):96;
}
inline BOOL adjust_window_rect(LPRECT rect,DWORD style,BOOL menu,DWORD extended,UINT dpi){
 if(!force_legacy_dpi&&dpi_api().adjust)return dpi_api().adjust(rect,style,menu,extended,dpi);
 return AdjustWindowRectEx(rect,style,menu,extended);
}
}
