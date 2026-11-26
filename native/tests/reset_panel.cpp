// Real shared panel integration: field isolation, typed adapters and DPI/theme.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/generator_controls.hpp"
#include "PIProperties.h"
#include "library_qa.hpp"
#include <array>
#include <cstring>
#include <iostream>
namespace {
int theme=3;
OSErr property(PIType signature,PIType key,int32,intptr_t* value,Handle*) {
 if(signature!=0x3842494d||key!=propUIBrightness||!value)return -1;
 *value=theme;return noErr;
}
enum class Mode {first,second,third};
struct Parameters {
 double amount=25, x=320,y=240;
 int count=7;Mode mode=Mode::second;bool invert=false, enabled=true;
 std::array<double,3> color{1,.5,0};std::uint64_t layer=0;
 double percent=.5,disabled=10;
};
bool equal(const Parameters& a,const Parameters& b) {
 return a.amount==b.amount&&a.x==b.x&&a.y==b.y&&a.count==b.count&&a.mode==b.mode&&a.invert==b.invert&&a.enabled==b.enabled&&a.color==b.color&&a.layer==b.layer&&a.percent==b.percent&&a.disabled==b.disabled;
}
}
int main(int argc,char** argv) {
 try {
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  const bool large=argc>1&&std::string(argv[1])=="--200";
  const std::filesystem::path out=argc>2?argv[2]:"evidence/parameter-reset-0.27/ui";
  std::filesystem::create_directories(out);
  PropertyProcs props{};props.getPropertyProc=property;FilterRecord host{};host.propertyProcs=&props;
  const Parameters defaults{};
  Parameters p=defaults;p.amount=90;p.x=-12;p.y=99;p.count=22;p.mode=Mode::third;p.invert=true;p.enabled=false;p.color={.1,.2,.3};p.layer=1234567890123ULL;p.percent=.83;p.disabled=80;
  Parameters expected=p;bool preview=true;int changed_events=0,checks=0;
  const char* labels[]={"数量 / Amount","位置 / Position","整数 / Count","模式 / Mode","反转 / Invert","颜色 / Color","图层 / Reference layer with a long English label","百分比 / Percent","禁用 / Disabled"};
  constexpr auto section = "参数 / Parameters";
  constexpr auto enabled = "启用 / Enabled";
  constexpr std::array reset_fields{0, 1, 2, 3, 5, 6, 7, 8};
  psui::PanelTestHook hook;hook.software=true;hook.initial_scale=large?2:1;hook.host_locale=large?"en_US":"zh_CN";hook.force_legacy_dpi=large;
  hook.before_frame=[&](psui::LibraryPanel& ui){
   Sleep(8);auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.MouseDoubleClickTime=.01f;io.ClearEventsQueue();io.AddFocusEvent(true);
   const int n=ui.frame();
   for(int i : reset_fields) {
    const int t=8+i*5;
    if(n==t){libraryqa::point(ui,std::string(labels[i])+".reset");io.AddMouseButtonEvent(0,true);}
    if(n==t+1)io.AddMouseButtonEvent(0,false);
   }
   if(n==57){libraryqa::point(ui,std::string(labels[0])+".reset");io.AddMouseButtonEvent(0,true);}if(n==58)io.AddMouseButtonEvent(0,false);
   if(n==64){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(n==65)io.AddMouseButtonEvent(0,false);
   if(n==70)theme=0;
   if(n==80||n==85||n==90){libraryqa::point(ui,n==90?labels[4]:section);io.AddMouseButtonEvent(0,true);}
   if(n==81||n==86||n==91)io.AddMouseButtonEvent(0,false);
   if(n==95){libraryqa::point(ui,enabled);io.AddMouseButtonEvent(0,true);}
   if(n==96)io.AddMouseButtonEvent(0,false);
  };
  hook.tick=[&](psui::LibraryPanel& ui){
   const int n=ui.frame();
   if(n==3){
    for (const auto *label : {labels[4], enabled, "预览 / Preview"}) {
     libraryqa::require(ui.items.contains(label) && !ui.items.contains(std::string(label)+".reset"),
                        "checkbox is present without a separate reset");
    }
    double outside=0,value=0;
    libraryqa::require(!ui.parameter_default(&outside,&value,sizeof(value)),"unbound field rejected");
   }
   for(int i : reset_fields)if(n==10+i*5) {
    switch(i){case 0:expected.amount=defaults.amount;break;case 1:expected.x=defaults.x;expected.y=defaults.y;break;case 2:expected.count=defaults.count;break;case 3:expected.mode=defaults.mode;break;case 5:expected.color=defaults.color;break;case 6:expected.layer=defaults.layer;break;case 7:expected.percent=defaults.percent;break;default:break;}
    libraryqa::require(equal(p,expected),"single reset changes only its parameter; disabled stays untouched");++checks;
   }
   if(n==60)libraryqa::require(changed_events==7,"default/disabled reset does not invalidate preview");
   if(n==67){libraryqa::require(equal(p,defaults),"global reset restores switches and disabled parameters");++checks;}
   if(n==4||n==75)libraryqa::require(ui.capture((out/(std::string(large?"200-":"100-")+(n==4?"dark.bmp":"light.bmp"))).c_str()),"capture reset layout");
   if(n==6){
    const auto number=ui.items.at(labels[0]), slider=ui.items.at(std::string(labels[0])+".slider");
    const auto choice=ui.items.at(labels[3]), point=ui.items.at(std::string(labels[1])+".x");
    libraryqa::require(std::abs(number.x-choice.x)<=1.f&&std::abs(number.x-point.x)<=1.f,
                       "numeric, choice and point fields share a column");
    libraryqa::require(number.z-number.x<=87*ui.scale()&&number.z<slider.x&&slider.z<=ImGui::GetIO().DisplaySize.x,
                       "numeric entry and slider fit the narrow parameter pane");
   }
   if(n==83)libraryqa::require(!ui.items.contains(labels[0])&&equal(p,defaults),"collapsing the section leaves parameters intact");
   if(n==88)libraryqa::require(ui.items.contains(labels[0])&&equal(p,defaults),"reopening the section restores its controls");
   if(n==94){
    expected=defaults;expected.invert=true;
    libraryqa::require(equal(p,expected),"section checkbox changes only its bound parameter");++checks;
   }
   if(n==98){
    expected.enabled=false;
    libraryqa::require(equal(p,expected),"checkbox changes only its bound parameter");++checks;
    ui.close(true);
   }
  };
  psui::set_panel_test_hook(&hook);
  const auto result=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host,L"Parameter reset regression",760,620,[&](psui::LibraryPanel& ui){
   ui.parameter_defaults(p,defaults);bool changed=false;const char* modes[]={"一 / First","二 / Second","三 / Third"};
   ImGui::Dummy({std::max(1.f,ImGui::GetContentRegionAvail().x-350*ui.scale()),1});
   ImGui::SameLine();ui.begin_filter_parameters();
   if(ui.section(section,true)){
    changed|=ui.number(labels[0],p.amount,0,100,0,100);
    changed|=psui::point_control(ui,labels[1],p.x,p.y);
    ui.inherit_number_default(labels[2],p.count);double count=p.count;if(ui.number(labels[2],count,0,100,0,100,true,true)){p.count=int(count);changed=true;}
    ui.inherit_choice_default(labels[3],p.mode);int mode=int(p.mode);if(ui.choice(labels[3],mode,modes,3)){p.mode=Mode(mode);changed=true;}
    changed|=ui.section_check(labels[4],p.invert);
    changed|=ui.parameter_label(labels[5],p.color);
    ImGui::SameLine();ImGui::Text("%.2f %.2f %.2f",p.color[0],p.color[1],p.color[2]);
    if(ui.begin_parameter_row("long-label")){
     ImGui::TableNextColumn();changed|=ui.parameter_label(labels[6],p.layer);
     if(large&&ui.frame()==6)libraryqa::require(ImGui::GetItemRectSize().y>ImGui::GetTextLineHeight(),"long label wraps in its own column");
     ImGui::TableNextColumn();ImGui::Text("%llu",static_cast<unsigned long long>(p.layer));ImGui::EndTable();
    }
    ui.number_default(labels[7],defaults.percent*100);double percent=p.percent*100;if(ui.number(labels[7],percent,0,100,0,100)){p.percent=percent*.01;changed=true;}
    changed|=ui.number(labels[8],p.disabled,0,100,0,100,false);
   }
   changed|=ui.check(enabled,p.enabled);
   ui.check("预览 / Preview",preview);
   if(ui.button("reset","全部重置 / Reset All")){p=defaults;changed=true;}
   ui.end_filter_parameters();
   if(changed)++changed_events;
  },"数量 位置 整数 模式 反转 颜色 图层 百分比 禁用 一 二 三");
  psui::set_panel_test_hook(nullptr);
  libraryqa::require(result==IDOK&&checks==11&&changed_events==10,"all reset and checkbox scenarios completed");
  std::cout<<"PASS: checkboxes, single/all resets, alignment, disabled/no-op, "<<(large?200:100)<<"% DPI and dark/light themes\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
