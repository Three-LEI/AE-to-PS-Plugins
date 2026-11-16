#include "../../displacement/tests/mock_host.hpp"
#include "energy_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 if(argc<3)return 2;const std::string mode=argv[1],profile=argv[2];const bool chinese=profile=="zh";mock::Host host(1024,640);for(auto& v:host.pixels[0])v=0;for(int y=150;y<550;y++)for(int x=200;x<824;x++){auto i=(std::size_t(y)*1024+x)*4;host.pixels[0][i+3]=(x>380&&x<644&&y>260&&y<440)?0:1;}
 auto p=energy::material_preset(mode=="fire"?2:mode=="alpha"?0:1,energy::defaults(1024,640));p.input=mode=="alpha"||mode=="fire"?2:mode=="stroke"?3:0;const auto factory=p;energy::Inputs inputs;auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 3;auto source=energy::from_host(std::move(pixels));
 const std::string control=mode=="alpha"?"Alpha 阈值（%） / Alpha Threshold (%)":mode=="fire"?"火焰高度（像素） / Flame Height (px)":"扰动幅度（像素） / Distortion (px)";
 int start=-1,frames=0;float drafts=0,full=0;bool complete=false;psui::PanelTestHook hook;hook.software=true;hook.host_locale=chinese?"zh_CN":"en_US";hook.initial_scale=chinese?2:1;hook.force_legacy_dpi=chinese;
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(9));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);
  if(ui.frame()==3){libraryqa::point(ui,"核心 / Core");io.AddMouseButtonEvent(0,true);}if(ui.frame()==4)io.AddMouseButtonEvent(0,false);if(ui.frame()==6){libraryqa::point(ui,"发光 / Glow");io.AddMouseButtonEvent(0,true);}if(ui.frame()==7)io.AddMouseButtonEvent(0,false);
  if(start<0)return;int n=ui.frame()-start;if(n>=3&&n<=45){libraryqa::point(ui,control+".slider",.15f+(n-3)*.008f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);if(n==95){libraryqa::point(ui,control+".reset");io.AddMouseButtonEvent(0,true);}if(n==96)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){auto a=ui.items.at("adaptive.state"),state=ui.items.at("energy.state"),material=ui.items.at("energy.material");libraryqa::require(state.y==0,"new alpha/material preview succeeds");libraryqa::require(int(material.x)==factory.style&&int(state.w)==factory.input,"material and input selection stay independent");for(const char* key:{"ok","cancel","reset"}){auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=ImGui::GetIO().DisplaySize.y+.5f,"action buttons fit viewport");}
  if(chinese)for(auto code:{0x8F6E,0x5ED3,0x7535,0x6D41,0x706B,0x7130,0x79CD})libraryqa::require(ImGui::GetFont()->FindGlyphNoFallback(ImWchar(code)),"new material glyphs available");
  if(start<0&&ui.frame()>10&&a.y>=1&&a.w==0){start=ui.frame();drafts=a.x;full=a.y;auto r=ui.items.at(control+".slider");libraryqa::require(r.y>0&&r.w<ImGui::GetIO().DisplaySize.y,"material control visible for drag");}
  if(start>=0){int n=ui.frame()-start;if(n==45){frames=int(a.x-drafts);libraryqa::require(frames>=2,"new materials publish frames throughout drag");}if(n>155&&a.y>full+1&&a.z==0&&a.w==0){complete=true;if(argc>3)ui.capture(std::filesystem::path(argv[3]).c_str());ui.close(true);}}
  if(ui.frame()>1400)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);auto answer=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,inputs);psui::set_panel_test_hook(nullptr);if(answer!=IDOK||!complete||!hook.error.empty()||p!=factory){std::cerr<<"FAIL material panel "<<mode<<' '<<profile<<" answer="<<answer<<" reset="<<(p==factory)<<" error="<<hook.error<<'\n';return 4;}
 std::cout<<"PASS material panel "<<mode<<' '<<profile<<": "<<frames<<" drag drafts; full refinement; single-parameter reset; geometry preserved\n";
}
