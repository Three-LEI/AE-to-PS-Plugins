// Tests our own offscreen widget, not the user's Photoshop session.
#define WIN32_LEAN_AND_MEAN
#include "psui/widgets.hpp"
#include "psui/scroll_panel.hpp"
#include "psui/paint.hpp"
#include "psui/photoshop_host.hpp"
#include "psui/localization.hpp"
#include "psui/preview_composite.hpp"
#include "psui/preview_view.hpp"
#include "psui/plugin_info.hpp"
#include "PIProperties.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <stdexcept>

namespace {
void check(bool v,const char* text) { if(!v) { std::cerr<<"FAIL: "<<text<<'\n'; std::exit(1); } }
FilterRecord* active=nullptr;
int calls=0;
OSErr failure=noErr;
struct LocaleHandle { std::string bytes; };
std::string locale_value;
int locale_live=0,locale_locks=0;
bool locale_fail=false,locale_lock_fail=false;
OSErr locale_property(PIType signature,PIType key,int32,intptr_t*,Handle* result) {
    check(signature==0x3842494d && key==propLocale && result,"SDK locale property signature");
    *result=reinterpret_cast<Handle>(new LocaleHandle{locale_value});++locale_live;
    return locale_fail ? paramErr : noErr;
}
void locale_dispose(Handle h){delete reinterpret_cast<LocaleHandle*>(h);--locale_live;}
int32 locale_size(Handle h){return static_cast<int32>(reinterpret_cast<LocaleHandle*>(h)->bytes.size());}
Ptr locale_lock(Handle h,Boolean){if(locale_lock_fail)return nullptr;++locale_locks;return reinterpret_cast<LocaleHandle*>(h)->bytes.data();}
void locale_unlock(Handle){--locale_locks;}
void localization_tests() {
    PropertyProcs properties{};properties.getPropertyProc=locale_property;
    HandleProcs handles{};handles.getSizeProc=locale_size;handles.disposeProc=locale_dispose;handles.lockProc=locale_lock;handles.unlockProc=locale_unlock;
    FilterRecord host{};host.propertyProcs=&properties;host.handleProcs=&handles;
    for(const std::string value:{"zh_CN","zh_TW","en_US","en_GB","de_DE",""}){
        locale_value=value;
        check(psui::host_locale(&host)==value,"locale handles are byte-counted, not zero-terminated");
        check(locale_live==0&&locale_locks==0,"locale property handle released");
    }
    locale_value=std::string("zh_CN\0padding",13);
    check(psui::host_locale(&host)=="zh_CN","optional locale terminator accepted");
    locale_value=std::string("z\0h\0_\0C\0N\0\0\0",12);
    check(psui::host_locale(&host)=="zh_CN","actual PS 27.7 UTF-16LE locale handle");
    locale_value=std::string("e\0n\0_\0U\0S\0",10);
    check(psui::host_locale(&host)=="en_US","UTF-16LE without terminator");
    locale_value=std::string("\xff\xfez\0h\0_\0C\0N\0",12);
    check(psui::host_locale(&host)=="zh_CN","UTF-16LE BOM");
    locale_value=std::string("\0z\0h\0_\0C\0N",10);
    check(psui::host_locale(&host)=="zh_CN","UTF-16BE locale");
    locale_value=std::string("z\0h\0_\0C\0N",9);
    check(psui::host_locale(&host).empty(),"truncated UTF-16 rejected");
    locale_value=std::string(129,'x');check(psui::host_locale(&host).empty(),"oversized locale rejected");
    locale_value="zh_CN";locale_fail=true;check(psui::host_locale(&host).empty(),"property failure falls back");locale_fail=false;
    locale_lock_fail=true;check(psui::host_locale(&host).empty(),"failed handle lock falls back");locale_lock_fail=false;
    check(locale_live==0&&locale_locks==0,"failed locale reads release handle without unlocking unowned locks");
    check(psui::host_locale(nullptr).empty(),"missing host supported");
    host.handleProcs=nullptr;check(psui::host_locale(&host).empty(),"missing handle suite does not allocate");
    using psui::Language;using psui::LanguageMode;
    for(const auto value:{"zh_CN","zh-Hans","ZH_tw","zh"})check(psui::language_for_locale(value)==Language::Chinese,"Chinese locale family");
    for(const auto value:{"","en_US","de_DE","zhong"})check(psui::language_for_locale(value)==Language::English,"non-Chinese/missing locale uses English");
    check(psui::resolve_language(LanguageMode::English,"zh_CN")==Language::English,"explicit English overrides Chinese host");
    check(psui::resolve_language(LanguageMode::Chinese,"en_US")==Language::Chinese,"explicit Chinese overrides English host");
    check(psui::localized_text("子位移 X / Sub Offset X",Language::English)=="Sub Offset X","English parameter translation");
    check(psui::localized_text("子位移 X / Sub Offset X",Language::Chinese)=="子位移 X","Chinese parameter translation");
    check(psui::localized_text("English only",Language::Chinese)=="English only","unpaired metadata preserved");
    std::cout<<"PASS: PS locale property ownership/failure; language selection and fallbacks.\n";
}
void generator_preview_tests() {
    using psui::preview_channel;
    check(preview_channel(255,80,0,255,156,true,true)==80,"blank generator preview creates visible output");
    check(preview_channel(255,80,0,255,156,false,true)==156,"disabled preview shows original transparency");
    check(preview_channel(255,80,0,255,156,true,false)==156,"protected transparency remains transparent");
    check(preview_channel(32,80,0,0,200,true,true)==200,"outside selection preserves transparency");
    check(preview_channel(32,80,0,128,200,true,true)==140,"feathered selection composites generated opacity");
    check(preview_channel(200,80,128,0,156,true,true)==178,"outside selection retains semi-transparent source");
    check(preview_channel(200,80,128,255,156,true,true)==80,"editable output fills semi-transparent alpha");
    check(preview_channel(200,80,255,255,156,true,true)==80,"opaque source matches processed color");
    std::cout<<"PASS: generator preview opacity, preview off, protected alpha, selection and feathering.\n";
}
void preview_view_tests() {
    psui::PreviewView view;view.configure({20,40,420,840},200,200);
    check(view.percent==25&&view.region().left==40&&view.region().right==840,"fit uses full source bounds and aspect ratio");
    view.zoom(100);auto r=view.region();check(r.left==340&&r.right==540&&r.top==120&&r.bottom==320,"100 percent requests centered document pixels");
    view.zoom(200);r=view.region();check(r.left==390&&r.right==490&&r.top==170&&r.bottom==270,"zoom increases real source detail around same center");
      check(view.pan(40,-20),"zoomed preview can pan");r=view.region();check(r.left==370&&r.top==180,"screen drag moves sampling ROI opposite image drag at pixel zoom");
      view.pan(1e6,-1e6);r=view.region();check(r.left==40&&r.bottom==420,"pan clamps to document edges");
      view.recenter();check(view.center_x()==440&&view.center_y()==220,"double-click recenter keeps zoom");
    view.configure({20,40,420,840},300,300);check(view.percent==200&&view.region().right-view.region().left==150,"DPI resize keeps manual pixel zoom");
    view.fit();check(view.percent==37.5&&view.region().left==40&&view.region().right==840,"fit restores full field after zoom");
      check(view.pan_x==0&&view.pan_y==0&&!view.pan(40,40),"fit resets pan and fitted image cannot drift into blank space");
      view.zoom(100);const double anchor_x=view.center_x()+40,anchor_y=view.center_y()-20;
      check(view.wheel(1,190,130)&&std::abs(view.percent-120)<1e-9,"one wheel notch zooms by twenty percent");
      check(std::abs(view.center_x()+40/1.2-anchor_x)<1e-9&&std::abs(view.center_y()-20/1.2-anchor_y)<1e-9,"wheel zoom preserves cursor anchor");
      check(view.wheel(-1,190,130)&&std::abs(view.percent-100)<1e-9,"reverse wheel returns to same pixel zoom");
      check(!view.wheel(0,0,0)&&!view.wheel(std::nan(""),0,0),"zero/nonfinite wheel events ignored");
      view.fit();
    view.configure({-500,-900,-100,-100},200,200);view.zoom(100);r=view.region();check(r.left==-600&&r.right==-400&&r.top==-400&&r.bottom==-200,"negative-origin crop remains centered");
    const psui::PluginInfo info{"QA effect","1.2.3"};
    for(auto language:{psui::Language::Chinese,psui::Language::English}){
        const auto text=psui::about_text(info,language);
        check(text.find("1.2.3")!=std::string::npos&&text.find("by：淚泪泪")!=std::string::npos,"shared about metadata and exact author");
        check(text.find(language==psui::Language::Chinese?"开源地址":"Source code")!=std::string::npos&&text.find(psui::source_repository)!=std::string::npos,"about source address localized and points to project");
    }
    std::cout<<"PASS: centered preview crops, fit/manual/DPI semantics, exact shared about content.\n";
}
std::vector<std::uint8_t> sample,mask;
OSErr advance() {
    ++calls;
    auto& f=*active;
    const auto r=f.bigDocumentData->inRect32;
    const auto out=f.bigDocumentData->outRect32;
    check(out.right==out.left && out.bottom==out.top,"proxy never requests output");
    if(failure!=noErr) return failure;
    if(r.right==r.left) return noErr;
    const int w=r.right-r.left,h=r.bottom-r.top,n=f.inHiPlane+1;
    const int bytes=f.depth==16?2:f.depth==32?4:1;
    f.inRowBytes=w*n*bytes+7; f.inColumnBytes=n*bytes; f.inPlaneBytes=bytes; f.maskRowBytes=w+3;
    sample.assign(f.inRowBytes*h,0xA5); mask.assign(f.maskRowBytes*h,128);
    for(int y=0;y<h;++y)for(int x=0;x<w;++x)for(int c=0;c<n;++c){auto* dest=sample.data()+y*f.inRowBytes+(x*n+c)*bytes;const auto value=static_cast<unsigned>((x+y+c*53)%256);if(bytes==1)*dest=static_cast<std::uint8_t>(value);else if(bytes==2){const auto v=static_cast<std::uint16_t>((value*32768+127)/255);std::memcpy(dest,&v,2);}else{const auto v=value/255.f;std::memcpy(dest,&v,4);}}
    f.inData=sample.data(); f.maskData=mask.data(); return noErr;
}
void proxy_tests() {
    FilterRecord f{}; BigDocumentStruct big{};
    big.filterRect32={3,5,4003,6005}; big.inRect32={1,2,3,4}; big.outRect32={5,6,7,8};
    f.bigDocumentData=&big; f.advanceState=advance; f.depth=8; f.samplingSupport=1;
    f.inLayerPlanes=3; f.inTransparencyMask=1; f.haveMask=true; f.inputRate=65536; f.maskRate=65536;
    f.inLoPlane=2; f.inHiPlane=2; f.inRowBytes=123; f.inColumnBytes=4; f.wantLayout=1;
    const auto saved=f; const auto rects=big;
    active=&f; calls=0; failure=noErr;
    psui::ProxyImage image;
    check(psui::host_read_proxy(&f,192,image)==noErr,"proxy read");
    check(calls==2,"input fetch and release");
    check(image.width<=194 && image.height<=194 && image.channels==4,"bounded proxy with alpha");
    check(image.pixels.size()==static_cast<std::size_t>(image.width*image.height*4),"packed proxy size");
    check(image.pixels[0]==0 && image.pixels[3]==159,"RGB/alpha copied without padding");
    check(image.mask.size()==static_cast<std::size_t>(image.width*image.height) && image.mask[0]==128,"selection mask");
    check(f.inputRate==saved.inputRate && f.maskRate==saved.maskRate && f.inLoPlane==2 && f.inHiPlane==2 && f.inRowBytes==123 && f.wantLayout==1,"host layout/rates restored");
    check(std::memcmp(&big.inRect32,&rects.inRect32,sizeof(VRect))==0 && std::memcmp(&big.outRect32,&rects.outRect32,sizeof(VRect))==0,"host rectangles restored");
    for(int depth:{16,32}){f.depth=depth;check(psui::host_read_proxy(&f,192,image)==noErr&&image.pixels[0]==0&&image.pixels[3]==159,"deep proxy normalization and alpha");}f.depth=8;
    calls=0; failure=userCanceledErr;
    check(psui::host_read_proxy(&f,192,image)==userCanceledErr && calls==1 && image.pixels.empty(),"host cancellation propagates without retry");
    check(f.inputRate==65536 && f.inHiPlane==2,"restored after error");
    failure=noErr; f.advanceState=nullptr;
    check(psui::host_read_proxy(&f,192,image)==noErr && image.pixels.empty(),"host without proxy API");
}
void widget_tests() {
    const auto module=GetModuleHandleW(nullptr);
    check(psui::register_widgets(module),"widget class registration");
    psui::Theme theme;
    HWND slider=CreateWindowExW(0,psui::slider_class,L"Amount",WS_POPUP,0,0,240,24,nullptr,nullptr,module,nullptr);
    check(slider!=nullptr,"custom slider creation");
    psui::slider_bind(slider,&theme); psui::slider_range(slider,-100,1000); psui::slider_steps(slider,1,100);
    psui::slider_set(slider,2000); check(psui::slider_get(slider)==1000,"upper clamp");
    psui::slider_set(slider,-500); check(psui::slider_get(slider)==-100,"lower clamp");
    SendMessageW(slider,WM_KEYDOWN,VK_RIGHT,0); check(psui::slider_get(slider)==-99,"keyboard small step");
    SendMessageW(slider,WM_KEYDOWN,VK_PRIOR,0); check(psui::slider_get(slider)==1,"keyboard page step");
    SendMessageW(slider,WM_KEYDOWN,VK_END,0); check(psui::slider_get(slider)==1000,"keyboard end");
    SendMessageW(slider,WM_KEYDOWN,VK_HOME,0); check(psui::slider_get(slider)==-100,"keyboard home");
    psui::slider_range(slider,0,1000);
    HDC dc=CreateCompatibleDC(nullptr);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=240; info.bmiHeader.biHeight=-24; info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    void* pixels=nullptr; auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0); auto original=SelectObject(dc,bitmap);
    check(bitmap && pixels,"test DIB");
    // Warm the retained slider bitmap and lazy Windows GDI objects before
    // measuring growth. Their one-time allocation is not a per-frame leak.
    for(int level=0;level<4;++level){theme.apply(nullptr,level);SendMessageW(slider,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT);}GdiFlush();
    const auto resources=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    for(int level=0;level<4;++level) {
        theme.apply(nullptr,level);
        const auto c=theme.colors();
        std::vector<std::uint32_t> baseline;
        for(int i=0;i<240;++i) {
            psui::slider_set(slider,(i*97)%1001);
            // A white-poisoned frame proves the entire background is repainted.
            std::memset(pixels,0xFF,240*24*4);
            SendMessageW(slider,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT); GdiFlush();
            auto* p=static_cast<std::uint32_t*>(pixels);
            for(int n=0;n<240*24;++n) {
                const auto v=p[n]&0xFFFFFF;
                check(v==c.bg || v==c.track || v==c.button || v==c.text,"full-buffer painting; no white/OS-color ghost");
            }
        }
        psui::slider_set(slider,0); SendMessageW(slider,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc),0); GdiFlush();
        baseline.assign(static_cast<std::uint32_t*>(pixels),static_cast<std::uint32_t*>(pixels)+240*24);
        psui::slider_set(slider,1000); SendMessageW(slider,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc),0);
        psui::slider_set(slider,0); SendMessageW(slider,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(dc),0); GdiFlush();
        check(std::equal(baseline.begin(),baseline.end(),static_cast<std::uint32_t*>(pixels)),"round-trip thumb position has no stale pixels");
    }
    std::cout<<"GDI warmed baseline="<<resources<<" after="<<GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<<"\n";check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=resources+1,"no per-frame GDI handle leak");
    SelectObject(dc,original); DeleteObject(bitmap); DeleteDC(dc); DestroyWindow(slider);
    HWND panel=CreateWindowExW(WS_EX_CONTROLPARENT,psui::scroll_panel_class,L"",WS_POPUP,0,0,240,150,nullptr,nullptr,module,nullptr);
    check(panel!=nullptr,"scroll panel creation");psui::scroll_bind(panel,&theme);
    HWND child=CreateWindowExW(0,L"EDIT",L"123",WS_CHILD|WS_TABSTOP,0,0,1,1,panel,nullptr,module,nullptr);
    psui::scroll_begin(panel);psui::scroll_place(panel,child,{10,800,90,820});psui::scroll_end(panel,1000);
    psui::scroll_ensure_visible(panel,child);check(psui::scroll_position(panel)==670,"keyboard focus scrolls field into view");
    ValidateRect(panel,nullptr);ValidateRect(child,nullptr);psui::scroll_ensure_visible(panel,child);check(!GetUpdateRect(panel,nullptr,FALSE)&&!GetUpdateRect(child,nullptr,FALSE),"visible field focus must not invalidate the page");
    SendMessageW(panel,WM_VSCROLL,SB_BOTTOM,0);check(psui::scroll_position(panel)==850,"scroll bottom clamp");
    SendMessageW(panel,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),0);check(psui::scroll_position(panel)<850,"mouse wheel scroll");
    psui::scroll_begin(panel);psui::scroll_place(panel,child,{10,20,90,40});psui::scroll_end(panel,60);check(psui::scroll_position(panel)==0,"collapsing sections clamps offset");
    wchar_t text[32];GetWindowTextW(child,text,32);check(std::wstring(text)==L"123","reflow retains field state");
    DestroyWindow(panel);
    psui::release_widgets(module);
    WNDCLASSW gone{}; check(!GetClassInfoW(module,psui::slider_class,&gone),"no dangling DLL widget class after close");
    check(!GetClassInfoW(module,psui::scroll_panel_class,&gone),"no dangling scroll panel class after close");
}
void painting_tests(){
    psui::Theme theme;HDC target=CreateCompatibleDC(nullptr);BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=240;info.bmiHeader.biHeight=-100;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    void* bits=nullptr;auto bitmap=CreateDIBSection(target,&info,DIB_RGB_COLORS,&bits,nullptr,0);auto old=SelectObject(target,bitmap);auto* pixels=static_cast<std::uint32_t*>(bits);check(bitmap&&pixels,"paint test target");
    const RECT rect{7,11,207,85};
    auto draw=[&](int level){theme.apply(nullptr,level);std::fill(pixels,pixels+240*100,0x00102030);const auto before=GetPixel(target,10,20);
      check(psui::buffered_paint(target,rect,theme,[&](HDC memory){
        check(memory!=target,"preview display callback must receive memory DC");
        auto brush=CreateSolidBrush(RGB(140,90,60));FillRect(memory,&rect,brush);DeleteObject(brush);GdiFlush();
        check(GetPixel(target,10,20)==before,"in-progress preview must preserve the last visible frame");
      }),"buffer commit");GdiFlush();
      check(GetPixel(target,10,20)==RGB(140,90,60)&&GetPixel(target,0,0)==before,"single commit and bounded coordinates");
    };
    draw(3);const auto resources=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    for(int i=0;i<400;++i)draw(i%4);
    check(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=resources+1,"buffered preview has no per-frame GDI leak");
    const auto before=GetPixel(target,10,20);
    try{psui::buffered_paint(target,rect,theme,[&](HDC memory){FillRect(memory,&rect,theme.field());throw std::runtime_error("incomplete frame");});}catch(const std::runtime_error&){}
    check(GetPixel(target,10,20)==before,"failed render never submits a partial frame");
    const auto module=GetModuleHandleW(nullptr);HWND parent=CreateWindowExW(0,L"STATIC",L"Paint regression",WS_POPUP,0,0,240,100,nullptr,nullptr,module,nullptr);
    HWND button=CreateWindowExW(0,L"BUTTON",L"Owner draw",WS_CHILD|BS_OWNERDRAW,0,0,200,70,parent,nullptr,module,nullptr);check(button!=nullptr,"erase test button");
    theme.apply(parent,3,true);
    for(int i=0;i<400;++i){std::fill(pixels,pixels+240*100,0x00102030);SendMessageW(button,WM_ERASEBKGND,reinterpret_cast<WPARAM>(target),0);GdiFlush();check(std::all_of(pixels,pixels+240*100,[](auto v){return (v&0xffffff)==0x102030;}),"owner draw erase preserves last frame");}
    DestroyWindow(parent);SelectObject(target,old);DeleteObject(bitmap);DeleteDC(target);
}
}
int main() {
    preview_view_tests();generator_preview_tests();localization_tests();painting_tests();widget_tests(); proxy_tests();
    std::cout<<"PASS: custom double-buffered slider; 960 white-poisoned frames/four themes; no stale thumb pixels/GDI leak; keyboard/ranges; bounded RGBA preview + selection mask; no output requests; host state restore/cancel.\n";
}
