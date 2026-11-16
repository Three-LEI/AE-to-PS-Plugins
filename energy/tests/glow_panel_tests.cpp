#include "../../displacement/tests/mock_host.hpp"
#include "energy_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 const bool chinese=argc>1&&std::string(argv[1])=="zh";
 mock::Host host(1024,640);for(auto& value:host.pixels[0])value=0;
 for(int y=180;y<460;y++)for(int x=260;x<764;x++)if(x<266||x>757||y<186||y>453)host.pixels[0][(std::size_t(y)*1024+x)*4+3]=1;
 auto p=energy::calibrated_defaults(1024,640);p.input=3;const auto initial=p;energy::Inputs input;
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;auto source=energy::from_host(std::move(pixels));
 const std::string control="近光半径（像素） / Halo Radius (px)";
 psui::PanelTestHook hook;hook.software=true;hook.host_locale=chinese?"zh_CN":"en_US";hook.initial_scale=chinese?2:1;hook.force_legacy_dpi=chinese;
 int start=-1,drafts=0;float first_draft=0,first_full=0;bool complete=false;
 hook.before_frame=[&](psui::LibraryPanel& ui){
  std::this_thread::sleep_for(std::chrono::milliseconds(10));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);
  if(ui.frame()==3){libraryqa::point(ui,"核心 / Core");io.AddMouseButtonEvent(0,true);}if(ui.frame()==4)io.AddMouseButtonEvent(0,false);
  if(start<0)return;const int n=ui.frame()-start;
  if(n>=3&&n<=45){libraryqa::point(ui,control+".slider",.12f+(n-3)*.009f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);
  if(n==95){libraryqa::point(ui,control+".reset");io.AddMouseButtonEvent(0,true);}if(n==96)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){
  const auto state=ui.items.at("energy.state"),light=ui.items.at("energy.glow"),adaptive=ui.items.at("adaptive.state");
  libraryqa::require(state.y==0&&light.x==1,"layered preview succeeds with independent halo");
  for(auto key:{"ok","cancel","reset"}){auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=ImGui::GetIO().DisplaySize.y+.5f,"action buttons visible");}
  if(chinese)for(auto glyph:{0x5206,0x5C42,0x8FD1,0x5149,0x534A,0x5F84})libraryqa::require(ImGui::GetFont()->FindGlyphNoFallback(ImWchar(glyph)),"new layered-light glyphs available");
  if(start<0&&ui.frame()>12&&adaptive.y>=1&&adaptive.w==0){auto r=ui.items.at(control+".slider");libraryqa::require(r.y>0&&r.w<ImGui::GetIO().DisplaySize.y,"new halo control visible");start=ui.frame();first_draft=adaptive.x;first_full=adaptive.y;}
  if(start>=0){const int n=ui.frame()-start;if(n==45){drafts=int(adaptive.x-first_draft);libraryqa::require(drafts>=2,"layered light publishes frames during drag");}if(n>150&&adaptive.y>first_full+1&&adaptive.z==0&&adaptive.w==0){complete=true;if(argc>2)ui.capture(std::filesystem::path(argv[2]).c_str());ui.close(true);}}
  if(ui.frame()>1300)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto result=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,input);psui::set_panel_test_hook(nullptr);
 if(result!=IDOK||!complete||p!=initial||!hook.error.empty()){std::cerr<<"FAIL layered panel "<<hook.error<<" reset="<<(p==initial)<<'\n';return 3;}
 std::cout<<"PASS layered panel "<<(chinese?"zh200 legacy DPI":"en100")<<": "<<drafts<<" drag drafts, full settle and single-parameter reset\n";
}
