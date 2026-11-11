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
 int count=7;Mode mode=Mode::second;bool invert=false;
 std::array<double,3> color{1,.5,0};std::uint64_t layer=0;
 double percent=.5,disabled=10;
};
bool equal(const Parameters& a,const Parameters& b) {
 return a.amount==b.amount&&a.x==b.x&&a.y==b.y&&a.count==b.count&&a.mode==b.mode&&a.invert==b.invert&&a.color==b.color&&a.layer==b.layer&&a.percent==b.percent&&a.disabled==b.disabled;
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
  Parameters p=defaults;p.amount=90;p.x=-12;p.y=99;p.count=22;p.mode=Mode::third;p.invert=true;p.color={.1,.2,.3};p.layer=1234567890123ULL;p.percent=.83;p.disabled=80;
  Parameters expected=p;bool preview=true;int changed_events=0,checks=0;
  const char* labels[]={"数量 / Amount","位置 / Position","整数 / Count","模式 / Mode","反转 / Invert","颜色 / Color","图层 / Layer","百分比 / Percent","禁用 / Disabled"};
  psui::PanelTestHook hook;hook.software=true;hook.initial_scale=large?2:1;hook.host_locale=large?"en_US":"zh_CN";
  hook.before_frame=[&](psui::LibraryPanel& ui){
   Sleep(8);auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.MouseDoubleClickTime=.01f;io.ClearEventsQueue();io.AddFocusEvent(true);
   const int n=ui.frame();
   for(int i=0;i<9;++i) {const int t=8+i*5;if(n==t){libraryqa::point(ui,std::string(labels[i])+".reset");io.AddMouseButtonEvent(0,true);}if(n==t+1)io.AddMouseButtonEvent(0,false);}
   if(n==57){libraryqa::point(ui,std::string(labels[0])+".reset");io.AddMouseButtonEvent(0,true);}if(n==58)io.AddMouseButtonEvent(0,false);
   if(n==64){libraryqa::point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(n==65)io.AddMouseButtonEvent(0,false);
   if(n==70)theme=0;
  };
  hook.tick=[&](psui::LibraryPanel& ui){
   const int n=ui.frame();
   if(n==3){libraryqa::require(ui.items.count("预览 / Preview.reset")==0,"preview has no parameter reset");double outside=0,value=0;libraryqa::require(!ui.parameter_default(&outside,&value,sizeof(value)),"unbound field rejected");}
   for(int i=0;i<9;++i)if(n==10+i*5) {
    switch(i){case 0:expected.amount=defaults.amount;break;case 1:expected.x=defaults.x;expected.y=defaults.y;break;case 2:expected.count=defaults.count;break;case 3:expected.mode=defaults.mode;break;case 4:expected.invert=defaults.invert;break;case 5:expected.color=defaults.color;break;case 6:expected.layer=defaults.layer;break;case 7:expected.percent=defaults.percent;break;default:break;}
    libraryqa::require(equal(p,expected),"single reset changes only its parameter; disabled stays untouched");++checks;
   }
   if(n==60)libraryqa::require(changed_events==8,"default/disabled reset does not invalidate preview");
   if(n==67){libraryqa::require(equal(p,defaults),"global reset includes disabled and dependent parameters");++checks;}
   if(n==4||n==75)libraryqa::require(ui.capture((out/(std::string(large?"200-":"100-")+(n==4?"dark.bmp":"light.bmp"))).c_str()),"capture reset layout");
   if(n==78)ui.close(true);
  };
  psui::set_panel_test_hook(&hook);
  const auto result=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host,L"Parameter reset regression",760,620,[&](psui::LibraryPanel& ui){
   ui.parameter_defaults(p,defaults);bool changed=false;const char* modes[]={"一 / First","二 / Second","三 / Third"};
   changed|=ui.number(labels[0],p.amount,0,100,0,100);
   changed|=psui::point_control(ui,labels[1],p.x,p.y);
   ui.inherit_number_default(labels[2],p.count);double count=p.count;if(ui.number(labels[2],count,0,100,0,100,true,true)){p.count=int(count);changed=true;}
   ui.inherit_choice_default(labels[3],p.mode);int mode=int(p.mode);if(ui.choice(labels[3],mode,modes,3)){p.mode=Mode(mode);changed=true;}
   changed|=ui.check(labels[4],p.invert);
   changed|=ui.parameter_label(labels[5],p.color);
   ImGui::SameLine();ImGui::Text("%.2f %.2f %.2f",p.color[0],p.color[1],p.color[2]);
   changed|=ui.parameter_label(labels[6],p.layer);ImGui::SameLine();ImGui::Text("%llu",static_cast<unsigned long long>(p.layer));
   ui.number_default(labels[7],defaults.percent*100);double percent=p.percent*100;if(ui.number(labels[7],percent,0,100,0,100)){p.percent=percent*.01;changed=true;}
   changed|=ui.number(labels[8],p.disabled,0,100,0,100,false);
   ui.check("预览 / Preview",preview);
   if(ui.button("reset","全部重置 / Reset All")){p=defaults;changed=true;}
   if(changed)++changed_events;
  },"数量 位置 整数 模式 反转 颜色 图层 百分比 禁用 一 二 三");
  psui::set_panel_test_hook(nullptr);
  libraryqa::require(result==IDOK&&checks==10&&changed_events==9,"all reset scenarios completed");
  std::cout<<"PASS: single/all resets, isolation, document coordinates, enum/integer/percent adapters, RGB/layer/bool, disabled/no-op, "<<(large?200:100)<<"% DPI and dark/light themes\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
