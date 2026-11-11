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
#include <cstring>
#include "../../native/tests/library_qa.hpp"
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
bool blank_source=false,blank_selection=false;
std::vector<std::uint8_t> selection_mask;
OSErr advance() {
    auto& f=*active_host;
    const auto r=f.bigDocumentData->inRect32;
    const int w=r.right-r.left,h=r.bottom-r.top;
    if(w<=0 || h<=0) return noErr;
    const int sample_step=f.inputRate/65536; latest_region={r.top*sample_step,r.left*sample_step,r.bottom*sample_step,r.right*sample_step};
    const int channels=f.inHiPlane+1;
    f.inColumnBytes=channels; f.inPlaneBytes=1; f.inRowBytes=w*channels;
    proxy.resize(static_cast<std::size_t>(w)*h*channels);
    if(blank_source){
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){for(int k=0;k<3;++k)proxy[(y*w+x)*channels+k]=255;proxy[(y*w+x)*channels+3]=0;}
        if(blank_selection){selection_mask.resize(w*h);for(int y=0;y<h;++y)for(int x=0;x<w;++x)selection_mask[y*w+x]=x<w/2?255:0;f.maskData=selection_mask.data();f.maskRowBytes=w;}
        f.inData=proxy.data();return noErr;
    }
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
}
}

