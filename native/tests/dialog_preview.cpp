// Development preview of the production dialog with mock host services.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "tint_dialog.hpp"
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
    std::ifstream input(theme_file); int next;
    if(input>>next && next>=0 && next<=3) theme=next;
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
}
int main(int argc,char** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    wchar_t module[32768]; GetModuleFileNameW(nullptr,module,32768);
    theme_file=std::filesystem::path(module).parent_path()/L"preview-theme.txt";
    if(argc>1) theme=std::clamp(std::atoi(argv[1]),0,3);
    owner=CreateWindowExW(0,L"STATIC",L"Tint UI Preview - mock host",WS_OVERLAPPEDWINDOW,180,100,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!owner) return 1;
    ShowWindow(owner,SW_SHOW); UpdateWindow(owner);
    PropertyProcs properties{}; properties.getPropertyProc=property;
    FilterRecord host{}; host.propertyProcs=&properties; host.colorServices=picker;
    BigDocumentStruct big{}; big.filterRect32={0,0,512,512};
    host.bigDocumentData=&big; host.samplingSupport=1; host.depth=8; host.advanceState=advance; active_host=&host;
    tint::Parameters values;
    const auto result=tint::show_dialog(GetModuleHandleW(nullptr),owner,&host,values);
    std::cout << "dialog_result=" << result << " theme=" << theme << " amount=" << values.amount << '\n';
    DestroyWindow(owner); return result==-1 ? 1 : 0;
}
