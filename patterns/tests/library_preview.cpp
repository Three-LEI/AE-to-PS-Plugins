#include "../../displacement/tests/mock_host.hpp"
#include "pattern_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 mock::Host host(384,256);host.layers[0].sheetID=2;host.layers[1].sheetID=1;host.layers[1].isVisible=true;host.channels[0][0].target=true;
 for(int y=0;y<256;++y)for(int x=0;x<384;++x){const auto i=(std::size_t(y)*384+x)*4;host.pixels[0][i]=.2f;host.pixels[0][i+1]=.4f;host.pixels[0][i+2]=.6f;host.pixels[0][i+3]=.7f;host.pixels[1][i]=.08f;host.pixels[1][i+1]=.12f;host.pixels[1][i+2]=.2f;host.pixels[1][i+3]=1;}
 auto p=patterns::defaults_for_host(&host.f);
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);const auto folder=std::filesystem::path(path).parent_path();
 const bool automated=argc>1;psui::PanelTestHook hook;hook.software=true;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1.25f;
 bool ready=false;int start=0;
 if(automated){
  hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);if(!ready)return;
   const int n=ui.frame()-start;const char* key=n==3?"预设 / Preset":n==7?"预设 / Preset/item/3":n==20?"不透明度（%） / Opacity (%).slider":n==30?(pattern_kind==patterns::Kind::grid?"反转网格 / Invert Grid":"交换棋格 / Swap Cells"):n==38?"显示背景 / Show Background":n==46?"preview.zoom.slider":n==56?"about":n==60?"about.close":n==78?"ok":nullptr;
   if(key){libraryqa::point(ui,key,n==20?.65f:n==46?.7f:.5f);io.AddMouseButtonEvent(0,true);}
   if(n==4||n==8||n==21||n==31||n==39||n==47||n==57||n==61||n==79)io.AddMouseButtonEvent(0,false);if(n==48)io.AddMousePosEvent(5,5);
  };
  hook.tick=[&](psui::LibraryPanel& ui){const auto state=ui.items.at("pattern.state");if(!ready&&state.x==1&&state.y==0&&state.z==0){ready=true;start=ui.frame();}
   const auto size=ImGui::GetIO().DisplaySize;for(const char* key:{"ok","cancel","reset","显示背景 / Show Background","preview.zoom.slider"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y,"preview/footer fit viewport");}
   const int n=ui.frame()-start;if(ready&&n==60)libraryqa::require(ui.items.count("about.content"),"About opens");if(ready&&n==65)libraryqa::require(!ui.items.count("about.content"),"About closes");
   if(ready&&(n==15||n==52||n==60)){const auto file=std::string(pattern_kind==patterns::Kind::grid?"grid-":"checker-")+hook.host_locale+'-'+std::to_string(n)+".bmp";libraryqa::require(ui.capture((folder/file).c_str()),"production GPU capture");}
   if(ui.frame()>400)ui.close(false);
  };psui::set_panel_test_hook(&hook);
 }
 HWND owner=CreateWindowExW(0,L"STATIC",L"Pattern production UI QA",WS_OVERLAPPEDWINDOW,100,100,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(owner,SW_SHOW);
 const auto answer=patterns::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,p);psui::set_panel_test_hook(nullptr);
 std::cout<<"dialog="<<answer<<" error="<<hook.error<<" opacity="<<p.opacity<<" invert="<<p.invert<<'\n';
 if(automated){
  if(answer!=IDOK||!hook.error.empty()||!p.invert||p.opacity<50||p.opacity>80||p.size_from!=patterns::SizeFrom::width_height)return 2;
  const auto before=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"宽度 / Width.slider",.8f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>12)ui.close(false);};psui::set_panel_test_hook(&hook);
  const auto cancelled=patterns::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,p);psui::set_panel_test_hook(nullptr);if(cancelled!=IDCANCEL||p!=before)return 3;
 }DestroyWindow(owner);std::cout<<"PASS: shared point/colour layout; generator preview; preset/opacity/invert; background/zoom; About; language/DPI; accept/cancel.\n";
}
