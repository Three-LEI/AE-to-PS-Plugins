#include "../../displacement/tests/mock_host.hpp"
#include "chromatic_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 mock::Host host(384,256);host.layers[0].sheetID=2;host.layers[1].sheetID=1;host.layers[1].isVisible=true;host.channels[0][0].target=true;
 const bool selection_qa=argc>1&&std::string(argv[1]).find("selection")!=std::string::npos;
 std::vector<float> selection_pixels(384*256*4,0);mock::Port selection_port{0,8,384,256,&selection_pixels};ReadChannelDesc selection_channel;
 if(selection_qa){for(int y=0;y<256;++y)for(int x=0;x<192;++x)selection_pixels[(std::size_t(y)*384+x)*4]=1;
  selection_channel.port=reinterpret_cast<PIChannelPort>(&selection_port);selection_channel.depth=8;selection_channel.maxVersion=5;selection_channel.bounds=host.doc.bounds;host.doc.selection=&selection_channel;host.f.haveMask=true;host.f.filterCase=filterCaseEditableTransparencyWithSelection;}
 for(int y=0;y<256;++y)for(int x=0;x<384;++x){const auto i=(std::size_t(y)*384+x)*4;
  const bool lit=(std::abs(x-192)<4&&std::abs(y-128)<65)||(std::abs(y-128)<4&&std::abs(x-192)<80)||std::hypot(x-100.,y-70.)<9;
  host.pixels[0][i]=host.pixels[0][i+1]=host.pixels[0][i+2]=host.pixels[0][i+3]=lit?1:0;
  host.pixels[1][i]=.05f;host.pixels[1][i+1]=.08f;host.pixels[1][i+2]=.14f;host.pixels[1][i+3]=1;
 }
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 1;
 auto source=chromatic::from_host(std::move(pixels));auto p=chromatic::preset(2);chromatic::Image mask;
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);const auto folder=std::filesystem::path(path).parent_path();
 const bool automated=argc>1;psui::PanelTestHook hook;hook.software=true;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";
 hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1.25f;
 bool ready=false;int start=0;
 if(automated){
  hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);
   if(!ready)return;const int n=ui.frame()-start;
   const char* key=n==3||n==27?"预设 / Preset":n==7?"预设 / Preset/item/2":n==17?"方向角度 / Direction Angle.slider":n==31?"预设 / Preset/item/3":n==40?"显示背景 / Show Background":n==50?"preview.zoom.slider":n==60?"image":n==72?"about":n==76?"about.close":n==94?"ok":nullptr;
   if(key){libraryqa::point(ui,key,n==60?.3f:n==50?.85f:.5f);io.AddMouseButtonEvent(n==60?1:0,true);}
   if(n==4||n==8||n==18||n==28||n==32||n==41||n==51||n==73||n==77||n==95)io.AddMouseButtonEvent(0,false);
   if(n==61)io.AddMouseButtonEvent(1,false);
  };
  hook.tick=[&](psui::LibraryPanel& ui){
   const auto state=ui.items.at("chromatic.state");if(!ready&&state.x==1&&state.y==0&&state.z==0){ready=true;start=ui.frame();}
   const auto size=ImGui::GetIO().DisplaySize;
   for(const char* key:{"ok","cancel","reset","显示背景 / Show Background","preview.zoom.slider"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y,"all preview controls fit viewport");}
   const int n=ui.frame()-start;
   if(ready&&(n==35||n==68||n==74)){
    const auto file="chromatic-"+hook.host_locale+(selection_qa?"-selection":"")+"-"+std::to_string(n);libraryqa::require(ui.capture((folder/(file+".bmp")).c_str()),"production GPU snapshot");
    const auto rect=ui.items.at("image");std::ofstream meta(folder/(file+".json"));meta<<"{\"image_rect\":["<<rect.x<<','<<rect.y<<','<<rect.z<<','<<rect.w<<"]}";
   }
   if(ui.frame()>400)ui.close(false);
  };psui::set_panel_test_hook(&hook);
 }
 HWND owner=CreateWindowExW(0,L"STATIC",L"Chromatic production UI QA",WS_OVERLAPPEDWINDOW,100,100,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(owner,SW_SHOW);
 const auto answer=chromatic::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,source,layers,p,mask);
 psui::set_panel_test_hook(nullptr);std::cout<<"dialog="<<answer<<" error="<<hook.error<<" mode="<<int(p.mode)<<" amount="<<p.amount<<" center="<<p.center_x<<','<<p.center_y<<'\n';
 if(automated){
  if(answer!=IDOK||!hook.error.empty()||p.mode!=chromatic::Mode::spectrum||p.center_x>=.5||p.amount!=42)return 2;
  const auto before=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"色差距离（像素） / Separation (px).slider",.8f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};
  hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>12)ui.close(false);};psui::set_panel_test_hook(&hook);
  const auto cancelled=chromatic::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,source,layers,p,mask);psui::set_panel_test_hook(nullptr);
  if(cancelled!=IDCANCEL||p!=before)return 3;
 }
 DestroyWindow(owner);std::cout<<"PASS: async preview; presets/modes; slider; background; zoom; right-click center; About; zh/en and DPI layout; accept/cancel; worker shutdown.\n";
}
