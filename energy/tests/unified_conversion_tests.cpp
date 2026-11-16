#include "../../displacement/tests/mock_host.hpp"
#include "energy_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char** argv){
 const bool chinese=argc>1&&std::string(argv[1])=="zh";mock::Host host(1024,640);for(auto& value:host.pixels[0])value=0;
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;auto source=energy::from_host(std::move(pixels));
 auto original=energy::material_preset(4,energy::calibrated_defaults(1024,640));original.input=1;const energy::Inputs saved{energy::line_path(original),"鎏金_兼容路径"};
 auto values=original;auto inputs=saved;
 for(bool accept:{false,true}){
  psui::PanelTestHook hook;hook.software=true;hook.host_locale=chinese?"zh_CN":"en_US";hook.initial_scale=chinese?2:1;hook.force_legacy_dpi=chinese;bool converted=false;
  hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(10));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"common.convert");io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};
  hook.tick=[&](psui::LibraryPanel& ui){auto state=ui.items.at("energy.unified"),material=ui.items.at("energy.material"),adaptive=ui.items.at("adaptive.state");
   if(ui.frame()==3)libraryqa::require(state.x==0&&ui.items.contains("common.convert"),"legacy dialog offers explicit conversion");
   if(ui.frame()>8){libraryqa::require(state.x==1&&material.x==0&&!ui.items.contains("样式 / Style")&&!ui.items.contains("common.convert"),"converted dialog uses common controls with no material selector");converted=true;}
   if(ui.frame()>95&&adaptive.y>=1&&adaptive.w==0)ui.close(accept);if(ui.frame()>1100)ui.close(false);
  };
  psui::set_panel_test_hook(&hook);auto result=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,values,inputs);psui::set_panel_test_hook(nullptr);
  if(!converted||!hook.error.empty()||result!=(accept?IDOK:IDCANCEL)||values!=(accept?energy::to_unified(original):original)||inputs.path!=saved.path||inputs.name!=saved.name){std::cerr<<"FAIL conversion "<<accept<<' '<<hook.error<<'\n';return 3;}
 }
 values=energy::unified_preset(3,energy::unified_defaults(1024,640));values.input=3;inputs=saved;
 psui::PanelTestHook reset;reset.software=true;reset.host_locale=chinese?"zh_CN":"en_US";reset.initial_scale=chinese?2:1;reset.force_legacy_dpi=chinese;
 reset.before_frame=[](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(10));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(ui.frame()==5){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(ui.frame()==6)io.AddMouseButtonEvent(0,false);};
 reset.tick=[](psui::LibraryPanel& ui){auto a=ui.items.at("adaptive.state");if(ui.frame()>95&&a.y>=1&&a.w==0)ui.close(true);if(ui.frame()>1100)ui.close(false);};
 psui::set_panel_test_hook(&reset);const auto result=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,values,inputs);psui::set_panel_test_hook(nullptr);
 if(result!=IDOK||!reset.error.empty()||values!=energy::unified_defaults(1024,640)||!inputs.path.curves.empty()||!inputs.name.empty()){std::cerr<<"FAIL common reset "<<reset.error<<'\n';return 4;}
 std::cout<<"PASS common conversion "<<(chinese?"zh200 legacy DPI":"en100")<<": legacy cancel, explicit accept, common controls and reset-all\n";
}
