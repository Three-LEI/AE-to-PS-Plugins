#include "../../displacement/tests/mock_host.hpp"
#include "chromatic_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <thread>
#include <chrono>
#include <iostream>
int main(int argc,char** argv){
 if(argc<3)return 2;const bool defocus=std::string(argv[1])=="defocus";const auto factory=chromatic::preset(defocus?6:4);auto p=factory;
 mock::Host host(1024,640);for(int y=0;y<640;y++)for(int x=0;x<1024;x++){auto i=(std::size_t(y)*1024+x)*4;const bool lit=std::abs(y-320)<4||std::abs(x-512)<4||std::hypot(x-320.,y-190.)<13;for(int k=0;k<4;k++)host.pixels[0][i+k]=lit?1:0;}
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 3;auto source=chromatic::from_host(std::move(pixels));chromatic::Image mask;
 psui::PanelTestHook hook;hook.software=true;const std::string profile=argv[2];hook.host_locale=profile.find("zh")!=std::string::npos?"zh_CN":"en_US";hook.initial_scale=profile.find("200")!=std::string::npos?2:1;hook.force_legacy_dpi=profile.find("legacy")!=std::string::npos;
 const std::string control=defocus?"散焦半径（像素） / Defocus Radius (px)":"变换强度（%） / Transform Amount (%)";bool ready=false,complete=false;int start=0,frames=0;float initial=0,full=0;
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(!ready)return;int n=ui.frame()-start;
  if(n>=3&&n<=45){libraryqa::point(ui,control+".slider",.2f+(n-3)*.009f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);
  if(n==90){libraryqa::point(ui,control+".reset");io.AddMouseButtonEvent(0,true);}if(n==91)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){const auto a=ui.items.at("adaptive.state"),state=ui.items.at("chromatic.state");libraryqa::require(state.w==0,"new-mode preview has no error");const auto size=ImGui::GetIO().DisplaySize;
  for(const char* key:{"ok","cancel","reset","交互快速预览 / Adaptive Preview"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5f,"new-mode controls fit viewport");}
  if(hook.host_locale=="zh_CN")for(auto code:{0x8C31,0x53D8,0x6362,0x68F1,0x955C,0x6563,0x7126,0x5708,0x73AF})libraryqa::require(ImGui::GetFont()->FindGlyphNoFallback(ImWchar(code))!=nullptr,"new Chinese glyph included");
  if(!ready&&a.y>=1&&a.w==0){ready=true;start=ui.frame();initial=a.x;full=a.y;}
  if(ready){const int n=ui.frame()-start;if(n==45){frames=int(a.x-initial);libraryqa::require(frames>=2,"new modes produce drafts while dragging");}if(n>135&&a.y>full+1&&a.z==0&&a.w==0){complete=true;ui.close(true);}}
  if(ui.frame()>1000)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto answer=chromatic::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,mask);psui::set_panel_test_hook(nullptr);
 if(answer!=IDOK||!complete||!hook.error.empty()||p!=factory){std::cerr<<"FAIL extended panel "<<defocus<<" answer="<<answer<<" complete="<<complete<<" error="<<hook.error<<" reset="<<(p==factory)<<'\n';return 4;}
 const auto saved=p;hook.before_frame=[&](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,control+".slider",.8f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>15)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancelled=chromatic::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,mask);psui::set_panel_test_hook(nullptr);if(cancelled!=IDCANCEL||p!=saved)return 5;
 std::cout<<"PASS extended panel: "<<(defocus?"defocus":"warp")<<" profile="<<profile<<" drag frames="<<frames<<"; automatic full refinement; parameter reset/cancel; Chinese glyphs; viewport/legacy DPI.\n";
}
