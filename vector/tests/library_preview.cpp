#include "../../displacement/tests/mock_host.hpp"
#include "vector_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 mock::Host host(384,256);host.layers[0].sheetID=2;host.layers[1].sheetID=1;host.layers[1].isVisible=true;host.channels[0][0].target=true;
 for(int y=0;y<256;++y)for(int x=0;x<384;++x){const auto i=(std::size_t(y)*384+x)*4;
  const float radius=std::hypot(x-192.f,y-128.f);const bool lit=(int(radius/12)%2==0&&radius<92)||(std::abs(x-90)<3&&y>70&&y<190);
  host.pixels[0][i]=lit?.95f:0;host.pixels[0][i+1]=lit?.45f:0;host.pixels[0][i+2]=lit?.15f:0;host.pixels[0][i+3]=lit?1:0;
  host.pixels[1][i]=.08f;host.pixels[1][i+1]=.1f;host.pixels[1][i+2]=.15f;host.pixels[1][i+3]=1;
 }
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 1;
 auto source=vectorblur::from_host(std::move(pixels));auto p=vectorblur::preset(2);vectorblur::Image map;
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);const auto folder=std::filesystem::path(path).parent_path();
 const bool automated=argc>1;psui::PanelTestHook hook;hook.software=true;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";
 hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1.25f;
 bool ready=false;int start=0;
 if(automated){
  hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);
   if(!ready)return;const int n=ui.frame()-start;
   const char* key=n==3?"预设 / Preset":n==7?"预设 / Preset/item/4":n==20?"数量（像素） / Amount (px).slider":n==30?"显示矢量 / Show Vectors":n==38?"显示背景 / Show Background":n==46?"preview.zoom.slider":n==56?"about":n==60?"about.close":n==78?"ok":nullptr;
   if(key){libraryqa::point(ui,key,n==20?.65f:n==46?.7f:.5f);io.AddMouseButtonEvent(0,true);}
   if(n==4||n==8||n==21||n==31||n==39||n==47||n==57||n==61||n==79)io.AddMouseButtonEvent(0,false);
   if(n==48)io.AddMousePosEvent(5,5);
  };
  hook.tick=[&](psui::LibraryPanel& ui){
   const auto state=ui.items.at("vector.state");if(!ready&&state.x==1&&state.y==0&&state.z==0){ready=true;start=ui.frame();}
   const auto size=ImGui::GetIO().DisplaySize;
   for(const char* key:{"ok","cancel","reset","显示背景 / Show Background","preview.zoom.slider","显示矢量 / Show Vectors"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y,"all preview controls fit viewport");}
   const int n=ui.frame()-start;
   if(ready&&n==60)libraryqa::require(ui.items.count("about.content")&&ui.items.count("about.close"),"shared About modal opens");
   if(ready&&n==65)libraryqa::require(!ui.items.count("about.content"),"About closes without closing filter");
   if(ready&&(n==15||n==52||n==60)){
    const auto file="vector-"+hook.host_locale+"-"+std::to_string(n);libraryqa::require(ui.capture((folder/(file+".bmp")).c_str()),"production GPU snapshot");
   }
   if(ui.frame()>400)ui.close(false);
  };psui::set_panel_test_hook(&hook);
 }
 HWND owner=CreateWindowExW(0,L"STATIC",L"Vector Blur production UI QA",WS_OVERLAPPEDWINDOW,100,100,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(owner,SW_SHOW);
 const auto answer=vectorblur::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,source,layers,p,map);
 psui::set_panel_test_hook(nullptr);std::cout<<"dialog="<<answer<<" error="<<hook.error<<" type="<<int(p.type)<<" amount="<<p.amount<<'\n';
 if(automated){
  if(answer!=IDOK||!hook.error.empty()||p.type!=vectorblur::Type::direction_fading||p.amount<50||p.amount>80)return 2;
  const auto before=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"数量（像素） / Amount (px).slider",.8f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};
  hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>12)ui.close(false);};psui::set_panel_test_hook(&hook);
  const auto cancelled=vectorblur::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,source,layers,p,map);psui::set_panel_test_hook(nullptr);
  if(cancelled!=IDCANCEL||p!=before)return 3;
 }
 DestroyWindow(owner);std::cout<<"PASS: production async preview; preset/type; slider; vectors; background; zoom; About; zh/en and DPI; accept/cancel; worker shutdown.\n";
}