int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 const bool dpi_qa=argc>1&&std::string(argv[1]).find("--qa-dpi")==0;
 const bool about_qa=argc>1&&std::string(argv[1]).find("--qa-about")==0;
 const bool passive_verify=argc>1&&std::string(argv[1])=="--verify-manual";
 const bool language_qa=argc>1&&std::string(argv[1]).find("--qa-language")==0;
 const bool blank_qa=argc>1&&std::string(argv[1]).find("--qa-blank")==0;
 blank_source=blank_qa;blank_selection=argc>1&&std::string(argv[1])=="--qa-blank-selection";
 const bool unaware=argc>1&&std::string(argv[1])=="--qa-dpi-unaware";
 if(unaware)SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
 const auto caller_dpi=GetThreadDpiAwarenessContext();
 const bool automated=argc>1&&std::string(argv[1]).find("--qa")==0;
 wchar_t module[32768];GetModuleFileNameW(nullptr,module,32768);const auto output=std::filesystem::path(module).parent_path();
 owner=CreateWindowExW(0,L"STATIC",L"Library UI test owner",WS_OVERLAPPEDWINDOW,180,100,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(owner,SW_SHOW);
 PropertyProcs properties{};properties.getPropertyProc=property;FilterRecord host{};host.propertyProcs=&properties;host.colorServices=picker;BigDocumentStruct big{};big.filterRect32={0,0,512,512};big.imageSize32={512,512};host.bigDocumentData=&big;host.depth=8;host.samplingSupport=1;host.advanceState=advance;active_host=&host;
 if(blank_qa){host.inLayerPlanes=3;host.inTransparencyMask=1;host.haveMask=blank_selection;host.filterCase=blank_selection?filterCaseEditableTransparencyWithSelection:filterCaseEditableTransparencyNoSelection;}
 fractal::Parameters values;values.offset_x=values.offset_y=256;
 if(automated)libraryqa::setup(true,theme,output,std::string(argv[1])=="--qa-warp");
 if(dpi_qa)libraryqa::setup_dpi(true,theme,output);
 if(argc>1&&std::string(argv[1])=="--qa-pan")libraryqa::setup_pan(true,theme,output);
 if(argc>1&&std::string(argv[1])=="--qa-wheel")libraryqa::setup_wheel(true,theme,output);
 if(argc>1&&std::string(argv[1])=="--qa-wheel-native"){libraryqa::setup_wheel(true,theme,output);libraryqa::hook.native_wheel_input=true;}
 if(language_qa)libraryqa::setup_language(true,theme,output,std::string(argv[1])=="--qa-language-zh"?"zh_CN":std::string(argv[1])=="--qa-language-en"?"en_US":"");
 if(language_qa){values.contrast=137.25;values.invert=true;values.rotation=27;values.noise=fractal::NoiseType::spline;}
 if(blank_qa){
  libraryqa::hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==15||ui.frame()==31){libraryqa::point(ui,"预览 / Preview");io.AddMouseButtonEvent(0,true);}if(ui.frame()==16||ui.frame()==32)io.AddMouseButtonEvent(0,false);};
  libraryqa::hook.tick=[output](psui::LibraryPanel& ui){const int n=ui.frame();if(n==8||n==24||n==40){const auto name=std::string(blank_selection?"blank-selection-":"blank-layer-")+std::to_string(n)+".bmp";libraryqa::require(ui.capture((output/name).c_str()),"blank-layer preview capture");const auto r=ui.items.at("image");std::ofstream meta(output/(name+".json"));meta<<"{\"image_rect\":["<<r.x<<','<<r.y<<','<<r.z<<','<<r.w<<"]}";}if(n>46)ui.close(true);};
 }
 if(about_qa){libraryqa::setup_about_zoom(true,theme,output,std::string(argv[1])=="--qa-about-zh");const auto common_tick=libraryqa::hook.tick;libraryqa::hook.tick=[common_tick](psui::LibraryPanel& ui){common_tick(ui);if(ui.frame()==26)libraryqa::require(latest_region.right-latest_region.left<512,"zoom fetches smaller real input region");if(ui.frame()==45)libraryqa::require(latest_region.right-latest_region.left>=512,"Fit restores full input region");};}
 if(about_qa&&std::string(argv[1])=="--qa-about-200")libraryqa::hook.initial_scale=2.f;
 const auto language_original=values;
 const auto answer=fractal::show_dialog(GetModuleHandleW(nullptr),owner,&host,values);
 psui::set_panel_test_hook(nullptr);DestroyWindow(owner);
 if(!AreDpiAwarenessContextsEqual(caller_dpi,GetThreadDpiAwarenessContext())){std::cerr<<"FAIL: caller DPI context changed\n";return 4;}
 std::cout<<"library_dialog_result="<<answer<<" error="<<libraryqa::hook.error<<"\n";
 if(passive_verify){const bool same=std::memcmp(&values,&language_original,sizeof(values))==0;std::cout<<"PASSIVE parameter_equal="<<same<<" contrast="<<values.contrast<<" complexity="<<values.complexity<<" scale="<<values.scale<<" rotation="<<values.rotation<<"\n";if(answer==IDOK&&!same)return 9;}
 if(automated&&answer!=IDOK)return 2;
 if((language_qa||about_qa||(argc>1&&(std::string(argv[1])=="--qa-pan"||(std::string(argv[1])=="--qa-wheel"||std::string(argv[1])=="--qa-wheel-native"))))&&std::memcmp(&values,&language_original,sizeof(values))!=0){std::cerr<<"FAIL: view action changed effect parameters\n";return 5;}
 if(automated){
  std::vector<unsigned char> original(sizeof(values));std::memcpy(original.data(),&values,sizeof(values));
  libraryqa::hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"对比度 / Contrast.slider",.1f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};
  libraryqa::hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>15)ui.close(false);};
  psui::set_panel_test_hook(&libraryqa::hook);
  const auto canceled=fractal::show_dialog(GetModuleHandleW(nullptr),nullptr,&host,values);psui::set_panel_test_hook(nullptr);
  if(canceled!=IDCANCEL||std::memcmp(original.data(),&values,sizeof(values))!=0){std::cerr<<"FAIL: cancel or repeat dialog lifetime\n";return 3;}
 }
 if(blank_qa)std::cout<<"PASS: blank RGBA source; generated preview/on/off/on; optional selection; repeat/cancel.\n";
 else if(about_qa)std::cout<<"PASS: localized About; author glyph; Escape and Close; real zoom crop; Fit; effect parameters unchanged.\n";
 else if(language_qa)std::cout<<"PASS: follow PS locale; live English/Chinese/auto; section state; parameters unchanged; popup; no font rebuild; repeat/cancel.\n";
 else if(automated)std::cout<<"PASS: single-window UI library; Chinese glyphs; popup selection; continuous slider drag; preview toggle; scrolling; theme update; reset; accept; complete GPU-frame snapshots.\n";
 return answer==-1?1:0;
}
