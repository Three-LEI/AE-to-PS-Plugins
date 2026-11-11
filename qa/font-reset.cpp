#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/library_panel.hpp"
#include "../native/tests/library_qa.hpp"
#include "../displacement/tests/mock_host.hpp"
#include <filesystem>
#include <iostream>
#include <thread>
int main(int argc,char** argv){
 if(argc!=3)return 2;const float scale=std::stof(argv[1]);const std::filesystem::path out(argv[2]);
 mock::Host host(32,32);host.layers[1].unicodeName=reinterpret_cast<const uint16*>(L"鎏金");
 auto layers=psui::host_layers(&host.f);libraryqa::require(layers.size()==2&&layers[1].name=="鎏金","Unicode layer name retained");
 host.layers[1].unicodeName=nullptr;host.layers[1].name=const_cast<char*>("鎏金");
 libraryqa::require(psui::host_layers(&host.f)[1].name=="鎏金","legacy UTF-8 layer names are not decoded as ANSI");
 host.layers[1].unicodeName=reinterpret_cast<const uint16*>(L"鎏金");
 struct Values{double first,second;}value{25,77},defaults{1,2};int selected=1;
 const char* names[]={layers[0].name.c_str(),layers[1].name.c_str()};
 psui::PanelTestHook hook;hook.software=true;hook.host_locale="zh_CN";hook.initial_scale=scale;hook.force_legacy_dpi=true;
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(3));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);io.ConfigInputTrickleEventQueue=false;const int n=ui.frame();if(n==3||n==10){libraryqa::point(ui,n==3?"数值 A / Value A.reset":"all");io.AddMouseButtonEvent(0,true);}if(n==4||n==11)io.AddMouseButtonEvent(0,false);};
 hook.tick=[&](psui::LibraryPanel& ui){const int n=ui.frame();if(n==2)libraryqa::require(ImGui::GetIO().FontDefault->FindGlyphNoFallback(0x938f)!=nullptr,"actual document-only glyph 鎏 is baked");if(n==7)libraryqa::require(value.first==1&&value.second==77,"single reset changes only its own parameter");if(n==8)libraryqa::require(ui.capture((out/(scale==1?"font-reset-100.bmp":"font-reset-200.bmp")).c_str()),"capture reset outline and glyph");if(n==14)libraryqa::require(value.first==1&&value.second==2,"global reset restores both parameters");if(n>18)ui.close(false);};
 psui::set_panel_test_hook(&hook);
 const auto result=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host.f,L"Font and reset QA",500,350,[&](psui::LibraryPanel& ui){ui.parameter_defaults(&value,&defaults,sizeof(value));ui.choice("图层 / Layer",selected,names,2);ui.number("数值 A / Value A",value.first,0,100,0,100);ui.number("数值 B / Value B",value.second,0,100,0,100);if(ui.button("all","全部重置 / Reset All"))value=defaults;},"图层 数值");
 psui::set_panel_test_hook(nullptr);if(result!=IDCANCEL||!hook.error.empty()){std::cerr<<"FAIL: "<<hook.error<<'\n';return 1;}
 std::cout<<"PASS: document-only glyph 鎏; UTF-8 legacy layer name; reference reset outline; independent/all reset; legacy DPI scale="<<scale<<'\n';
}
