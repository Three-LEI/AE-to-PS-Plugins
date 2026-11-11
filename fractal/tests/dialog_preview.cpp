// Development preview of the production dialog with mock host services.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "fractal_core.hpp"
#include "PIFilter.h"
#include "psui/scroll_panel.hpp"
namespace fractal {INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,Parameters&);}
#include "PIProperties.h"
#include <commdlg.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstdint>

namespace {
int theme=3;
bool qa=false;
int qa_step=0;
HWND owner=nullptr;
std::filesystem::path theme_file;
FilterRecord* active_host=nullptr;
std::vector<std::uint8_t> proxy;
OSErr advance() {
    auto& f=*active_host;
    const auto r=f.bigDocumentData->inRect32;
    const int w=r.right-r.left,h=r.bottom-r.top;
    if(w<=0 || h<=0) return noErr;
    f.inColumnBytes=3; f.inPlaneBytes=1; f.inRowBytes=w*3;
    proxy.resize(static_cast<std::size_t>(w)*h*3);
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        const int band=(x*6)/w;
        const int intensity=64+(191*y)/std::max(1,h-1);
        proxy[(y*w+x)*3]=static_cast<std::uint8_t>((band==0 || band==3 || band==5) ? intensity : 0);
        proxy[(y*w+x)*3+1]=static_cast<std::uint8_t>((band==1 || band==3 || band==4) ? intensity : 0);
        proxy[(y*w+x)*3+2]=static_cast<std::uint8_t>((band==2 || band==4 || band==5) ? intensity : 0);
    }
    f.inData=proxy.data(); return noErr;
}
OSErr property(PIType signature,PIType key,int32,intptr_t* simple,Handle*) {
    if(signature!=0x3842494d || key!=propUIBrightness || !simple) return -1;
    // Mock-host fixture only: change this file to exercise live theme updates.
    if(!qa){std::ifstream input(theme_file);int next;if(input>>next&&next>=0&&next<=3)theme=next;}
    *simple=theme; return noErr;
}
OSErr picker(ColorServicesInfo* info) {
    COLORREF custom[16]{};
    CHOOSECOLORW choose{}; choose.lStructSize=sizeof(choose); choose.hwndOwner=GetLastActivePopup(owner);
    choose.rgbResult=RGB(info->colorComponents[0],info->colorComponents[1],info->colorComponents[2]);
    choose.lpCustColors=custom; choose.Flags=CC_RGBINIT | CC_FULLOPEN;
    if(!ChooseColorW(&choose)) return userCanceledErr;
    info->colorComponents[0]=GetRValue(choose.rgbResult); info->colorComponents[1]=GetGValue(choose.rgbResult); info->colorComponents[2]=GetBValue(choose.rgbResult);
    return noErr;
}
void check(bool value,const char* text){if(!value){std::cerr<<"FAIL: "<<text<<'\n';std::exit(2);}}
void erase_probe(HWND dialog){
 struct Probe{int controls=0,changed=0;};Probe probe;
 EnumChildWindows(dialog,[](HWND child,LPARAM data)->BOOL{
  auto& p=*reinterpret_cast<Probe*>(data);wchar_t cls[32];GetClassNameW(child,cls,32);const auto style=GetWindowLongPtrW(child,GWL_STYLE);
  if(!((std::wstring(cls)==L"Button"&&(style&BS_TYPEMASK)==BS_OWNERDRAW)||(std::wstring(cls)==L"Static"&&(style&SS_TYPEMASK)==SS_OWNERDRAW)))return TRUE;
  ++p.controls;HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=128;info.bmiHeader.biHeight=-64;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;void* bits=nullptr;auto bmp=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(dc,bmp);auto* pixels=static_cast<std::uint32_t*>(bits);std::fill(pixels,pixels+128*64,0x00102030);
  SendMessageW(child,WM_ERASEBKGND,reinterpret_cast<WPARAM>(dc),0);GdiFlush();
  if(std::any_of(pixels,pixels+128*64,[](auto x){return (x&0xffffff)!=0x102030;})){++p.changed;std::cout<<"ERASE wrote control id="<<GetDlgCtrlID(child)<<'\n';}
  SelectObject(dc,old);DeleteObject(bmp);DeleteDC(dc);return TRUE;
 },reinterpret_cast<LPARAM>(&probe));
 std::cout<<"ERASE probe controls="<<probe.controls<<" altered="<<probe.changed<<'\n';check(probe.controls>20&&probe.changed==0,"owner-draw erase must preserve previous frame; no system-colored intermediate frame");
}
void snapshot(HWND w,const wchar_t* name){
 RECT window;GetWindowRect(w,&window);RECT r{0,0,window.right-window.left,window.bottom-window.top};BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
 HDC dc=CreateCompatibleDC(nullptr);void* bits=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(dc,bitmap);
 SendMessageW(w,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_NONCLIENT|PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND|PRF_CHECKVISIBLE);GdiFlush();
 BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+r.right*r.bottom*4;
 std::ofstream file(theme_file.parent_path()/name,std::ios::binary);file.write(reinterpret_cast<const char*>(&header),sizeof(header));file.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(info.bmiHeader));file.write(static_cast<const char*>(bits),r.right*r.bottom*4);
 SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
}
void CALLBACK qa_tick(HWND,UINT,UINT_PTR timer,DWORD){
 HWND w=GetLastActivePopup(owner);if(w==owner||!IsWindow(w))return;HWND panel=GetDlgItem(w,220);if(!panel)return;
 auto click=[&](int id){SendMessageW(w,WM_COMMAND,MAKEWPARAM(id,BN_CLICKED),reinterpret_cast<LPARAM>(GetDlgItem(panel,id)));};
 switch(qa_step++){
 case 0:erase_probe(w);check(SendDlgItemMessageW(panel,1000,CB_GETCOUNT,0,0)==17,"17 fractal choices");check(SendDlgItemMessageW(panel,1003,CB_GETCOUNT,0,0)==29,"29 blend choices");snapshot(w,L"ui-default.bmp");for(int i:{1,2,3})click(6100+i);break;
 case 1:{psui::scroll_to(panel,100000);check(psui::scroll_position(panel)>700,"expanded groups overflow viewport");SendMessageW(panel,WM_VSCROLL,SB_BOTTOM,0);break;}
 case 2:snapshot(w,L"ui-expanded-bottom.bmp");check(psui::scroll_position(panel)>0,"scrollbar reaches bottom");click(2001);check(IsWindowEnabled(GetDlgItem(panel,312))&&!IsWindowEnabled(GetDlgItem(panel,302)),"uniform/width dependency");click(2004);check(IsWindowEnabled(GetDlgItem(panel,317)),"cycle field enabled");SetDlgItemTextW(panel,317,L"2");SetDlgItemTextW(panel,304,L"135");SetDlgItemTextW(panel,303,L"8.5");SendDlgItemMessageW(panel,1000,CB_SETCURSEL,8,0);SendMessageW(w,WM_COMMAND,MAKEWPARAM(1000,CBN_SELCHANGE),reinterpret_cast<LPARAM>(GetDlgItem(panel,1000)));psui::scroll_to(panel,0);break;
 case 3:snapshot(w,L"ui-expanded-top.bmp");SetDlgItemTextW(panel,300,L"bad");SendMessageW(w,WM_COMMAND,IDOK,0);check(IsWindow(w),"invalid input keeps dialog open");SetDlgItemTextW(panel,300,L"150");theme=0;SendMessageW(w,WM_TIMER,1,0);break;
 case 4:snapshot(w,L"ui-light.bmp");SendMessageW(w,WM_COMMAND,209,0);{wchar_t text[32];GetDlgItemTextW(panel,308,text,32);check(std::wstring(text)==L"256","reset uses document center");}for(int i:{1,2,3})click(6100+i);break;
 case 5:check(psui::scroll_position(panel)==0,"collapse clamps scroll");snapshot(w,L"ui-light-collapsed.bmp");SetDlgItemTextW(panel,300,L"135");KillTimer(nullptr,timer);SendMessageW(w,WM_COMMAND,IDOK,0);break;
 }
}
}
int main(int argc,char** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    wchar_t module[32768]; GetModuleFileNameW(nullptr,module,32768);
    theme_file=std::filesystem::path(module).parent_path()/L"preview-theme.txt";
    if(argc>1){qa=std::string(argv[1])=="--qa";if(!qa)theme=std::clamp(std::atoi(argv[1]),0,3);}
    owner=CreateWindowExW(0,L"STATIC",L"Fractal Noise UI Preview - mock host",WS_OVERLAPPEDWINDOW,180,100,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!owner) return 1;
    ShowWindow(owner,SW_SHOW); UpdateWindow(owner);
    PropertyProcs properties{}; properties.getPropertyProc=property;
    FilterRecord host{}; host.propertyProcs=&properties; host.colorServices=picker;
    BigDocumentStruct big{}; big.filterRect32={0,0,512,512};big.imageSize32={512,512};
    host.bigDocumentData=&big; host.samplingSupport=1; host.depth=8; host.advanceState=advance; active_host=&host;
    fractal::Parameters values;values.offset_x=values.offset_y=256;
    if(qa)SetTimer(nullptr,0,250,qa_tick);
    const auto result=fractal::show_dialog(GetModuleHandleW(nullptr),owner,&host,values);
    std::cout << "dialog_result=" << result << " theme=" << theme << " amount=" << values.complexity << '\n';
    if(qa){check(result==IDOK&&values.contrast==135&&values.uniform_scale&&values.cycles==1,"accepted values and reset state");std::cout<<"PASS: production dialog; all dropdown choices; collapse/reflow/scrollbar; numeric validation; scale/cycle dependencies; preview updates; reset; dark/light snapshots; accept. Mock host only.\n";}
    DestroyWindow(owner); return result==-1 ? 1 : 0;
}
