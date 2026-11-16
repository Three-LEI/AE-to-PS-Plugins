#include "../../displacement/tests/mock_host.hpp"
#include "energy_host.hpp"
#include "PIProperties.h"
#include "../../native/tests/library_qa.hpp"
#include <thread>
#include <chrono>
#include <iostream>
namespace {
bool available=true;
std::vector<std::uint8_t> path_resource(){
 std::vector<std::uint8_t> b(26*4);b[1]=6;b[27]=3;b[29]=2;
 for(int i=0;i<2;i++){auto* p=b.data()+(i+2)*26;p[1]=5;for(int j=0;j<3;j++){std::uint32_t x=i?0x00c00000:0x00400000,y=0x00a00000;for(int k=0;k<4;k++){p[2+j*8+k]=std::uint8_t(y>>(24-8*k));p[6+j*8+k]=std::uint8_t(x>>(24-8*k));}}}return b;
}
OSErr property(unsigned32,unsigned32 key,int32,intptr_t* value,Handle* result){
 if(key==propNumberOfPaths){*value=available?1:0;return noErr;}
 if(key==propUnicodePathName){const wchar_t name[]=L"鎏金_钢笔路径";*result=mock::allocate(sizeof(name));std::memcpy(mock::lock(*result,false),name,sizeof(name));return noErr;}
 if(key==propPathContents){auto b=path_resource();*result=mock::allocate(int32(b.size()));std::memcpy(mock::lock(*result,false),b.data(),b.size());return noErr;}return paramErr;
}
}
int main(){try{
 mock::Host host(1024,640);for(auto& v:host.pixels[0])v=0;PropertyProcs properties{};properties.getPropertyProc=property;host.f.propertyProcs=&properties;
 auto layers=psui::host_layers(&host.f);psui::LayerPixels pixels;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,pixels))return 2;auto source=energy::from_host(std::move(pixels));auto p=energy::defaults(1024,640);p.input=1;energy::Inputs input{energy::line_path(p),"鎏金_钢笔路径"};auto original=input.path;
 for(int mode=0;mode<4;mode++){
  available=mode!=2;int start=-1;bool done=false,reset_seen=false;float full=0;psui::PanelTestHook hook;hook.software=true;hook.host_locale="zh_CN";
  hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(7));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(start<0||mode==2)return;int n=ui.frame()-start;if(n==3){libraryqa::point(ui,mode==3?"文档路径 / Document Path.reset":"path.update");io.AddMouseButtonEvent(0,true);}if(n==4)io.AddMouseButtonEvent(0,false);if(mode==3&&n==10){libraryqa::point(ui,"path.update");io.AddMouseButtonEvent(0,true);}if(mode==3&&n==11)io.AddMouseButtonEvent(0,false);};
  hook.tick=[&](psui::LibraryPanel& ui){auto a=ui.items.at("adaptive.state"),state=ui.items.at("energy.state");libraryqa::require(state.y==0,"path preview has no render error");if(mode==3&&start>=0&&ui.frame()-start==8){libraryqa::require(state.z==0,"path parameter reset clears geometry snapshot");reset_seen=true;}libraryqa::require(ImGui::GetFont()->FindGlyphNoFallback(ImWchar(0x938F)),"rare Unicode path name glyph retained");
   if(start<0&&a.y>=1&&a.w==0){start=ui.frame();full=a.y;}if(start>=0&&ui.frame()-start>50&&a.w==0&&(mode==2||a.y>full)){done=true;ui.close(mode!=0);}if(ui.frame()>800)ui.close(false);
  };
  psui::set_panel_test_hook(&hook);auto answer=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,input);psui::set_panel_test_hook(nullptr);libraryqa::require(done&&hook.error.empty(),"path panel completes without errors");if(mode==0)libraryqa::require(answer==IDCANCEL&&input.path==original,"cancel after Update Path preserves original snapshot");else if(mode==1)libraryqa::require(answer==IDOK&&input.path!=original&&input.path.curves[0].knots[0].anchor==energy::Point{256,400},"Update Path imports new geometry on main thread");else libraryqa::require(answer==IDOK&&input.path.curves[0].knots[0].anchor==energy::Point{256,400}&&(mode!=3||reset_seen),"missing external path or reset/reimport preserves geometry");
 }
 // Endpoint right-drag is independent of left-button preview panning.
 p=energy::defaults(1024,640);input={};int start=-1;float full=0;bool done=false;psui::PanelTestHook hook;hook.software=true;
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(7));auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);if(start<0)return;int n=ui.frame()-start;const auto a=ui.items.at("beam.start");if(n==3){io.AddMousePosEvent(a.x,a.y);io.AddMouseButtonEvent(1,true);}if(n>=4&&n<=15){io.AddMousePosEvent(a.x+2,a.y-1);}if(n==16)io.AddMouseButtonEvent(1,false);};
 hook.tick=[&](psui::LibraryPanel& ui){const auto a=ui.items.at("adaptive.state");if(start<0&&a.y>=1&&a.w==0){start=ui.frame();full=a.y;}if(start>=0&&ui.frame()-start>60&&a.y>full&&a.w==0){done=true;ui.close(true);}if(ui.frame()>800)ui.close(false);};
 psui::set_panel_test_hook(&hook);auto answer=energy::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,input);psui::set_panel_test_hook(nullptr);libraryqa::require(answer==IDOK&&done&&p.start_x>204.8&&p.start_y<320&&p.end_x==819.2&&p.end_y==320,"right drag changes nearest endpoint only");
 std::cout<<"PASS path UI: explicit update/cancel, snapshot reset/reimport, deleted external path, rare Unicode name, and right-drag endpoint with automatic full refinement\n";
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
