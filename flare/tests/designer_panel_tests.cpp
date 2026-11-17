#include "../../displacement/tests/mock_host.hpp"
#include "flare_host.hpp"
#include "../../native/tests/library_qa.hpp"
#include "../src/preset_files.hpp"
#include <chrono>
#include <thread>
int main(int argc,char** argv){
 const bool zh=argc>1&&std::string(argv[1])=="zh";mock::Host host(1024,640);auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;auto source=flare::from_host(std::move(pixels));auto p=flare::defaults(1024,640);auto design=flare::design_from_legacy(p);flare::Image mask;
 if(argc>3)flare::files::test_directory=std::filesystem::path(argv[3]);
 const auto original=p;auto expected=flare::design_preset(10,1024,640).design;psui::PanelTestHook hook;hook.software=true;hook.host_locale=zh?"zh_CN":"en_US";hook.initial_scale=zh?2:1;hook.force_legacy_dpi=zh;bool done=false;float first_drafts=0;int preset_start=-1;
 auto click=[](psui::LibraryPanel& ui,const char* key,bool down){libraryqa::point(ui,key);ImGui::GetIO().AddMouseButtonEvent(0,down);};
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);const int n=ui.frame();
  for(const auto& action:std::vector<std::pair<int,const char*>>{{8,"designer.open"},{14,"designer.add"},{18,"designer.add-type/8"},{32,"designer.up"},{38,"designer.delete/13"},{44,"designer.undo"},{50,"designer.redo"},{96,"元素亮度 / Element Brightness.reset"},{102,"designer.solo"},{108,"designer.solo"},{114,"designer.presets-tab"}})if(n==action.first||n==action.first+1)click(ui,action.second,n==action.first);
  if(n==26||n==27){io.AddKeyEvent(ImGuiMod_Ctrl,n==26);io.AddKeyEvent(ImGuiKey_D,n==26);}
  if(n==52||n==53){libraryqa::point(ui,"designer.element/13");if(n==52)io.AddMouseButtonEvent(0,true);}if(n==54||n==55)libraryqa::point(ui,"designer.element/12");if(n==56)io.AddMouseButtonEvent(0,false);
  if(n>=60&&n<=90){libraryqa::point(ui,"元素亮度 / Element Brightness.slider",.2f+(n-60)*.012f);if(n==60)io.AddMouseButtonEvent(0,true);}if(n==91)io.AddMouseButtonEvent(0,false);
  if(preset_start>=0){const int t=n-preset_start;if(t==1){libraryqa::point(ui,"designer.preset/0");io.AddMouseWheelEvent(0,-14);}for(const auto& action:std::vector<std::pair<int,const char*>>{{8,"designer.preset/10"},{18,"designer.save"},{26,"designer.save"},{34,"designer.saved/0"},{42,"designer.elements-tab"}})if(t==action.first||t==action.first+1)click(ui,action.second,t==action.first);}
 };
 hook.tick=[&](psui::LibraryPanel& ui){libraryqa::actions_fit(ui);
  for(int row=0;row<13;row++){const auto key=std::to_string(row);if(!ui.items.contains("designer.visible/"+key)||!ui.items.contains("designer.element/"+key)||!ui.items.contains("designer.delete/"+key))continue;const auto check=ui.items.at("designer.visible/"+key),name=ui.items.at("designer.element/"+key),button=ui.items.at("designer.delete/"+key);const auto center=[](ImVec4 rect){return (rect.y+rect.w)*.5f;};libraryqa::require(std::abs(center(check)-center(name))<=1.f&&std::abs(center(button)-center(name))<=1.f,"checkbox, name and delete vertically centered");}
  const int n=ui.frame();const auto a=ui.items.at("adaptive.state");if(n==59)first_drafts=a.x;
  if(n>10&&ui.items.contains("designer.state")){const auto state=ui.items.at("designer.state");if(n==21)libraryqa::require(int(state.x)==14&&ui.items.at("designer.selected").z==8,"visual card adds glint in one click");if(n==29)libraryqa::require(int(state.x)==15,"Ctrl+D duplicates");if(n==35)libraryqa::require(int(state.y)==13,"reorder button");if(n==41)libraryqa::require(int(state.x)==14,"inline delete");if(n==47)libraryqa::require(int(state.x)==15,"undo deletion");if(n==53)libraryqa::require(int(state.x)==14,"redo deletion");if(n==59)libraryqa::require(int(state.y)==12,"drag reorders selected element");if(n==90)libraryqa::require(a.x>=first_drafts+2,"designer continuous preview");if(n==99)libraryqa::require(ui.items.at("designer.selected").x==1,"per-object parameter reset");if(n==105)libraryqa::require(state.w>=0,"preview solo");if(n==111)libraryqa::require(state.w<0,"solo stopped");
   if(n>125&&preset_start<0&&ui.items.contains("designer.thumbnails")){const auto thumbs=ui.items.at("designer.thumbnails");libraryqa::require(thumbs.x==640&&thumbs.y==400&&thumbs.w==0,"native thumbnail resolution and no render error");if(thumbs.z==12){libraryqa::require(ui.items.at("cached-image/flare-preset-0").z==1,"card texture uploaded once");if(argc>2){auto path=std::filesystem::path(argv[2]);path+=L".presets.bmp";libraryqa::require(ui.capture(path.c_str()),"preset library capture");}preset_start=n;}}
   if(preset_start>=0&&n-preset_start>65&&a.y>=1&&a.w==0){libraryqa::require(state.x==5,"new Starfield preset and saved file reload");libraryqa::require(ui.items.at("designer.naming").y==(zh?1:0),"automatic names use UI language");if(argc>2)ui.capture(std::filesystem::path(argv[2]).c_str());done=true;ui.close(true);}
  }if(n>1800)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto answer=flare::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,mask,&design);psui::set_panel_test_hook(nullptr);
 if(answer!=IDOK||!done||!hook.error.empty()||p!=original||design!=expected||flare::files::list().size()<2){std::cerr<<"FAIL designer panel "<<hook.error<<'\n';return 3;}
 const auto saved=design;const int count=int(saved.elements.size());bool protected_text=false,deleted=false;
 hook.before_frame=[&](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);const int n=ui.frame();for(const auto& action:std::vector<std::pair<int,const char*>>{{8,"designer.open"},{14,"designer.name"},{24,"designer.element/0"}})if(n==action.first||n==action.first+1)click(ui,action.second,n==action.first);
  if(n==18||n==19||n==28||n==29){const bool down=n==18||n==28;io.AddKeyEvent(ImGuiMod_Ctrl,down);io.AddKeyEvent(ImGuiKey_D,down);}if(n==20||n==21||n==32||n==33)io.AddKeyEvent(ImGuiKey_Delete,n==20||n==32);
 };
 hook.tick=[&](psui::LibraryPanel& ui){if(ui.frame()==23){libraryqa::require(ui.items.at("designer.state").x==count,"text editing blocks element shortcuts");protected_text=true;}if(ui.frame()==31)libraryqa::require(ui.items.at("designer.state").x==count+1,"keyboard duplicate outside input");if(ui.frame()==35){libraryqa::require(ui.items.at("designer.state").x==count,"Delete removes outside input");deleted=true;}if(ui.frame()>38)ui.close(false);};
 psui::set_panel_test_hook(&hook);const auto cancel=flare::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,mask,&design);psui::set_panel_test_hook(nullptr);if(cancel!=IDCANCEL||!hook.error.empty()||!protected_text||!deleted||design!=saved||p!=original){std::cerr<<"FAIL guarded shortcuts "<<hook.error<<'\n';return 4;}
 std::cout<<"PASS designer UI "<<(zh?"zh200 legacy DPI":"en100")<<": visual add, inline delete, Ctrl+D/Delete with text guard, drag sorting, undo/redo, native thumbnails/cached uploads, new preset save/reload, live preview, reset/cancel and layout\n";
}
