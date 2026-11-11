#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/library_panel.hpp"
#include "../native/tests/library_qa.hpp"
#include "PIProperties.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
int theme=3;
OSErr property(PIType signature,PIType key,int32,intptr_t* value,Handle*){
 if(signature!=0x3842494d||key!=propUIBrightness||!value)return -1;*value=theme;return noErr;
}
int main(int argc,char** argv){
 if(argc!=5&&argc!=6)return 2;SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 theme=std::stoi(argv[2]);const float scale=std::stof(argv[3]);const std::filesystem::path folder(argv[4]);std::filesystem::create_directories(folder);
 psui::PanelTestHook hook;hook.software=true;hook.host_locale=argv[1];hook.initial_scale=scale;hook.force_legacy_dpi=argc==6;
 PropertyProcs properties{};properties.getPropertyProc=property;FilterRecord host{};host.propertyProcs=&properties;
 const psui::PluginInfo info{"Roughen Edges 毛边","0.1"};int calls=0;bool legacy=false;double value=42.125;bool saw_error=false;
 hook.open_url=[&](const char* url){libraryqa::require(std::string(url)==psui::source_repository,"click targets configured public repository");++calls;return legacy||calls==1;};
 hook.before_frame=[&](psui::LibraryPanel& ui){
  std::this_thread::sleep_for(std::chrono::milliseconds(3));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);const int n=ui.frame();
  const char* key=legacy?(n==3?"about.source":n==10?"about.close":nullptr):(n==3||n==20?"about":n==8||n==24?"about.source":n==16?"about.close":nullptr);
  if(key){libraryqa::point(ui,key);io.AddMouseButtonEvent(0,true);}
  if(n==4||n==9||n==11||n==17||n==21||n==25)io.AddMouseButtonEvent(0,false);
  if(!legacy&&n==34)io.AddKeyEvent(ImGuiKey_Escape,true);
  if(!legacy&&n==35)io.AddKeyEvent(ImGuiKey_Escape,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){
  const int n=ui.frame();if(ui.items.count("about.source")){
   const auto blue=ui.items.at("about.source.color");libraryqa::require(blue.z>blue.y&&blue.y>blue.x,"source label uses theme-aware parameter blue");
   const auto r=ui.items.at("about.source");const auto size=ImGui::GetIO().DisplaySize;libraryqa::require(r.x>=0&&r.y>=0&&r.z<=size.x&&r.w<=size.y,"source link fits viewport");
  }
  if(!legacy&&n==12)libraryqa::require(calls==1&&!ui.items.count("about.source.error"),"successful click dispatches exactly once");
  if(!legacy&&n==28){libraryqa::require(calls==2&&ui.items.count("about.source.error")&&ui.items.count("about.source.copy"),"failed browser launch offers copy link");saw_error=true;}
  if(!legacy&&n==14&&theme==3&&scale==1&&std::string(argv[1])=="zh_CN")libraryqa::require(ui.capture((folder/"about-source-zh.bmp").c_str()),"capture one source-link panel");
  if(!legacy&&n==37)libraryqa::require(!ui.items.count("about.content"),"Escape closes About without changing effect");
  if(n>(legacy?20:40))ui.close(false);
 };
 psui::set_panel_test_hook(&hook);
 auto answer=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host,L"AE-to-PS About link QA",640,420,[&](psui::LibraryPanel& ui){ui.number("QA value",value,0,100,0,100);},info.name,&info);
 psui::set_panel_test_hook(nullptr);if(answer!=IDCANCEL||value!=42.125||calls!=2||!saw_error||!hook.error.empty()){std::cerr<<"FAIL panel: "<<hook.error<<" calls="<<calls<<'\n';return 3;}
 legacy=true;psui::set_panel_test_hook(&hook);psui::show_host_about(info,L"AE-to-PS Host About QA");psui::set_panel_test_hook(nullptr);
 if(calls!=3||!hook.error.empty()){std::cerr<<"FAIL host About: "<<hook.error<<'\n';return 4;}
 std::cout<<"PASS: About source URL; blue borderless link; success/failure; copy fallback; Escape/Close; effect unchanged; host About; "<<argv[1]<<" theme="<<theme<<" scale="<<scale<<" legacy_dpi="<<hook.force_legacy_dpi<<'\n';
}
