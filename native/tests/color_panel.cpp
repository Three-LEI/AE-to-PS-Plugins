// Own production-panel integration test; no user Photoshop document is touched.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "psui/library_panel.hpp"
#include "PIProperties.h"
#include "library_qa.hpp"
#include <array>
#include <iostream>
namespace {
POINT expected{};int canvas_calls=0,picker_calls=0,screen_calls=0;bool invalid=false;
bool use_screen=false;
OSErr property(PIType signature,PIType key,int32,intptr_t* value,Handle*){if(signature!=0x3842494d||key!=propUIBrightness||!value)return -1;*value=3;return noErr;}
OSErr colors(ColorServicesInfo* info){
 if(info->selector==plugIncolorServicesChooseColor){++picker_calls;info->colorComponents[0]=17;info->colorComponents[1]=34;info->colorComponents[2]=51;return noErr;}
 ++canvas_calls;libraryqa::require(false,"shared eyedropper must not invoke legacy interactive SamplePoint");return paramErr;
}
}
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 const bool high=argc>1&&std::string(argv[1])=="--200";
 const std::filesystem::path folder=argc>2?argv[2]:"evidence/color-picker-0.20";
 std::filesystem::create_directories(folder);
 PropertyProcs props{};props.getPropertyProc=property;FilterRecord host{};host.propertyProcs=&props;host.colorServices=colors;
 psui::PreviewView view;psui::ProxyImage proxy;proxy.width=proxy.height=8;proxy.step=1;
 std::vector<std::uint8_t> rgb(8*8*3);for(int i=0;i<64;++i){rgb[i*3]=210;rgb[i*3+1]=80;rgb[i*3+2]=20;}
 double a[3]{.1,.2,.3},b[3]{.6,.7,.8};bool uploaded=false;
 psui::PanelTestHook hook;hook.software=true;hook.initial_scale=high?2:1;hook.host_locale=high?"zh_CN":"en_US";
 hook.screen_sample=[](POINT point,double* result){++screen_calls;if(invalid)return false;libraryqa::require(point.x==expected.x&&point.y==expected.y,"screen provider receives physical point");result[0]=(use_screen?17:51)/255.;result[1]=(use_screen?34:102)/255.;result[2]=(use_screen?51:153)/255.;return true;};
 auto expect_value=[](double c,double expected,const char* message){libraryqa::require(std::abs(c-expected)<.00001,message);};
 hook.before_frame=[&](psui::LibraryPanel& ui){
  Sleep(8);const int n=ui.frame();auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);io.ConfigInputTrickleEventQueue=false;io.MouseDoubleClickTime=.04f;
  auto click=[&](const char* key){libraryqa::point(ui,key);io.AddMouseButtonEvent(0,true);};
  if(n==5||n==30||n==46||n==58||n==66)click("a.pick");
  if(n==12||n==20||n==74)click("b.pick");
  if(n==82||n==90)click("a.pick");
  if(n==40)click("a");
  if(n==6||n==13||n==21||n==31||n==41||n==47||n==59||n==67||n==75||n==91)io.AddMouseButtonEvent(0,false);
  auto native_click=[&](int x,int y){expected={x,y};ClientToScreen(ui.window(),&expected);SendMessageW(ui.window(),WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(SHORT(x),SHORT(y)));SendMessageW(ui.window(),WM_LBUTTONUP,0,MAKELPARAM(SHORT(x),SHORT(y)));};
  if(n==8||n==70){const auto r=ui.items.at("image");native_click(int((r.x+r.z)*.5),int((r.y+r.w)*.5));}
  if(n==15){SendMessageW(ui.window(),WM_RBUTTONDOWN,MK_RBUTTON,0);SendMessageW(ui.window(),WM_RBUTTONUP,0,0);}
  if(n==24||n==50||n==52){invalid=n==50;native_click(-60,int(80*ui.scale()));}
  if(n==34){invalid=true;native_click(int(580*ui.scale()),int(300*ui.scale()));}
  if(n==36){SendMessageW(ui.window(),WM_KEYDOWN,VK_ESCAPE,0);SendMessageW(ui.window(),WM_KEYUP,VK_ESCAPE,0);invalid=false;}
  if(n==60)ReleaseCapture();
  if(n==78){use_screen=true;native_click(-70,int(80*ui.scale()));}
  if(n==83){expected={-90,LONG(120*ui.scale())};ClientToScreen(ui.window(),&expected);SendMessageW(ui.window(),WM_LBUTTONUP,0,MAKELPARAM(SHORT(-90),SHORT(120*ui.scale())));}
 };
 hook.tick=[&](psui::LibraryPanel& ui){const int n=ui.frame();
  if(n==8||n==69)libraryqa::require(ui.picking_color()&&GetCapture()==ui.window(),"picker owns mouse capture");
  if(n==10){expect_value(a[0],210/255.,"preview RGB reaches selected colour");expect_value(a[1],80/255.,"preview RGB green");expect_value(b[0],.6,"other colour unchanged");libraryqa::require(!ui.picking_color(),"preview click completes pick");}
  if(n==17){expect_value(b[0],.6,"right-click cancels without changing colour");libraryqa::require(!ui.picking_color(),"right-click releases mode");}
  if(n==26){expect_value(b[0],.2,"screen canvas RGB reaches second target");expect_value(b[1],.4,"canvas green");libraryqa::require(canvas_calls==0,"no interactive host canvas request");}
  if(n==36)libraryqa::require(ui.picking_color()&&ui.items.at("color.pick.active").y==1,"panel chrome is not a colour sample");
  if(n==38)libraryqa::require(!ui.picking_color()&&GetCapture()!=ui.window(),"Esc cancels pick and keeps panel open");
  if(n==43){expect_value(a[0],17/255.,"host colour picker remains available");libraryqa::require(picker_calls==1,"native picker called once");}
  if(n==52){expect_value(a[0],17/255.,"invalid canvas sample leaves colour unchanged");libraryqa::require(ui.picking_color(),"invalid sample remains armed for retry");}
  if(n==54){expect_value(a[0],.2,"valid retry succeeds");libraryqa::require(canvas_calls==0,"retry never calls interactive host sampling");}
  if(n==63)libraryqa::require(!ui.picking_color(),"capture loss cancels safely");
  if(n==69)libraryqa::require(ui.capture((folder/(high?"picker-200-zh.bmp":"picker-100-en.bmp")).c_str()),"eyedropper instruction GPU capture");
  if(n==73){expect_value(a[0],210/255.,"second preview pick succeeds");libraryqa::require(view.pan_x==0&&view.pan_y==0&&view.fitted,"picking never pans or zooms preview");}
  if(n==80){expect_value(b[0],17/255.,"screen sampling works outside Photoshop");libraryqa::require(screen_calls>=3&&canvas_calls==0,"full-screen sampling always skips native canvas behind it");}
  if(n==85){expect_value(a[0],17/255.,"drag release samples the screen");libraryqa::require(!ui.picking_color()&&GetCapture()!=ui.window(),"drag release completes and releases capture");}
  if(n==88)libraryqa::require(ui.capture((folder/(high?"icons-200-zh.bmp":"icons-100-en.bmp")).c_str()),"DPI vector eyedropper icon screenshot");
  if(n==94)ui.close(true);
 };
 psui::set_panel_test_hook(&hook);const psui::PluginInfo info{"Colour controls 颜色控件","0.20"};
 const auto answer=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host,L"Shared colour picker QA",640,400,[&](psui::LibraryPanel& ui){
  const auto s=ui.scale();ImGui::BeginChild("left",{220*s,0});view.configure({0,0,8,8},200*s,200*s);
  if(!uploaded){libraryqa::require(ui.preview(&host,rgb.data(),8,8),"preview upload");uploaded=true;view.dirty=false;}
  ui.image_view(view,proxy);ui.preview_zoom(view);ImGui::EndChild();ImGui::SameLine();
  ImGui::BeginChild("right",{0,0});ImGui::TextUnformatted(ui.text("颜色 A / Color A").c_str());ui.color_swatch("a",&host,a);
  ImGui::TextUnformatted(ui.text("颜色 B / Color B").c_str());ui.color_swatch("b",&host,b);ImGui::EndChild();
 },"颜色",&info);psui::set_panel_test_hook(nullptr);
 if(answer!=IDOK||!hook.error.empty()){std::cerr<<"FAIL: panel="<<answer<<" error="<<hook.error<<'\n';return 1;}
 libraryqa::require(GetCapture()==nullptr,"closing panel never leaves mouse captured");
 std::cout<<"PASS: production colour controls "<<(high?200:100)<<"% "<<hook.host_locale<<"; preview/canvas/full-screen; drag release; vector icon; native picker; target isolation; capture/invalid retry; Esc/right-click; no view motion.\n";
}
