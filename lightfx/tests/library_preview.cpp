#include "../../displacement/tests/mock_host.hpp"
#include "lightfx_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 mock::Host host(768,512);for(int y=0;y<512;y++)for(int x=0;x<768;x++){const auto i=(std::size_t(y)*768+x)*4;const bool bright=(x-330)*(x-330)+(y-210)*(y-210)<900||(x>160&&x<580&&y>300&&y<315);host.pixels[0][i]=bright?2.f:.03f;host.pixels[0][i+1]=bright?1.5f:.04f;host.pixels[0][i+2]=bright?1.f:.07f;host.pixels[0][i+3]=bright?1.f:.1f;}
 psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=lightfx::from_host(std::move(pixels));const auto factory=lightfx::defaults(lightfx::effect_kind,768,512);auto p=factory;
 psui::PanelTestHook hook;hook.software=true;const std::string option=argc>1?argv[1]:"zh100";hook.initial_scale=option.find("200")!=std::string::npos?2:1;hook.host_locale=option.find("zh")!=std::string::npos?"zh_CN":"en_US";hook.force_legacy_dpi=option.find("legacy")!=std::string::npos;
 const auto folder=std::filesystem::path(argc>2?argv[2]:"evidence/lightfx-v01/ui");std::filesystem::create_directories(folder);const auto tag=std::to_string(int(lightfx::effect_kind))+"-"+option;
 const char* control=lightfx::effect_kind==lightfx::Kind::starglow?"星芒长度（像素） / Streak Length (px)":"光束长度 / Ray Length";
 int start=0,initial_frames=0,drag_frames=0;bool started=false,reset=false;double max_tick=0;
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(6));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(!started)return;const int n=ui.frame()-start;
  if(n>=3&&n<=45){
   if(lightfx::effect_kind==lightfx::Kind::shine){const auto r=ui.items.at("image");io.AddMousePosEvent(r.x+(r.z-r.x)*(.2f+float(n-3)/100),r.y+(r.w-r.y)*(.4f+.1f*std::sin(n*.2f)));if(n==3)io.AddMouseButtonEvent(1,true);}
   else{libraryqa::point(ui,std::string(control)+".slider",.2f+float(n-3)/100);if(n==3)io.AddMouseButtonEvent(0,true);}
  }
  if(n==46){io.AddMouseButtonEvent(0,false);io.AddMouseButtonEvent(1,false);}
  if(n==70){libraryqa::point(ui,std::string(control)+".reset");io.AddMouseButtonEvent(0,true);}if(n==71)io.AddMouseButtonEvent(0,false);
  if(n==85){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(n==86)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){
  libraryqa::actions_fit(ui);
  const auto state=ui.items.at("lightfx.state");libraryqa::require(state.w==0,"production preview has no error");const auto size=ImGui::GetIO().DisplaySize;
  for(const char* key:{"ok","cancel","reset"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5,"footer fits viewport");}
  if(!started&&state.y>=1){started=true;start=ui.frame();initial_frames=int(state.x);}
  if(started){const int n=ui.frame()-start;if(n==44){drag_frames=int(state.x)-initial_frames;libraryqa::require(drag_frames>=3,"continuous dragging produces new preview frames");}
   if(n==78)libraryqa::require(ui.capture((folder/(tag+".bmp")).c_str()),"capture representative panel");
   if(n>=95&&state.y>=2&&state.z==0){reset=true;ui.close(true);}
  }
  if(ui.frame()>700)ui.close(false);(void)max_tick;
 };
 psui::set_panel_test_hook(&hook);const auto answer=lightfx::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p);psui::set_panel_test_hook(nullptr);
 if(answer!=IDOK||!reset||!hook.error.empty()||p!=factory){std::cerr<<"FAIL: panel="<<answer<<" "<<hook.error<<" reset="<<reset<<'\n';return 2;}
 const auto saved=p;hook.before_frame=[&](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,std::string(control)+".slider",.8f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>15)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancelled=lightfx::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p);psui::set_panel_test_hook(nullptr);if(cancelled!=IDCANCEL||p!=saved)return 3;
 std::cout<<"PASS: production "<<(lightfx::effect_kind==lightfx::Kind::shine?"Shine":"Starglow")<<" panel; continuous drag frames="<<drag_frames<<"; automatic full-quality refinement; reset/cancel; locale/DPI; legacy="<<hook.force_legacy_dpi<<'\n';
}
