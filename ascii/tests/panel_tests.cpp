#include "../../displacement/tests/mock_host.hpp"
#include "ascii_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
int main(int argc,char** argv){
 const bool zh=argc>1&&std::string(argv[1])=="zh";mock::Host host(1024,640);
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;
 auto source=asciifx::from_host(std::move(pixels));asciifx::Parameters p;const auto initial=p;
 psui::PanelTestHook hook;hook.software=true;hook.host_locale=zh?"zh_CN":"en_US";hook.initial_scale=zh?2:1;hook.force_legacy_dpi=zh;
 int start=-1,drafts=0;float first=0,full=0;bool done=false;
 const std::string control="字符宽度（像素） / Cell Width (px)";
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(start<0)return;const int n=ui.frame()-start;
  if(n>=3&&n<=45){libraryqa::point(ui,control+".slider",.2f+(n-3)*.01f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);
  if(n==85){libraryqa::point(ui,control+".reset");io.AddMouseButtonEvent(0,true);}if(n==86)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){libraryqa::actions_fit(ui);const auto a=ui.items.at("adaptive.state"),state=ui.items.at("ascii.state");libraryqa::require(state.y==0,"valid preview");
  if(start<0&&a.y>=1&&a.w==0){start=ui.frame();first=a.x;full=a.y;}
  if(start>=0){const int n=ui.frame()-start;if(n==45){drafts=int(a.x-first);libraryqa::require(drafts>=2&&state.z>4,"continuous drag updates characters");}if(n>140&&a.y>full+1&&a.w==0){done=true;if(argc>2)ui.capture(std::filesystem::path(argv[2]).c_str());ui.close(true);}}
  if(ui.frame()>900)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto result=asciifx::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p);psui::set_panel_test_hook(nullptr);
 if(result!=IDOK||!done||p!=initial||!hook.error.empty()){std::cerr<<"FAIL asciifx UI "<<hook.error<<'\n';return 3;}
 auto saved=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,"预设 / Preset");io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>18)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancel=asciifx::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p);psui::set_panel_test_hook(nullptr);if(cancel!=IDCANCEL||p!=saved)return 4;
 std::cout<<"PASS asciifx UI "<<(zh?"zh200 legacy DPI":"en100")<<": "<<drafts<<" drag frames; full refinement; single reset/cancel; taller footer/Fit layout\n";
}
