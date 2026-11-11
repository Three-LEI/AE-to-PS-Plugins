#include "../../displacement/tests/mock_host.hpp"
#include "tint_dialog.hpp"
#include "fractal_core.hpp"
#include "glow_host.hpp"
#include "psui/library_panel.hpp"
#include "library_qa.hpp"
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
namespace fractal {INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,Parameters&);}
namespace {
mock::Host* active=nullptr;std::vector<std::uint8_t> input;
OSErr advance(){
 auto& f=active->f;const auto r=f.bigDocumentData->inRect32;
 const int w=r.right-r.left,h=r.bottom-r.top,step=f.inputRate/65536;
 input.resize(std::size_t(w)*h*4);f.inRowBytes=w*4;f.inColumnBytes=4;f.inPlaneBytes=1;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){const int sx=std::clamp((r.left+x)*step,0,active->width-1),sy=std::clamp((r.top+y)*step,0,active->height-1);const auto i=(std::size_t(sy)*active->width+sx)*4;
  for(int k=0;k<4;++k)input[(std::size_t(y)*w+x)*4+k]=std::uint8_t(std::lround(std::clamp(active->pixels[0][i+k],0.f,1.f)*255));
 }f.inData=input.data();return noErr;
}
}
int main(int argc,char** argv){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 mock::Host host(256,192);active=&host;host.f.inLayerPlanes=3;host.f.inTransparencyMask=1;host.f.advanceState=advance;host.f.samplingSupport=1;
 host.layers[0].sheetID=2;host.layers[1].sheetID=1;host.layers[1].isVisible=true;host.channels[0][0].target=true;
 host.f.filterCase=filterCaseProtectedTransparencyNoSelection;
 for(int y=0;y<192;++y)for(int x=0;x<256;++x){const auto i=(std::size_t(y)*256+x)*4;const bool solid=x>=96&&x<160&&y>=64&&y<128;
  host.pixels[0][i]=solid?1:0;host.pixels[0][i+1]=host.pixels[0][i+2]=0;host.pixels[0][i+3]=solid?1:0;
  host.pixels[1][i]=host.pixels[1][i+1]=0;host.pixels[1][i+2]=host.pixels[1][i+3]=1;
 }
 const auto original_pixels=host.pixels[0];
 const std::string name=BACKGROUND_EFFECT==0?"tint":BACKGROUND_EFFECT==1?"fractal":BACKGROUND_EFFECT==2?"displacement":BACKGROUND_EFFECT==3?"glow":"deep-glow";
 wchar_t file[32768];GetModuleFileNameW(nullptr,file,32768);const auto folder=std::filesystem::path(file).parent_path();
 psui::PanelTestHook hook;hook.software=true;hook.host_locale=argc>1&&std::string(argv[1]).find("zh")!=std::string::npos?"zh_CN":"en_US";
 hook.initial_scale=argc>1&&std::string(argv[1]).find("200")!=std::string::npos?2:1.25f;
 hook.before_frame=[](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(8));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);
  const int n=ui.frame();if(n==15||n==38){libraryqa::point(ui,"显示背景 / Show Background");io.AddMouseButtonEvent(0,true);}if(n==16||n==39)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[&](psui::LibraryPanel& ui){
  const auto size=ImGui::GetIO().DisplaySize;
  const auto item=ui.items.at("显示背景 / Show Background");libraryqa::require(item.y>=0&&item.w<size.y,"background checkbox visible");
  if(ui.frame()==10||ui.frame()==32||ui.frame()==50){
   const auto prefix="background-"+name+"-"+std::to_string(ui.frame());
   libraryqa::require(ui.capture((folder/(prefix+".bmp")).c_str()),"complete GPU background frame");
   const auto r=ui.items.at("image");std::ofstream meta(folder/(prefix+".json"));meta<<"{\"image_rect\":["<<r.x<<','<<r.y<<','<<r.z<<','<<r.w<<"]}";
  }
  if(ui.frame()==55)ui.close(true);if(ui.frame()>65)ui.close(false);
 };psui::set_panel_test_hook(&hook);
 HWND owner=CreateWindowExW(0,L"STATIC",L"Background production dialog QA",WS_OVERLAPPEDWINDOW,100,100,1400,900,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);ShowWindow(owner,SW_SHOW);
 INT_PTR answer=IDCANCEL;
#if BACKGROUND_EFFECT==0
 tint::Parameters p;const auto original=p;answer=tint::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,p);libraryqa::require(std::memcmp(&p,&original,sizeof(p))==0,"Tint parameters unchanged");
#elif BACKGROUND_EFFECT==1
 fractal::Parameters p;const auto original=p;answer=fractal::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,p);libraryqa::require(p.seed==original.seed&&p.contrast==original.contrast&&p.complexity==original.complexity,"Fractal parameters unchanged");
#else
 auto layers=psui::host_layers(&host.f);psui::LayerPixels source_pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,source_pixels))return 2;
#if BACKGROUND_EFFECT==2
 auto source=displacement::from_host(std::move(source_pixels));displacement::Parameters p;p.layer_id=1;const auto original=p;displacement::Image map;answer=displacement::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,layers,source,p,map);libraryqa::require(p.amount_x==original.amount_x&&p.amount_y==original.amount_y,"displacement parameters unchanged");
#else
 auto source=glow::from_host(std::move(source_pixels));auto p=glow::defaults(glow::effect_kind);const auto original=p;glow::Image mask,texture;answer=glow::show_dialog(GetModuleHandleW(nullptr),owner,&host.f,source,layers,p,mask,texture);libraryqa::require(p==original,"glow parameters unchanged");
#endif
#endif
 psui::set_panel_test_hook(nullptr);DestroyWindow(owner);
 libraryqa::require(host.pixels[0]==original_pixels,"view toggle never writes source layer");
 std::cout<<name<<" dialog="<<answer<<" error="<<hook.error<<'\n';return answer==IDOK&&hook.error.empty()?0:1;
}
