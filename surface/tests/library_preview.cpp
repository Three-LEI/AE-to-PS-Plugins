#include "../../displacement/tests/mock_host.hpp"
#include "surface_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);mock::Host host(384,256);
 for(int y=0;y<256;++y)for(int x=0;x<384;++x){auto i=(std::size_t(y)*384+x)*4;host.pixels[0][i]=x/383.f;host.pixels[0][i+1]=y/255.f;host.pixels[0][i+2]=float(.5+.4*std::sin(x*.15)*std::cos(y*.12));host.pixels[0][i+3]=.7f;}
 psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;++k,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=surface::from_host(std::move(pixels));auto layers=psui::host_layers(&host.f);const auto factory=surface::defaults(surface::effect_kind,384,256);auto p=factory;p.light_type=1;surface::Image map;
 psui::PanelTestHook hook;hook.software=true;hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";
 const auto folder=std::filesystem::path("evidence/surface-v01/ui");std::filesystem::create_directories(folder);const std::string tag=std::string(surface::effect_kind==surface::Kind::blob?"blob":"glass")+(argc>1?argv[1]:"interactive");
 bool ready=false;int start=0;
 if(argc>1){hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(!ready)return;const int n=ui.frame()-start;
  const char* key=n==3?"柔和度 / Softness.slider":n==12?"柔和度 / Softness.reset":n==20?"显示背景 / Show Background":n==26?"preview.zoom.slider":n==35?"about":n==39?"about.close":n==60?"ok":nullptr;
  if(key){libraryqa::point(ui,key,n==3?.7f:.5f);io.AddMouseButtonEvent(0,true);}if(n==4||n==13||n==21||n==27||n==36||n==40||n==61)io.AddMouseButtonEvent(0,false);
 };hook.tick=[&](psui::LibraryPanel& ui){const auto state=ui.items.at("surface.state");if(!ready&&state.x==1&&state.y==0&&state.z==0){ready=true;start=ui.frame();}const auto size=ImGui::GetIO().DisplaySize;for(const char* key:{"ok","cancel","reset"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5,"footer fits viewport");}libraryqa::require(!ui.items.count("使用 / Using")&&!ui.items.count("AE Lights"),"only Effect Light controls");if(ready&&ui.frame()-start==18)libraryqa::require(ui.capture((folder/(tag+".bmp")).c_str()),"capture production surface panel");if(ui.frame()>600)ui.close(false);};psui::set_panel_test_hook(&hook);}
 const auto answer=surface::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);psui::set_panel_test_hook(nullptr);
 if(argc>1){auto expected=factory;expected.light_type=1;if(answer!=IDOK||!hook.error.empty()||p!=expected)return 2;
  const auto saved=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,"柔和度 / Softness.slider",.9f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>10)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancelled=surface::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);psui::set_panel_test_hook(nullptr);if(cancelled!=IDCANCEL||p!=saved)return 3;
  p.softness=80;p.map_key=layers.back().name_key;p.light_color={.1,1,.3};p.light_type=1;p.light_x=10;p.light_y=20;p.direction=77;p.roughness=.3;p.metal=0;p.ambient=17;p.height=-35;p.displacement=230;p.cut=60;
  hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>14)ui.close(true);};psui::set_panel_test_hook(&hook);const auto reset=surface::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);psui::set_panel_test_hook(nullptr);if(reset!=IDOK||p!=factory||map.valid())return 4;
 }
 std::cout<<"PASS: production surface panel; Effect Light only; async preview; parameter/single reset isolation; point-light values; background/zoom; About; language/DPI; cancel; all-reset restores document coordinates, color and map.\n";
}
