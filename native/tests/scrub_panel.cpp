// Integration test of the actual shared DX11 panel, not a Photoshop session.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/generator_controls.hpp"
#include "PIProperties.h"
#include "library_qa.hpp"
#include <cmath>
#include <iostream>
namespace {
int theme=3;
OSErr property(PIType signature,PIType key,int32,intptr_t* value,Handle*) {
  if(signature!=0x3842494d||key!=propUIBrightness||!value)return -1;
  *value=theme;return noErr;
}
}
int main(int argc,char** argv) {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  const bool high_dpi=argc>1&&std::string(argv[1])=="--200";
  const std::filesystem::path folder=argc>2?argv[2]:"evidence/numeric-scrub-0.18";
  std::filesystem::create_directories(folder);
  PropertyProcs props{};props.getPropertyProc=property;
  FilterRecord host{};host.propertyProcs=&props;
  psui::PreviewView view;view.configure({0,0,512,512},256,256);
  const double initial_zoom=view.percent;
  double value=100,x=100,y=200;
  const psui::PluginInfo info{"Numeric controls 数值控件","0.18"};
  psui::PanelTestHook hook;hook.software=true;hook.initial_scale=high_dpi?2:1;
  hook.host_locale=high_dpi?"zh_CN":"en_US";
  std::string active;ImVec4 box{};
  auto assert_value=[](double a,double b,const char* message){if(std::abs(a-b)>=.00001)std::cerr<<message<<": "<<a<<" != "<<b<<'\n';libraryqa::require(std::abs(a-b)<.00001,message);};
  hook.before_frame=[&](psui::LibraryPanel& ui){
    Sleep(8);auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;
    io.MouseDoubleClickTime=.04f; // Separate the deliberately rapid test gestures.
    io.ClearEventsQueue();io.AddFocusEvent(true);const int n=ui.frame();
    auto down=[&](const char* key){active=key;box=ui.items.at(key);libraryqa::point(ui,key);io.AddMouseButtonEvent(0,true);};
    if(n==5||n==12||n==18||n==36||n==44){if(n==12)io.AddKeyEvent(ImGuiMod_Shift,true);if(n==18)io.AddKeyEvent(ImGuiMod_Ctrl,true);down("数值 / Value");}
    if(n==24)down("位置 / Position.x");if(n==30)down("preview.zoom");
    if(n==6||n==13||n==19||n==25||n==31)io.AddMousePosEvent((box.x+box.z)*.5f+20*ui.scale(),(box.y+box.w)*.5f);
    if(n==7||n==14||n==20||n==26||n==32||n==37||n==45){io.AddMouseButtonEvent(0,false);io.AddKeyEvent(ImGuiMod_Shift,false);io.AddKeyEvent(ImGuiMod_Ctrl,false);}
    if(n==38)io.AddInputCharactersUTF8("405.5");if(n==40)io.AddKeyEvent(ImGuiKey_Escape,true);if(n==41)io.AddKeyEvent(ImGuiKey_Escape,false);
    if(n==46)io.AddInputCharactersUTF8("112.25");if(n==47)io.AddKeyEvent(ImGuiKey_Enter,true);if(n==48)io.AddKeyEvent(ImGuiKey_Enter,false);
    if(n==52){theme=0;io.AddMousePosEvent(5,5);}
    // The native backend polls real modifier state each frame. Reassert the
    // synthetic test state after it, without changing global OS key state.
    io.AddKeyEvent(ImGuiMod_Shift,n>=12&&n<=13);
    io.AddKeyEvent(ImGuiMod_Ctrl,n>=18&&n<=19);
  };
  hook.tick=[&](psui::LibraryPanel& ui){
    const int n=ui.frame();
    if(n==9)assert_value(value,120,"shared normal scrub");
    if(n==16)assert_value(value,320,"shared Shift scrub");
    if(n==22)assert_value(value,322,"shared Ctrl scrub");
    if(n==28){assert_value(x,120,"shared point scrub");assert_value(y,200,"other point coordinate unchanged");}
    if(n==34){assert_value(view.percent,initial_zoom+20,"shared zoom scrub");assert_value(value,322,"zoom does not alter effect value");}
    if(n==40)assert_value(value,405.5,"shared text editor");
    if(n==43)assert_value(value,322,"Escape cancels text without closing filter");
    if(n==50)assert_value(value,112.25,"Enter commits typed value");
    if(n==11||n==90){const auto name=std::string(high_dpi?"200-zh-":"100-en-")+(n==11?"dark.bmp":"light.bmp");libraryqa::require(ui.capture((folder/name).c_str()),"shared panel screenshot");}
    if(n==95)ui.close(true);
  };
  psui::set_panel_test_hook(&hook);
  const auto answer=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host,L"Shared numeric control QA",760,480,[&](psui::LibraryPanel& ui){
    ui.number("数值 / Value",value,0,1000,0,200);
    psui::point_control(ui,"位置 / Position",x,y);
    ImGui::Separator();ImGui::SetNextItemWidth(200*ui.scale());
    ImGui::BeginChild("view",{220*ui.scale(),150*ui.scale()});ui.preview_zoom(view);ImGui::EndChild();
  },"数值 位置",&info);
  psui::set_panel_test_hook(nullptr);
  if(answer!=IDOK||!hook.error.empty()){std::cerr<<"FAIL: panel="<<answer<<" error="<<hook.error<<'\n';return 1;}
  std::cout<<"PASS: production DX11 controls, "<<(high_dpi?200:100)<<"% DPI, "<<hook.host_locale<<", scalar/point/zoom; Shift/Ctrl; text; Escape stays in dialog; dark/light captures.\n";
}
