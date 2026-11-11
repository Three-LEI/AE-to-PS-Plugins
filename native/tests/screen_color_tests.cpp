// Tests real desktop pixel readback against this test's own opaque window.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/photoshop_host.hpp"
#include <cmath>
#include <iostream>
int main(){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 HBRUSH brush=CreateSolidBrush(RGB(24,180,216));
 WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"ZaYu.ScreenColorQA";wc.hbrBackground=brush;
 if(!RegisterClassW(&wc))return 1;
 HWND w=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,wc.lpszClassName,L"ZaYu Screen RGB fixture",WS_POPUP,30,120,160,120,nullptr,nullptr,wc.hInstance,nullptr);
 if(!w)return 2;
 ShowWindow(w,SW_SHOWNOACTIVATE);UpdateWindow(w);GdiFlush();Sleep(50);
 POINT point{80,50};ClientToScreen(w,&point);double rgb[3]{};
 const bool read=psui::screen_sample_rgb(point,rgb);
 bool ok=read&&std::abs(rgb[0]-24/255.)<.00001&&std::abs(rgb[1]-180/255.)<.00001&&std::abs(rgb[2]-216/255.)<.00001;
 const auto before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 for(int n=0;n<256;++n)ok=psui::screen_sample_rgb(point,rgb)&&ok;
 const auto after=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 ok=ok&&before==after;
 const double previous=rgb[0];ok=ok&&!psui::screen_sample_rgb({LONG_MIN,LONG_MIN},rgb)&&rgb[0]==previous;
 DestroyWindow(w);UnregisterClassW(wc.lpszClassName,wc.hInstance);DeleteObject(brush);
 if(!ok){std::cerr<<"FAIL: screen RGB="<<rgb[0]<<','<<rgb[1]<<','<<rgb[2]<<" GDI="<<before<<'/'<<after<<'\n';return 3;}
 std::cout<<"PASS: actual desktop RGB 24/180/216; 256 samples without GDI handle growth; invalid screen coordinate leaves colour unchanged.\n";
}
