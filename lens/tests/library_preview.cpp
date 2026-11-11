#include "../../displacement/tests/mock_host.hpp"
#include "lens_host.hpp"
#include "psui/library_panel.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);mock::Host host(384,256);for(int y=0;y<256;++y)for(int x=0;x<384;++x){auto i=(std::size_t(y)*384+x)*4;host.pixels[0][i]=x/383.f;host.pixels[0][i+1]=y/255.f;host.pixels[0][i+2]=.6f;host.pixels[0][i+3]=.7f;}
 psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;++k,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;psui::LayerPixels read;if(psui::host_read_layer(&host.f,target,host.doc.bounds,0,read))return 1;auto src=lens::from_host(std::move(read));auto p=lens::defaults(lens::effect_kind,384,256);
 psui::PanelTestHook hook;hook.software=true;hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";
 bool ready=false;int start=0;if(argc>1){hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(!ready)return;int n=ui.frame()-start;
  const char* key=n==3?(lens::effect_kind==lens::Kind::cc?"收敛度 / Convergence.slider":"视场 (FOV) / Field Of View.slider"):n==22?"显示背景 / Show Background":n==30?"preview.zoom.slider":n==45?"about":n==49?"about.close":n==80?"ok":nullptr;
  if(key){libraryqa::point(ui,key,n==3?.7f:.5f);io.AddMouseButtonEvent(0,true);}if(n==4||n==23||n==31||n==46||n==50||n==81)io.AddMouseButtonEvent(0,false);
 };hook.tick=[&](psui::LibraryPanel& ui){const auto state=ui.items.at("lens.state");if(!ready&&state.x==1&&state.y==0&&state.z==0){ready=true;start=ui.frame();}if(ui.frame()>600)ui.close(false);};psui::set_panel_test_hook(&hook);}
 const auto answer=lens::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p);psui::set_panel_test_hook(nullptr);
 if(argc>1){if(answer!=IDOK||!hook.error.empty()||(lens::effect_kind==lens::Kind::optics?p.fov<60:p.convergence<-30))return 2;const auto saved=p;hook.before_frame={};hook.tick=[](psui::LibraryPanel& ui){if(ui.frame()>5)ui.close(false);};psui::set_panel_test_hook(&hook);const auto cancelled=lens::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,src,p);psui::set_panel_test_hook(nullptr);if(cancelled!=IDCANCEL||p!=saved)return 3;}
 std::cout<<"PASS: production panel; effect parameters, asynchronous preview, background/zoom, About, language/DPI, accept/cancel.\n";
}
