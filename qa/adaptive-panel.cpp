#include "../displacement/tests/mock_host.hpp"
#include "../native/tests/library_qa.hpp"
#include <thread>
#include <chrono>
#if ADAPT_MODULE==0
#include "tint_dialog.hpp"
namespace effect=tint;
#endif
#if ADAPT_MODULE==1
#include "fractal_core.hpp"
namespace effect=fractal;
namespace fractal {INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,Parameters&);}
#endif
#if ADAPT_MODULE==2
#include "displacement_host.hpp"
namespace effect=displacement;
#endif
#if ADAPT_MODULE==3
#include "glow_host.hpp"
namespace effect=glow;
#endif
#if ADAPT_MODULE==4
#include "chromatic_host.hpp"
namespace effect=chromatic;
#endif
#if ADAPT_MODULE==5
#include "vector_host.hpp"
namespace effect=vectorblur;
#endif
#if ADAPT_MODULE==6
#include "pattern_host.hpp"
namespace effect=patterns;
#endif
#if ADAPT_MODULE==7
#include "cell_host.hpp"
namespace effect=cell;
#endif
#if ADAPT_MODULE==8
#include "lens_host.hpp"
namespace effect=lens;
#endif
#if ADAPT_MODULE==9
#include "jpeg_host.hpp"
namespace effect=jpegfx;
#endif
#if ADAPT_MODULE==10
#include "surface_host.hpp"
namespace effect=surface;
#endif
#if ADAPT_MODULE==11
#include "creative_host.hpp"
namespace effect=creative;
#endif
#if ADAPT_MODULE==12
#include "roughen_host.hpp"
namespace effect=roughen;
#endif
int main(int argc,char** argv){
 mock::Host host(1024,640);for(int layer=0;layer<2;layer++)for(int y=0;y<640;y++)for(int x=0;x<1024;x++){
  const auto i=(std::size_t(y)*1024+x)*4;host.pixels[layer][i]=float(.5+.45*std::sin(x*.09));host.pixels[layer][i+1]=y/639.f;host.pixels[layer][i+2]=x/1023.f;host.pixels[layer][i+3]=(x>100&&x<930&&y>70&&y<580)?.8f:0;
 }
 auto layers=psui::host_layers(&host.f);effect::Parameters p;
#if ADAPT_MODULE==0
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,p);};
#endif
#if ADAPT_MODULE==1
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,p);};
#endif
#if ADAPT_MODULE==2
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,layers,src,p,map);};
#endif
#if ADAPT_MODULE==3
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
p=effect::defaults(effect::effect_kind);effect::Image texture;
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map,texture);};
#endif
#if ADAPT_MODULE==4
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);};
#endif
#if ADAPT_MODULE==5
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);};
#endif
#if ADAPT_MODULE==6
p=effect::defaults_for_host(&host.f);
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,p);};
#endif
#if ADAPT_MODULE==7
p=effect::defaults_for_host(&host.f);
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,p);};
#endif
#if ADAPT_MODULE==8
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
p=effect::defaults(effect::effect_kind,1024,640);
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p);};
#endif
#if ADAPT_MODULE==9
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);};
#endif
#if ADAPT_MODULE==10
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
p=effect::defaults(effect::effect_kind,1024,640);
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,layers,p,map);};
#endif
#if ADAPT_MODULE==11
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
p=effect::defaults(effect::effect_kind,1024,640);
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p);};
#endif
#if ADAPT_MODULE==12
psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;k++,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels))return 1;auto src=effect::from_host(std::move(pixels));effect::Image map;
p=effect::defaults();effect::Prepared prepared;
auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p,prepared);};
#endif
psui::PanelTestHook hook;hook.software=true;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1;hook.force_legacy_dpi=argc>1&&std::string(argv[1]).find("legacy")!=std::string::npos;
 bool ready=false,complete=false;int start=0,frames=0;float initial_draft=0,initial_full=0;std::string slider;
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(!ready)return;int n=ui.frame()-start;
  if(n>=3&&n<=45){libraryqa::point(ui,slider,.25f+(n-3)*.007f);if(n==3)io.AddMouseButtonEvent(0,true);}if(n==46)io.AddMouseButtonEvent(0,false);
  if(n==90){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(n==91)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){const auto state=ui.items.at("adaptive.state");const auto size=ImGui::GetIO().DisplaySize;
  libraryqa::actions_fit(ui);
  for(const char* key:{"ok","cancel","reset","交互快速预览 / Adaptive Preview"}){const auto r=ui.items.at(key);libraryqa::require(r.y>=0&&r.w<=size.y+.5f,"adaptive checkbox/footer fits viewport");}
  if(!ready&&state.y>=1&&state.w==0){for(const auto& [key,r]:ui.items)if(key.ends_with(".slider")&&key!="preview.zoom.slider"&&r.y>=0&&r.w<size.y){slider=key;break;}libraryqa::require(!slider.empty(),"visible parameter slider");ready=true;start=ui.frame();initial_draft=state.x;initial_full=state.y;}
  if(ready){int n=ui.frame()-start;if(n==45){frames=int(state.x+state.y-initial_draft-initial_full);libraryqa::require(frames>=2,"preview frames continue during drag");}
   if(n>=130&&state.y>initial_full&&state.z==0&&state.w==0){complete=true;ui.close(true);}}
  if(ui.frame()>850)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto answer=show();psui::set_panel_test_hook(nullptr);
 if(answer!=IDOK||!complete||!hook.error.empty()){std::cerr<<"FAIL adaptive UI module="<<ADAPT_MODULE<<" answer="<<answer<<" error="<<hook.error<<" frames="<<frames<<'\n';return 2;}
 const auto saved=p;hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);if(ui.frame()==4){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(ui.frame()==5)io.AddMouseButtonEvent(0,false);};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>15)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancel=show();psui::set_panel_test_hook(nullptr);
 if(cancel!=IDCANCEL||std::memcmp(&p,&saved,sizeof(p))!=0)return 3;
 std::cout<<"PASS adaptive UI module="<<ADAPT_MODULE<<" profile="<<(argc>1?argv[1]:"en")<<" drag frames="<<frames<<"; idle full refinement; reset/cancel; viewport fit.\n";
}
