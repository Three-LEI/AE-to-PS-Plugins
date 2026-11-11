// Development preview of the production dialog with mock host services.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "tint_dialog.hpp"
#include "PIProperties.h"
#include <commdlg.h>
#include <cstring>
#include "library_qa.hpp"
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
VRect latest_region{};
OSErr advance() {
    auto& f=*active_host;
    const auto r=f.bigDocumentData->inRect32;
    const int w=r.right-r.left,h=r.bottom-r.top;
    if(w<=0 || h<=0) return noErr;
    const int sample_step=f.inputRate/65536; latest_region={r.top*sample_step,r.left*sample_step,r.bottom*sample_step,r.right*sample_step};
    f.inColumnBytes=3; f.inPlaneBytes=1; f.inRowBytes=w*3;
    proxy.resize(static_cast<std::size_t>(w)*h*3);
    for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
        const int band=std::clamp(((r.left+x)*sample_step*6)/512,0,5);
        const int intensity=64+191*std::clamp((r.top+y)*sample_step,0,511)/511;
        proxy[(y*w+x)*3]=static_cast<std::uint8_t>((band==0 || band==3 || band==5) ? intensity : 0);
        proxy[(y*w+x)*3+1]=static_cast<std::uint8_t>((band==1 || band==3 || band==4) ? intensity : 0);
        proxy[(y*w+x)*3+2]=static_cast<std::uint8_t>((band==2 || band==4 || band==5) ? intensity : 0);
    }
    f.inData=proxy.data(); return noErr;
}
OSErr property(PIType signature,PIType key,int32,intptr_t* simple,Handle*) {
    if(signature!=0x3842494d || key!=propUIBrightness || !simple) return -1;
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
}}

int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 const bool dpi_qa=argc>1&&std::string(argv[1]).find("--qa-dpi")==0;
 const bool about_qa=argc>1&&std::string(argv[1]).find("--qa-about")==0;
 const bool language_qa=argc>1&&std::string(argv[1]).find("--qa-language")==0;
 const bool unaware=argc>1&&std::string(argv[1])=="--qa-dpi-unaware";
 if(unaware)SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
 const auto caller_dpi=GetThreadDpiAwarenessContext();
 const bool automated=argc>1&&std::string(argv[1]).find("--qa")==0;
 wchar_t module[32768];GetModuleFileNameW(nullptr,module,32768);const auto output=std::filesystem::path(module).parent_path();
 owner=CreateWindowExW(0,L"STATIC",L"Library UI test owner",WS_OVERLAPPEDWINDOW,180,100,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(owner,SW_SHOW);
 PropertyProcs properties{};properties.getPropertyProc=property;FilterRecord host{};host.propertyProcs=&properties;host.colorServices=picker;BigDocumentStruct big{};big.filterRect32={0,0,512,512};big.imageSize32={512,512};host.bigDocumentData=&big;host.depth=8;host.samplingSupport=1;host.advanceState=advance;active_host=&host;
 tint::Parameters values;
 if(automated)libraryqa::setup(false,theme,output,std::string(argv[1])=="--qa-warp");
 if(dpi_qa)libraryqa::setup_dpi(false,theme,output);
 if(argc>1&&std::string(argv[1])=="--qa-pan")libraryqa::setup_pan(false,theme,output);
 if(argc>1&&std::string(argv[1])=="--qa-wheel")libraryqa::setup_wheel(false,theme,output);
 if(argc>1&&std::string(argv[1])=="--qa-wheel-native"){libraryqa::setup_wheel(false,theme,output);libraryqa::hook.native_wheel_input=true;}
 if(argc>1&&std::string(argv[1])=="--qa-layout")libraryqa::setup_tint_layout(theme,output);
 if(language_qa)libraryqa::setup_language(false,theme,output,std::string(argv[1])=="--qa-language-zh"?"zh_CN":std::string(argv[1])=="--qa-language-en"?"en_US":"");
 if(language_qa){values.amount=.423;values.black[0]=.15;values.white[2]=.67;}
 if(about_qa){libraryqa::setup_about_zoom(false,theme,output,std::string(argv[1])=="--qa-about-zh");const auto common_tick=libraryqa::hook.tick;libraryqa::hook.tick=[common_tick](psui::LibraryPanel& ui){common_tick(ui);if(ui.frame()==26)libraryqa::require(latest_region.right-latest_region.left<512,"zoom fetches smaller real input region");if(ui.frame()==45)libraryqa::require(latest_region.right-latest_region.left>=512,"Fit restores full input region");};}
 if(about_qa&&std::string(argv[1])=="--qa-about-200")libraryqa::hook.initial_scale=2.f;
 const auto language_original=values;
 const auto answer=tint::show_dialog(GetModuleHandleW(nullptr),owner,&host,values);
 psui::set_panel_test_hook(nullptr);DestroyWindow(owner);
 if(!AreDpiAwarenessContextsEqual(caller_dpi,GetThreadDpiAwarenessContext())){std::cerr<<"FAIL: caller DPI context changed\n";return 4;}
 std::cout<<"library_dialog_result="<<answer<<" error="<<libraryqa::hook.error<<"\n";
 if(automated&&answer!=IDOK)return 2;
 if((language_qa||about_qa||(argc>1&&(std::string(argv[1])=="--qa-pan"||(std::string(argv[1])=="--qa-wheel"||std::string(argv[1])=="--qa-wheel-native"))))&&std::memcmp(&values,&language_original,sizeof(values))!=0){std::cerr<<"FAIL: view action changed effect parameters\n";return 5;}
 if(automated){
  std::vector<unsigned char> original(sizeof(values));std::memcpy(original.data(),&values,sizeof(values));
  libraryqa::hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"着色量 / Amount.slider",.1f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};
  libraryqa::hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>15)ui.close(false);};
  psui::set_panel_test_hook(&libraryqa::hook);
  const auto canceled=tint::show_dialog(GetModuleHandleW(nullptr),nullptr,&host,values);psui::set_panel_test_hook(nullptr);
  if(canceled!=IDCANCEL||std::memcmp(original.data(),&values,sizeof(values))!=0){std::cerr<<"FAIL: cancel or repeat dialog lifetime\n";return 3;}
 }
 if(about_qa)std::cout<<"PASS: localized About; author glyph; Escape and Close; real zoom crop; Fit; effect parameters unchanged.\n";
 else if(language_qa)std::cout<<"PASS: follow PS locale; live English/Chinese/auto; section state; parameters unchanged; popup; no font rebuild; repeat/cancel.\n";
 else if(argc>1&&std::string(argv[1])=="--qa-layout")std::cout<<"PASS: initial/native minimum sizes; move; minimum resize; restore; all Tint control rectangles remain visible and stable.\n";
 else if(automated)std::cout<<"PASS: single-window UI library; Chinese glyphs; popup selection; continuous slider drag; preview toggle; scrolling; theme update; reset; accept; complete GPU-frame snapshots.\n";
 return answer==-1?1:0;
}
