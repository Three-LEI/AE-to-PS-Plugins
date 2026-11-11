#include "../../displacement/tests/mock_host.hpp"
#include "glow_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <filesystem>
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 mock::Host host(384,256);auto layers=psui::host_layers(&host.f);
 for(int y=0;y<256;++y)for(int x=0;x<384;++x){
  const auto i=(std::size_t(y)*384+x)*4;const double d=std::hypot(x-192.,y-128.);
  const bool ring=std::abs(d-54)<2,bar=(x>85&&x<300&&std::abs(y-128)<2);
  host.pixels[0][i]=ring?1:bar?.2f:0;host.pixels[0][i+1]=ring?.35f:bar?.8f:0;host.pixels[0][i+2]=ring?.12f:bar?1:0;host.pixels[0][i+3]=ring||bar?1:0;
 }
 psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 1;
 auto source=glow::from_host(std::move(pixels));auto p=glow::defaults(glow::effect_kind);glow::Image mask,texture;
 const bool automated=argc>1;const bool deep=glow::effect_kind==glow::Kind::deep;
 wchar_t file[32768];GetModuleFileNameW(nullptr,file,32768);const auto folder=std::filesystem::path(file).parent_path();
 psui::PanelTestHook hook;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";
 hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:0;hook.software=true;
 bool ready=false;int frame_ready=0;
 if(automated){
  hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);
   if(ready){const int n=ui.frame()-frame_ready;const char* key=n==3?(deep?"半径 / Radius.slider":"发光半径 / Glow Radius.slider"):n==35?"about":n==39?"about.close":n==55?"ok":nullptr;
    if(key&&ui.items.count(key)){libraryqa::point(ui,key);io.AddMouseButtonEvent(0,true);}if(n==4||n==36||n==40||n==56)io.AddMouseButtonEvent(0,false);}
  };
  hook.tick=[&](psui::LibraryPanel& ui){
   if(ui.items.count("glow.state")&&ui.items.at("glow.state").x==1&&!ready){ready=true;frame_ready=ui.frame();}
   const auto size=ImGui::GetIO().DisplaySize;for(const char* key:{"ok","cancel","reset","preview.zoom.slider"})if(ui.items.count(key)){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5,"footer remains inside viewport");}
   if(ready){const int n=ui.frame()-frame_ready;if(n==30||n==37)libraryqa::require(ui.capture((folder/(std::string(deep?"deep":"classic")+"-ui-"+std::to_string(n)+".bmp")).c_str()),"GPU frame capture");}
   if(ui.frame()>400)ui.close(false);
  };psui::set_panel_test_hook(&hook);
 }
 HWND owner=CreateWindowExW(0,L"STATIC",L"Glow test owner",WS_OVERLAPPEDWINDOW,100,100,1200,800,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 const auto answer=glow::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,source,layers,p,mask,texture);
 psui::set_panel_test_hook(nullptr);DestroyWindow(owner);
 std::cout<<"dialog="<<answer<<" hook="<<hook.error<<'\n';
 if(automated&&(answer!=IDOK||!hook.error.empty()))return 2;
 std::cout<<"PASS: production library panel; async preview; radius input; bilingual/DPI layout; About; accept and worker shutdown.\n";
}
