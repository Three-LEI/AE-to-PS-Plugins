#pragma once
#include "psui/library_panel.hpp"
#include "psui/ui_metrics.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
namespace libraryqa {
inline bool fractal=false;
inline int* theme=nullptr;
inline std::filesystem::path folder;
inline psui::PanelTestHook hook;
inline std::uint64_t base_atlas=0;
inline std::uint64_t atlas_hash(){unsigned char* pixels;int w,h;ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels,&w,&h);std::uint64_t hash=14695981039346656037ULL;for(int i=0;i<w*h;++i)hash=(hash^pixels[i])*1099511628211ULL;return hash;}
inline void require(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
inline void point(psui::LibraryPanel& ui,const std::string& key,float fraction=.5f){auto it=ui.items.find(key);require(it!=ui.items.end(),key.c_str());const auto r=it->second;ImGui::GetIO().AddMousePosEvent(r.x+(r.z-r.x)*fraction,(r.y+r.w)/2);}
inline void wheel_event(psui::LibraryPanel& ui,const char* key,float delta){
 if(!hook.native_wheel_input){point(ui,key);ImGui::GetIO().AddMouseWheelEvent(0,delta);return;}
 const auto r=ui.items.at(key);POINT p{LONG((r.x+r.z)*.5f),LONG((r.y+r.w)*.5f)};ClientToScreen(ui.window(),&p);
 // Intentionally leave ImGui's previous hover position unchanged. Routing must
 // follow the WM_MOUSEWHEEL coordinates, including fast entry/exit.
 SendMessageW(ui.window(),WM_MOUSEWHEEL,MAKEWPARAM(0,SHORT(delta*WHEEL_DELTA)),MAKELPARAM(SHORT(p.x),SHORT(p.y)));
}
inline void setup(bool is_fractal,int& host_theme,const std::filesystem::path& output,bool software){
 fractal=is_fractal;theme=&host_theme;folder=output;hook.software=software;
 hook.before_frame=[](psui::LibraryPanel& ui){const int n=ui.frame();auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);
  if(fractal&&n==5){point(ui,"杂色类型 / Noise");io.AddMouseButtonEvent(0,true);}if(n==6)io.AddMouseButtonEvent(0,false);
  if(fractal&&n==9){point(ui,"杂色类型 / Noise/item/1");io.AddMouseButtonEvent(0,true);}if(n==10)io.AddMouseButtonEvent(0,false);
  const std::string slider=fractal?"对比度 / Contrast.slider":"着色量 / Amount.slider";
  if(n==15){point(ui,slider,.1f);io.AddMouseButtonEvent(0,true);}if(n>=16&&n<=35)point(ui,slider,.1f+(n-16)*.04f);if(n==36)io.AddMouseButtonEvent(0,false);
  if(n==40||n==45){point(ui,"预览 / Preview");io.AddMouseButtonEvent(0,true);}if(n==41||n==46)io.AddMouseButtonEvent(0,false);
  if(fractal&&n==50){point(ui,"变换 / Transform");io.AddMouseButtonEvent(0,true);}if(n==51)io.AddMouseButtonEvent(0,false);
  if(fractal&&n==60){point(ui,"基本 / Basic");io.AddMouseWheelEvent(0,-8);}
  if(n==45)*theme=0;
  if(n==100){point(ui,"reset");io.AddMouseButtonEvent(0,true);}if(n==101)io.AddMouseButtonEvent(0,false);
  if(n==112){point(ui,"ok");io.AddMouseButtonEvent(0,true);}if(n==113)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[](psui::LibraryPanel& ui){int children=0;EnumChildWindows(ui.window(),[](HWND,LPARAM p)->BOOL{++*reinterpret_cast<int*>(p);return TRUE;},reinterpret_cast<LPARAM>(&children));require(children==0,"library UI must not create native child controls");
  require(ImGui::GetIO().Fonts->Fonts[0]->FindGlyphNoFallback(0x5f62)!=nullptr,"Chinese glyph available");
  const int n=ui.frame();if(n==95)require(ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x>.8f,"light theme applied");if(n==2||n==8||n==35||n==65||n==95){const auto file=folder/(std::string(fractal?"library-fractal-":"library-tint-")+std::to_string(n)+".bmp");require(ui.capture(file.c_str()),"capture complete rendered frame");}
  if(n>125)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);
}
inline void setup_dpi(bool is_fractal,int& host_theme,const std::filesystem::path& output){
 setup(is_fractal,host_theme,output,false);hook.initial_scale=1.f;host_theme=3;base_atlas=0;
 hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);switch(ui.frame()){case 5:ui.test_scale(1.25f);break;case 25:ui.test_scale(1.5f);break;case 45:ui.test_scale(2.f);break;case 65:ui.test_scale(1.f);break;}};
 hook.tick=[](psui::LibraryPanel& ui){
  const int n=ui.frame();
  if(n==2||n==12||n==32||n==52||n==72){
   const float expected=n==12?1.25f:n==32?1.5f:n==52?2.f:1.f;
   require(std::abs(ui.scale()-expected)<.001f,"requested scale applied");
   require(ui.font_pixels()==psui::raster_font_pixels(expected),"font rebuilt at target pixels");
   require(ImGui::GetIO().FontGlobalScale==1.f,"old font texture must not be stretched");
   require(ImGui::GetStyle().FramePadding.x==std::floor(5*expected),"style recomputed from baseline");
   require(AreDpiAwarenessContextsEqual(GetWindowDpiAwarenessContext(ui.window()),DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2),"UI window uses per-monitor V2 awareness");
   if(n==2)base_atlas=atlas_hash();if(n==72)require(atlas_hash()==base_atlas&&ui.font_generation()==5,"100%-200%-100% rebuild is deterministic");
   const auto name=std::string(fractal?"dpi-fractal-":"dpi-tint-")+(n==72?"return-100":std::to_string(static_cast<int>(expected*100)))+".bmp";
   require(ui.capture((folder/name).c_str()),"DPI frame capture");
   std::cout<<"DPI scale="<<expected<<" font_px="<<ui.font_pixels()<<" generation="<<ui.font_generation()<<'\n';
  }
  if(n>78)ui.close(true);
 };
}
inline void setup_tint_layout(int& host_theme,const std::filesystem::path& output){
 setup(false,host_theme,output,false);hook.initial_scale=1.f;
 static RECT original{};static std::map<std::string,ImVec4> initial;
 initial.clear();
 hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.AddFocusEvent(true);
  const int n=ui.frame();
  if(n==5){GetWindowRect(ui.window(),&original);SetWindowPos(ui.window(),nullptr,original.left+35,original.top+20,0,0,SWP_NOSIZE|SWP_NOZORDER);}
  if(n==18){MINMAXINFO mm{};SendMessageW(ui.window(),WM_GETMINMAXINFO,0,reinterpret_cast<LPARAM>(&mm));SetWindowPos(ui.window(),nullptr,0,0,mm.ptMinTrackSize.x,mm.ptMinTrackSize.y,SWP_NOMOVE|SWP_NOZORDER);}
  if(n==32)SetWindowPos(ui.window(),nullptr,original.left,original.top,original.right-original.left,original.bottom-original.top,SWP_NOZORDER);
 };
 hook.tick=[](psui::LibraryPanel& ui){const int n=ui.frame();
  if(n==2||n==10||n==25||n==40){
   RECT outer{};GetWindowRect(ui.window(),&outer);MINMAXINFO mm{};SendMessageW(ui.window(),WM_GETMINMAXINFO,0,reinterpret_cast<LPARAM>(&mm));
   require(outer.right-outer.left>=mm.ptMinTrackSize.x&&outer.bottom-outer.top>=mm.ptMinTrackSize.y,"initial size satisfies native tracking minimum");
   const auto size=ImGui::GetIO().DisplaySize;
   for(const auto* key:{"image","black","white","着色量 / Amount","ok","cancel","reset","preview.zoom.slider"}){
    const auto r=ui.items.at(key);require(r.x>=0&&r.y>=0&&r.z<=size.x+.5f&&r.w<=size.y+.5f,"Tint controls fit client viewport");
    if(n==2)initial[key]=r;
    if(n==10||n==40){const auto a=initial.at(key);require(std::abs(a.x-r.x)<.5f&&std::abs(a.y-r.y)<.5f&&std::abs(a.z-r.z)<.5f&&std::abs(a.w-r.w)<.5f,"move and resize roundtrip preserve control layout");}
   }
   require(ui.items.at("white").w<ui.items.at("ok").y,"footer never overlaps parameters");
   require(ui.capture((folder/("tint-layout-"+std::to_string(n)+".bmp")).c_str()),"layout frame capture");
  }
  if(n>46)ui.close(true);
 };
}
inline void setup_language(bool is_fractal,int& host_theme,const std::filesystem::path& output,const std::string& locale){
 setup(is_fractal,host_theme,output,false);hook.host_locale=locale;hook.initial_scale=1.f;host_theme=3;
 hook.before_frame=[](psui::LibraryPanel& ui){
  auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);
  const int n=ui.frame();
  if(fractal&&(n==5||n==9)){point(ui,n==5?"基本 / Basic":"变换 / Transform");io.AddMouseButtonEvent(0,true);}
  if(n==6||n==10)io.AddMouseButtonEvent(0,false);
  if(n==15||n==28||n==41){point(ui,"language");io.AddMouseButtonEvent(0,true);}
  if(n==16||n==29||n==42)io.AddMouseButtonEvent(0,false);
  if(n==19||n==32||n==45){point(ui,n==19?"language/item/2":n==32?"language/item/1":"language/item/0");io.AddMouseButtonEvent(0,true);}
  if(n==20||n==33||n==46)io.AddMouseButtonEvent(0,false);
  if(fractal&&(n==51||n==54)){point(ui,n==51?"基本 / Basic":"杂色类型 / Noise");io.AddMouseButtonEvent(0,true);}
  if(n==52||n==55)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[](psui::LibraryPanel& ui){
  const int n=ui.frame();const auto automatic=psui::language_for_locale(hook.host_locale);
  if(n==2)require(ui.language_mode()==psui::LanguageMode::follow_host&&ui.language()==automatic,"initial language follows Photoshop locale");
  if(n==12||n==24||n==37||n==50){
   const auto expected=n==24?psui::Language::English:n==37?psui::Language::Chinese:automatic;
   require(ui.language()==expected,"live language switch applied");
   require(ui.text("预览 / Preview")== (expected==psui::Language::Chinese?"预览":"Preview"),"checkbox/button labels localized");
   require(ui.text("基本 / Basic")== (expected==psui::Language::Chinese?"基本":"Basic"),"section labels localized");
   require(ui.font_generation()==1,"switching language does not rebuild fonts");
   if(fractal){require(ui.items.count("旋转(°) / Rotation")>0,"open section stays open");require(ui.items.count("对比度 / Contrast")==0,"closed section stays closed");}
   const auto name=std::string(fractal?"language-fractal-":"language-tint-")+(hook.host_locale.empty()?"missing":hook.host_locale)+"-"+std::to_string(n)+".bmp";
   require(ui.capture((folder/name).c_str()),"language frame capture");
   const auto image=ui.items.at("image");
   std::ofstream meta(folder/(name+".json"));
   meta<<"{\"image_rect\":["<<image.x<<','<<image.y<<','<<image.z<<','<<image.w<<"]}";
  }
  if(fractal&&n==57){require(ui.items.count("杂色类型 / Noise/item/2")>0,"translated effect popup opens after language switches");require(ui.capture((folder/("language-fractal-popup-"+(hook.host_locale.empty()?std::string("missing"):hook.host_locale)+".bmp")).c_str()),"translated popup capture");}
  if(n>60)ui.close(true);
 };
}
inline void setup_about_zoom(bool is_fractal,int& host_theme,const std::filesystem::path& output,bool chinese){
 setup(is_fractal,host_theme,output,false);hook.host_locale=chinese?"zh_CN":"en_US";hook.initial_scale=1.f;host_theme=3;
 hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);const int n=ui.frame();
  if(n==5||n==55){point(ui,"about");io.AddMouseButtonEvent(0,true);}if(n==6||n==56)io.AddMouseButtonEvent(0,false);
  if(n==12)io.AddKeyEvent(ImGuiKey_Escape,true);if(n==13)io.AddKeyEvent(ImGuiKey_Escape,false);
  if(n==20){point(ui,"preview.zoom.slider",.94f);io.AddMouseButtonEvent(0,true);}if(n==21)io.AddMouseButtonEvent(0,false);
  if(n==36){point(ui,"preview.fit");io.AddMouseButtonEvent(0,true);}if(n==37)io.AddMouseButtonEvent(0,false);
  if(n==59){point(ui,"about.close");io.AddMouseButtonEvent(0,true);}if(n==60)io.AddMouseButtonEvent(0,false);
  if(n==70){point(ui,"ok");io.AddMouseButtonEvent(0,true);}if(n==71)io.AddMouseButtonEvent(0,false);
 };
 hook.tick=[](psui::LibraryPanel& ui){const int n=ui.frame();
  if(n==10){require(ui.items.count("about.content")>0&&ui.items.count("about.close")>0,"shared About modal opens");require(ImGui::GetIO().Fonts->Fonts[0]->FindGlyphNoFallback(0x6dda)!=nullptr,"traditional author character available");}
  if(n==16)require(ui.items.count("about.content")==0,"Escape closes About without closing filter");
  if(n==64)require(ui.items.count("about.content")==0,"Close button closes modal");
  if(n==10||n==26||n==45){const auto file=folder/(std::string(fractal?"about-zoom-fractal-":"about-zoom-tint-")+hook.host_locale+"-"+std::to_string(n)+".bmp");require(ui.capture(file.c_str()),"About/zoom capture");}
  if(n>85)ui.close(false);
 };
}
inline void setup_pan(bool is_fractal,int& host_theme,const std::filesystem::path& output){
 setup(is_fractal,host_theme,output,false);hook.initial_scale=1;hook.host_locale="zh_CN";
 static ImVec4 zoomed;zoomed={};
 hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);const int n=ui.frame();
  if(n==5){point(ui,"preview.zoom.slider",.94f);io.AddMouseButtonEvent(0,true);}if(n==6)io.AddMouseButtonEvent(0,false);
  if(n==15){point(ui,"image",.35f);io.AddMouseButtonEvent(0,true);}
  if(n>=16&&n<=24){auto r=ui.items.at("image");io.AddMousePosEvent(r.x+(r.z-r.x)*.35f+(n-15)*7,r.y+(r.w-r.y)*.5f+(n-15)*4);}
  if(n==25)io.AddMouseButtonEvent(0,false);
  if(n==35){point(ui,"preview.fit");io.AddMouseButtonEvent(0,true);}if(n==36)io.AddMouseButtonEvent(0,false);
  if(n==48)ui.close(true);
 };
 hook.tick=[](psui::LibraryPanel& ui){const int n=ui.frame();
  if(n==12)zoomed=ui.items.at("preview.source");
  if(n==30){auto moved=ui.items.at("preview.source");require(moved.x<zoomed.x&&moved.y<zoomed.y,"drag pans actual source crop on both axes");}
  if(n==43){auto full=ui.items.at("preview.source");require(full.x<=0&&full.y<=0&&full.z>=512,"Fit restores centered full image after pan");}
  if(n==12||n==30||n==43)require(ui.capture((folder/("pan-"+std::to_string(n)+".bmp")).c_str()),"pan frame capture");
  if(n>60)ui.close(false);
 };
}
inline void setup_wheel(bool is_fractal,int& host_theme,const std::filesystem::path& output){
 setup(is_fractal,host_theme,output,false);hook.initial_scale=1;hook.host_locale="zh_CN";
 static float initial,zoomed;initial=zoomed=0;
 hook.before_frame=[](psui::LibraryPanel& ui){auto& io=ImGui::GetIO();io.ClearEventsQueue();io.ConfigInputTrickleEventQueue=false;io.AddFocusEvent(true);const int n=ui.frame();
  if(n==5)wheel_event(ui,"image",1);
  if(n==17)wheel_event(ui,"ok",2);
  if(n==28)wheel_event(ui,"image",-1);
  if(n==36){point(ui,"image");io.AddMouseButtonEvent(0,true);}
  if(n==37)wheel_event(ui,"ok",2);
  if(n==38)io.AddMouseButtonEvent(0,false);
  if(n==45)ui.close(true);
 };
 hook.tick=[](psui::LibraryPanel& ui){const int n=ui.frame();const auto transform=ui.items.at("preview.transform");
  if(n==2)initial=transform.x;
  if(n==12){zoomed=transform.x;require(std::abs(zoomed-initial*1.2f)<.01f,"wheel inside preview zooms by one step");auto roi=ui.items.at("preview.source");require(roi.z-roi.x<512,"wheel zoom fetches a smaller real ROI");}
  if(n==24)require(transform.x==zoomed,"wheel outside preview cannot zoom");
  if(n==33||n==41)require(std::abs(transform.x-initial)<.01f,"reverse wheel restores zoom; leaving active preview cannot zoom");
  if(n==12)require(ui.capture((folder/"wheel-zoom.bmp").c_str()),"wheel frame capture");
  if(n>60)ui.close(false);
 };
}
}
