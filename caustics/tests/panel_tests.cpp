#include "../../displacement/tests/mock_host.hpp"
#include "caustics_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
int main(int argc,char** argv){
 const bool zh=argc>1&&std::string(argv[1])=="zh",reference=argc>1&&std::string(argv[1])=="ref";mock::Host host(640,480);host.layers[1].unicodeName=reinterpret_cast<const uint16*>(L"鎏金水波");
 if(reference){for(int k=0;k<4;k++){host.channels[1][k].bounds={-20,-160,460,480};host.data[1][k].left=-160;host.data[1][k].top=-20;}
  for(int y=0;y<480;y++)for(int x=0;x<640;x++)for(int k=0;k<3;k++)host.pixels[1][(std::size_t(y)*640+x)*4+k]=float(.5+.4*std::sin(x*.04)*std::cos(y*.06));}
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;
 auto source=caustics::from_host(std::move(pixels));caustics::Parameters p;caustics::Image map;if(reference){p.surface=2;p.map_key=layers[1].name_key;p.shift_x=140;}const auto initial=p;
 psui::PanelTestHook hook;hook.software=true;hook.host_locale=zh?"zh_CN":"en_US";hook.initial_scale=zh?2:1;hook.force_legacy_dpi=zh;
 int start=-1,drafts=0;float first=0,full=0;bool done=false;float cache_hits=0;
 const std::string control=reference?"波高（像素） / Wave Height (px)":"水波尺寸（像素） / Wave Scale (px)";
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(start<0)return;const int n=ui.frame()-start;
  if(n>=3&&n<=45){libraryqa::point(ui,control+".slider",.2f+(n-3)*.01f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);
  if(n==85){libraryqa::point(ui,control+".reset");io.AddMouseButtonEvent(0,true);}if(n==86)io.AddMouseButtonEvent(0,false);
  if(n>=150&&n<=195){libraryqa::point(ui,"光纹强度（%） / Light Intensity (%).slider",.2f+(n-150)*.01f);if(n==150)io.AddMouseButtonEvent(0,true);}if(n==196)io.AddMouseButtonEvent(0,false);
  if(n==235){libraryqa::point(ui,"光纹强度（%） / Light Intensity (%).reset");io.AddMouseButtonEvent(0,true);}if(n==236)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){libraryqa::actions_fit(ui);libraryqa::require(ImGui::GetIO().Fonts->Fonts[0]->FindGlyphNoFallback(0x938F)!=nullptr,"rare layer-name glyph available");const auto a=ui.items.at("adaptive.state"),state=ui.items.at("caustics.state");libraryqa::require(state.y==0,"valid preview");
  if(start<0&&a.y>=1&&a.w==0){start=ui.frame();first=a.x;full=a.y;}
  if(start>=0){const int n=ui.frame()-start;if(n==45){drafts=int(a.x-first);libraryqa::require(drafts>=2&&state.z>2,"continuous drag updates dots");}if(n>290&&a.y>full+1&&a.w==0){cache_hits=ui.items.at("caustics.cache").x;libraryqa::require(cache_hits>=3,"intensity drag reuses water and refracted background");done=true;if(argc>2)ui.capture(std::filesystem::path(argv[2]).c_str());ui.close(true);}}
  if(ui.frame()>900)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto result=caustics::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,map);psui::set_panel_test_hook(nullptr);
 if(result!=IDOK||!done||p!=initial||!hook.error.empty()){std::cerr<<"FAIL caustics UI "<<hook.error<<'\n';return 3;}
 if(reference&&(map.left!=-160||map.top!=-20||map.width!=640||map.height!=480)){std::cerr<<"FAIL preview lost full reference extent\n";return 5;}
 auto saved=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,"预设 / Preset");io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>18)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancel=caustics::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,map);psui::set_panel_test_hook(nullptr);if(cancel!=IDCANCEL||p!=saved)return 4;
 std::cout<<"PASS caustics UI "<<(zh?"zh200 legacy DPI":"en100")<<(reference?" with full off-canvas reference":"")<<": "<<drafts<<" drag frames; "<<cache_hits<<" cache hits; full refinement; native refraction default; single reset/cancel; taller footer/Fit layout\n";
}
