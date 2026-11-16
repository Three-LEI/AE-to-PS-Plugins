#include "../../displacement/tests/mock_host.hpp"
#include "energy_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 const bool chinese=argc>1&&std::string(argv[1])=="zh";mock::Host host(1024,640);for(auto& v:host.pixels[0])v=0;auto p=energy::defaults(1024,640);auto factory=p;energy::Inputs input;auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;auto src=energy::from_host(std::move(pixels));
 psui::PanelTestHook hook;hook.software=true;hook.host_locale=chinese?"zh_CN":"en_US";hook.initial_scale=chinese?2:1;hook.force_legacy_dpi=chinese;int start=-1,drafts=0;float initial=0,full=0;bool complete=false;
 const std::string control="光晕半径（像素） / Glow Radius (px)";
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(7));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(start<0)return;int n=ui.frame()-start;
  if(n>=3&&n<=45){libraryqa::point(ui,control+".slider",.12f+(n-3)*.009f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);
  if(n==90){libraryqa::point(ui,control+".reset");io.AddMouseButtonEvent(0,true);}if(n==91)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){auto a=ui.items.at("adaptive.state"),state=ui.items.at("energy.state");libraryqa::require(state.y==0,"energy preview succeeds");const auto size=ImGui::GetIO().DisplaySize;for(auto key:{"ok","cancel","reset","交互快速预览 / Adaptive Preview"}){auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5f,"controls within viewport");}
  if(chinese)for(auto glyph:{0x80FD,0x91CF,0x5149,0x675F,0x8DEF,0x5F84})libraryqa::require(ImGui::GetFont()->FindGlyphNoFallback(ImWchar(glyph)),"Chinese beam glyphs available");
  if(start<0&&a.y>=1&&a.w==0){start=ui.frame();initial=a.x;full=a.y;}
  if(start>=0){int n=ui.frame()-start;if(n==45){drafts=int(a.x-initial);libraryqa::require(drafts>=2,"drafts publish during continuous drag");}if(n>135&&a.y>full+1&&a.z==0&&a.w==0){if(argc>2)ui.capture(std::filesystem::path(argv[2]).c_str());complete=true;ui.close(true);}}
  if(ui.frame()>1000)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);auto answer=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p,input);psui::set_panel_test_hook(nullptr);if(answer!=IDOK||!complete||p!=factory||!hook.error.empty()){std::cerr<<"FAIL panel "<<hook.error<<" reset="<<(p==factory)<<'\n';return 3;}
 hook.before_frame=[&](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,control+".slider",.8f);io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>20)ui.close(false);};psui::set_panel_test_hook(&hook);answer=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p,input);psui::set_panel_test_hook(nullptr);if(answer!=IDCANCEL||p!=factory)return 4;
 std::cout<<"PASS Energy UI "<<(chinese?"zh200 legacy DPI":"en100")<<"; "<<drafts<<" draft frames, full settle, parameter reset and cancel\n";
}
