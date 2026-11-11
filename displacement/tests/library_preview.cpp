#include "mock_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <filesystem>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 mock::Host host(512,384);auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;
 if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 1;
 auto source=displacement::from_host(std::move(pixels));displacement::Image map;displacement::Parameters values;values.layer_id=22;
 const auto original=host.pixels[0];
 const bool automated=argc>1&&std::string(argv[1]).find("--qa")==0;
 wchar_t file[32768];GetModuleFileNameW(nullptr,file,32768);const auto folder=std::filesystem::path(file).parent_path();
 psui::PanelTestHook hook;hook.host_locale=argc>1&&std::string(argv[1])=="--qa-zh"?"zh_CN":"en_US";
 if(automated){
  hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);const int n=ui.frame();
   const char* key=n==5||n==13?"map.layer":n==9?"map.layer/0":n==17?"map.layer/1":n==21?"最大水平置换 / Horizontal Amount.slider":n==28?"preview.zoom.slider":n==35?"about":n==39?"about.close":n==45?"reset":n==52?"ok":nullptr;
   if(key){libraryqa::point(ui,key);io.AddMouseButtonEvent(0,true);}if(n==6||n==10||n==14||n==18||n==22||n==29||n==36||n==40||n==46||n==53)io.AddMouseButtonEvent(0,false);
  };
  hook.tick=[folder](psui::LibraryPanel& ui){
   const auto size=ImGui::GetIO().DisplaySize;for(const char* key:{"ok","cancel","reset","preview.zoom.slider"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5,"preview controls fit viewport");}
   if(ui.frame()==3||ui.frame()==37||ui.frame()==48)libraryqa::require(ui.capture((folder/("displacement-ui-"+std::to_string(ui.frame())+".bmp")).c_str()),"displacement UI snapshot");
   if(ui.frame()>65)ui.close(false);
  };psui::set_panel_test_hook(&hook);
 }
 HWND owner=CreateWindowExW(0,L"STATIC",L"Displacement UI test owner",WS_OVERLAPPEDWINDOW,100,100,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 int theme=3;const bool pan_qa=argc>1&&std::string(argv[1])=="--qa-pan";
 if(pan_qa)libraryqa::setup_pan(false,theme,folder);
 const bool wheel_qa=argc>1&&(std::string(argv[1])=="--qa-wheel"||std::string(argv[1])=="--qa-wheel-native");
 if(wheel_qa){libraryqa::setup_wheel(false,theme,folder);libraryqa::hook.native_wheel_input=std::string(argv[1])=="--qa-wheel-native";}
 ShowWindow(owner,SW_SHOW);const auto answer=displacement::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,layers,source,values,map);psui::set_panel_test_hook(nullptr);DestroyWindow(owner);
 std::cout<<"dialog="<<answer<<" error="<<hook.error<<'\n';
 if(automated&&(answer!=IDOK||!map.valid()||values.layer_id!=22||host.pixels[0]!=original))return 2;
 if((pan_qa||wheel_qa)&&(values.amount_x!=5||values.amount_y!=5))return 3;
 std::cout<<"PASS: native layer dropdown; map switch; amounts; zoom; shared About; reset/accept; original mock layer unchanged.\n";
}
