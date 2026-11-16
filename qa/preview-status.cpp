#include "../displacement/tests/mock_host.hpp"
#include "../native/tests/library_qa.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2)return 2;const auto folder=std::filesystem::path(argv[1]);std::filesystem::create_directories(folder);
 mock::Host host(32,32);psui::PanelTestHook hook;hook.software=true;hook.host_locale="en_US";hook.initial_scale=1;
 double maximum=0,total=0;int calls=0;float busy_tail=0;std::array<ImVec2,4> origins{};bool done=false;
 hook.tick=[&](psui::LibraryPanel& ui){
  if(ui.frame()==18){libraryqa::require(ui.items.at("preview.status.mode").x==1,"SM4 Difference is active");libraryqa::require(ui.capture((folder/"busy.bmp").c_str()),"busy capture");busy_tail=ui.items.at("own.tail").y;}
  if(ui.frame()==32){libraryqa::require(!ui.items.contains("preview.status"),"idle overlay disappears");libraryqa::require(ui.items.at("own.tail").y==busy_tail,"status does not shift footer text");libraryqa::require(ui.capture((folder/"idle.bmp").c_str()),"idle capture");
   auto* font=ImGui::GetFont();const auto* glyph=font->FindGlyphNoFallback('R');libraryqa::require(glyph!=nullptr,"glyph exists");unsigned char* alpha;int aw,ah;font->ContainerAtlas->GetTexDataAsAlpha8(&alpha,&aw,&ah);
   int gx=int(std::lround(glyph->U0*aw)),gy=int(std::lround(glyph->V0*ah)),gw=int(std::lround((glyph->U1-glyph->U0)*aw)),gh=int(std::lround((glyph->V1-glyph->V0)*ah));
   int best=0,bx=0,by=0;for(int y=0;y<gh;y++)for(int x=0;x<gw;x++){const int value=alpha[(gy+y)*aw+gx+x];if(value>best){best=value;bx=x;by=y;}}
   libraryqa::require(best>128,"glyph has measurable coverage for blend oracle");std::ofstream out(folder/"samples.json");out<<"[";for(int i=0;i<4;i++){if(i)out<<',';out<<"{\"x\":"<<int(origins[i].x+glyph->X0+bx)<<",\"y\":"<<int(origins[i].y+glyph->Y0+by)<<",\"coverage\":"<<best<<"}";}out<<"]\n";done=true;ui.close(true);
  }
 };
 psui::set_panel_test_hook(&hook);
 const auto result=psui::run_library_panel(GetModuleHandleW(nullptr),nullptr,&host.f,L"Owned preview overlay check",640,400,[&](psui::LibraryPanel& ui){
  const bool busy=ui.frame()<24;
  constexpr ImU32 colors[]={IM_COL32(0,0,0,255),IM_COL32(255,255,255,255),IM_COL32(128,128,128,255),IM_COL32(200,80,10,255)};
  for(int i=0;i<4;i++){
   const auto p=ImGui::GetCursorScreenPos();ImGui::Dummy({300,55});ui.mark("image");ImGui::GetWindowDrawList()->AddRectFilled(p,{p.x+300,p.y+55},colors[i]);
   const auto before=ImGui::GetCursorScreenPos();const auto start=std::chrono::steady_clock::now();ui.preview_status(busy);const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
   if(ui.frame()>3&&busy){maximum=std::max(maximum,ms);total+=ms;calls++;}
   const auto after=ImGui::GetCursorScreenPos();libraryqa::require(before.x==after.x&&before.y==after.y,"overlay is layout-free");if(busy)origins[i]={ui.items.at("preview.status").x,ui.items.at("preview.status").y};
  }
  ImGui::TextUnformatted("Stable plugin description");ui.mark("own.tail");
 });
 psui::set_panel_test_hook(nullptr);if(result!=IDOK||!done||!hook.error.empty()){std::cerr<<"FAIL preview overlay "<<hook.error<<'\n';return 3;}
 std::cout<<"PASS preview overlay: no layout shift; idle hidden; WARP SM4 Difference; cached CPU mean_ms="<<total/std::max(1,calls)<<" max_ms="<<maximum<<'\n';
}
