// Capture the real production dialogs with immutable demo inputs. No PS documents
// are opened or edited. Only the host pixel/channel adapter is supplied here.
#include "../displacement/tests/mock_host.hpp"
#include "../native/tests/library_qa.hpp"
#include <wincodec.h>
#include <chrono>
#include <thread>
#include <type_traits>
#if GALLERY_MODULE==0
#include "tint_dialog.hpp"
namespace effect=tint;
#elif GALLERY_MODULE==1
#include "fractal_core.hpp"
namespace effect=fractal;
namespace fractal {INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,Parameters&);}
#elif GALLERY_MODULE==2
#include "displacement_host.hpp"
namespace effect=displacement;
#elif GALLERY_MODULE==3
#include "glow_host.hpp"
namespace effect=glow;
#elif GALLERY_MODULE==4
#include "chromatic_host.hpp"
namespace effect=chromatic;
#elif GALLERY_MODULE==5
#include "vector_host.hpp"
namespace effect=vectorblur;
#elif GALLERY_MODULE==6
#include "pattern_host.hpp"
namespace effect=patterns;
#elif GALLERY_MODULE==7
#include "cell_host.hpp"
namespace effect=cell;
#elif GALLERY_MODULE==8
#include "lens_host.hpp"
namespace effect=lens;
#elif GALLERY_MODULE==9
#include "jpeg_host.hpp"
namespace effect=jpegfx;
#elif GALLERY_MODULE==10
#include "surface_host.hpp"
namespace effect=surface;
#elif GALLERY_MODULE==11
#include "creative_host.hpp"
namespace effect=creative;
#elif GALLERY_MODULE==12
#include "roughen_host.hpp"
namespace effect=roughen;
#elif GALLERY_MODULE==13
#include "lightfx_host.hpp"
namespace effect=lightfx;
#elif GALLERY_MODULE==14
#include "energy_host.hpp"
namespace effect=energy;
#elif GALLERY_MODULE==15
#include "blinds_host.hpp"
namespace effect=blinds;
#elif GALLERY_MODULE==16
#include "halftone_host.hpp"
namespace effect=halftone;
#elif GALLERY_MODULE==17
#include "ascii_host.hpp"
namespace effect=asciifx;
#elif GALLERY_MODULE==18
#include "dither_host.hpp"
namespace effect=dither;
#elif GALLERY_MODULE==19
#include "flare_host.hpp"
namespace effect=flare;
#elif GALLERY_MODULE==20
#include "etching_host.hpp"
namespace effect=etching;
#elif GALLERY_MODULE==21
#include "caustics_host.hpp"
namespace effect=caustics;
#elif GALLERY_MODULE==22
#include "filmdamage_host.hpp"
namespace effect=filmdamage;
#elif GALLERY_MODULE==23
#include "longshadow_host.hpp"
namespace effect=longshadow;
#elif GALLERY_MODULE==24
#include "toner_host.hpp"
namespace effect=toner;
#elif GALLERY_MODULE==25
#include "mirror_host.hpp"
namespace effect=mirror;
#elif GALLERY_MODULE==26
#include "repetile_host.hpp"
namespace effect=repetile;
#elif GALLERY_MODULE==27
#include "noise_host.hpp"
namespace effect=noise;
#elif GALLERY_MODULE==28
#include "texturewrap_host.hpp"
namespace effect=texturewrap;
#elif GALLERY_MODULE==29
#include "weather_host.hpp"
namespace effect=weather;
#elif GALLERY_MODULE==30
#include "channelblur_host.hpp"
namespace effect=channelblur;
#elif GALLERY_MODULE==31
#include "compoundblur_host.hpp"
namespace effect=compoundblur;
#elif GALLERY_MODULE==32
#include "pixelsort_host.hpp"
namespace effect=pixelsort;
#endif
#include "gallery_settings.hpp"
namespace {
mock::Host* proxy_host=nullptr;
std::vector<std::uint8_t> proxy_pixels;
unsigned proxy_reads=0;
// Tint and Fractal use advanceState/inputRate; other dialogs snapshot channels.
// Both interfaces must be present in the standalone screenshot host.
OSErr read_legacy_proxy(){
 if(!proxy_host)return filterBadParameters;
 auto& f=proxy_host->f;const auto r=f.bigDocumentData->inRect32;
 const int w=r.right-r.left,h=r.bottom-r.top,step=std::max(1,int(f.inputRate/65536));
 if(w<=0||h<=0)return noErr;
 if(f.depth!=8||w>4096||h>4096)return filterBadParameters;
 proxy_pixels.resize(std::size_t(w)*h*4);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  const int sx=std::clamp((r.left+x)*step,0,proxy_host->width-1),sy=std::clamp((r.top+y)*step,0,proxy_host->height-1);
  const auto source=(std::size_t(sy)*proxy_host->width+sx)*4,dest=(std::size_t(y)*w+x)*4;
  for(int k=0;k<4;k++)proxy_pixels[dest+k]=std::uint8_t(std::lround(std::clamp(proxy_host->pixels[0][source+k],0.f,1.f)*255));
 }
 f.inRowBytes=w*4;f.inColumnBytes=4;f.inPlaneBytes=1;f.inData=proxy_pixels.data();++proxy_reads;return noErr;
}
}
template<class T>struct ComOwner{T* value=nullptr;~ComOwner(){if(value)value->Release();}T** put(){return &value;}T* operator->()const{return value;}};
std::vector<BYTE> load_pixels(const wchar_t* file,UINT& width,UINT& height){
 ComOwner<IWICImagingFactory> factory;ComOwner<IWICBitmapDecoder> decoder;ComOwner<IWICBitmapFrameDecode> frame;ComOwner<IWICFormatConverter> converter;
 auto require=[](HRESULT hr){if(FAILED(hr))throw std::runtime_error("image decoding failed");};
 require(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.put())));
 require(factory->CreateDecoderFromFilename(file,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,decoder.put()));require(decoder->GetFrame(0,frame.put()));require(frame->GetSize(&width,&height));
 require(factory->CreateFormatConverter(converter.put()));require(converter->Initialize(frame.value,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
 std::vector<BYTE> data(std::size_t(width)*height*4);require(converter->CopyPixels(nullptr,width*4,UINT(data.size()),data.data()));return data;
}
bool capture_window(psui::LibraryPanel& ui,const std::filesystem::path& path){
 auto client=path;client+=L".client.bmp";if(!ui.capture(client.c_str()))return false;
 UINT cw=0,ch=0;auto pixels=load_pixels(client.c_str(),cw,ch);RECT bounds{};GetWindowRect(ui.window(),&bounds);POINT origin{};ClientToScreen(ui.window(),&origin);
 const int w=bounds.right-bounds.left,h=bounds.bottom-bounds.top,ox=origin.x-bounds.left,oy=origin.y-bounds.top;
 if(ox<0||oy<0||ox+int(cw)>w||oy+int(ch)>h)return false;
 BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
 HDC screen=GetDC(ui.window()),memory=CreateCompatibleDC(screen);void* bits=nullptr;HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap){DeleteDC(memory);ReleaseDC(ui.window(),screen);return false;}
 auto previous=SelectObject(memory,bitmap);std::memset(bits,0,std::size_t(w)*h*4);PrintWindow(ui.window(),memory,2);
 for(UINT y=0;y<ch;y++)std::memcpy(static_cast<BYTE*>(bits)+(std::size_t(y+oy)*w+ox)*4,pixels.data()+std::size_t(y)*cw*4,std::size_t(cw)*4);
 const int stride=(w*3+3)&~3;BITMAPFILEHEADER fh{};BITMAPINFOHEADER bh{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(bh);fh.bfSize=fh.bfOffBits+stride*h;
 bh.biSize=sizeof(bh);bh.biWidth=w;bh.biHeight=-h;bh.biPlanes=1;bh.biBitCount=24;std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<char*>(&fh),sizeof(fh));file.write(reinterpret_cast<char*>(&bh),sizeof(bh));std::vector<BYTE> row(stride);
 for(int y=0;y<h;y++){for(int x=0;x<w;x++)std::memcpy(row.data()+x*3,static_cast<BYTE*>(bits)+(std::size_t(y)*w+x)*4,3);file.write(reinterpret_cast<char*>(row.data()),row.size());}
 SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(ui.window(),screen);return file.good();
}

// Exercise the controls inside each effect's real window with the same host as the gallery.
bool check_adjustment_controls(const std::function<INT_PTR()>& show, const wchar_t* capture, bool legacy) {
 constexpr auto toggle = "显示调整控件 / Show Adjustment Controls";
 constexpr auto prefix = "preview.adjustment.";
 psui::PanelTestHook hook;
 hook.software = true;
 hook.initial_scale = legacy ? 2.f : 1.f;
 hook.host_locale = legacy ? "zh_CN" : "en_US";
 hook.force_legacy_dpi = legacy;
 std::vector<std::string> handles;
 ImVec4 start{}, transform{};
 ImVec2 preview_size{};
 int first = -1;
 bool moved = false, complete = false;
 hook.before_frame = [&](psui::LibraryPanel& ui) {
  std::this_thread::sleep_for(std::chrono::milliseconds(4));
  auto& io = ImGui::GetIO();
  io.ClearEventsQueue();
  io.ConfigInputTrickleEventQueue = false;
  io.AddFocusEvent(true);
  if (first < 0) return;
  const int elapsed = ui.frame() - first, index = elapsed / 6, step = elapsed % 6;
  if (index < int(handles.size())) {
   if (step == 0) {
    start = ui.items.at(handles[index]);
    transform = ui.items.at("preview.transform");
    moved = false;
    io.AddMousePosEvent(start.x, start.y);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
   }
   const float delta = (step == 1 ? 12.f : -12.f) * ui.scale();
   if (step == 1 || step == 2) io.AddMousePosEvent(start.x + delta, start.y + .58f * delta);
   if (step == 3) io.AddKeyEvent(ImGuiKey_Escape, true);
   if (step == 4) {
    io.AddKeyEvent(ImGuiKey_Escape, false);
    io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
   }
  } else if (index == int(handles.size()) && step < 2) {
   libraryqa::point(ui, toggle);
   io.AddMouseButtonEvent(ImGuiMouseButton_Left, step == 0);
  }
 };
 hook.tick = [&](psui::LibraryPanel& ui) {
  libraryqa::require(ui.frame() < 1200, "adjustment controls timed out");
  const auto state = ui.items.find("preview.adjustments");
  if (state == ui.items.end()) return;
  if (first < 0) {
   const auto image = ui.items.at("image");
   const auto size = ImGui::GetIO().DisplaySize;
   const auto s = ui.scale(), target = psui::filter_preview_extent * s;
   preview_size = {(image.z - image.x) / s, (image.w - image.y) / s};
   libraryqa::require(image.z <= size.x + .5f && image.w <= size.y + .5f &&
                      image.z - image.x <= target + .5f && image.w - image.y <= target + .5f,
                      "preview fits its window and shared extent");
   if (size.x >= 750 * s) libraryqa::require(std::abs(image.z - image.x - target) < .5f, "standard preview width");
   if (size.y >= 700 * s) libraryqa::require(std::abs(image.w - image.y - target) < .5f, "standard preview height");
   libraryqa::actions_fit(ui);
   if (capture) libraryqa::require(capture_window(ui, capture), "adjustment screenshot failed");
   if (state->second.x == 0) {
    libraryqa::require(!ui.items.contains(toggle) && state->second.y == 0, "preview-only layout omits the adjustment switch");
    complete = true;
    ui.close(false);
    return;
   }
   const auto button = ui.items.at(toggle);
   libraryqa::require(button.y >= image.w && button.z <= size.x + .5f && button.w <= size.y + .5f,
                      "show-controls switch fits below preview");
   for (const auto& [key, position] : ui.items) {
    if (key.starts_with(prefix)) handles.push_back(key);
   }
   libraryqa::require(!handles.empty(), "at least one usable adjustment handle");
   first = ui.frame();
   return;
  }
  const int elapsed = ui.frame() - first - 1, index = elapsed / 6, step = elapsed % 6;
  if (index < int(handles.size())) {
   const auto& key = handles[index];
   if (step == 0) {
    const int expected = std::stoi(key.substr(std::string_view(prefix).size()));
    if (state->second.z != expected) throw std::runtime_error("could not grab " + key);
   }
   const auto position = ui.items.find(key);
   if ((step == 1 || step == 2) && position != ui.items.end())
    moved |= position->second.z != start.z || position->second.w != start.w;
   const auto v = ui.items.at("preview.transform");
   libraryqa::require(v.x == transform.x && v.y == transform.y && v.z == transform.z,
                      "handle drag preserves the viewport");
   if (step == 5) {
    if (!moved) throw std::runtime_error("drag did not update " + key);
    const auto restored = ui.items.at(key);
    libraryqa::require(restored.z == start.z && restored.w == start.w, "Escape restores just the active handle");
   }
  } else if (step == 3) {
   libraryqa::require(state->second.y == 0 && !ui.items.contains(handles.front()), "switch hides all adjustment handles");
   complete = true;
   ui.close(false);
  }
 };
 psui::set_panel_test_hook(&hook);
 const auto result = show();
 psui::set_panel_test_hook(nullptr);
 if (result != IDCANCEL || !complete || !hook.error.empty()) {
  std::cerr << "FAIL: " << gallery_slug << ": " << hook.error << " (dialog=" << result << ")\n";
  return false;
 }
 std::cout << "PASS: " << gallery_slug << " (" << handles.size() << " visible handles, "
           << (legacy ? "zh 200% legacy" : "en 100%") << "); " << preview_size.x << 'x' << preview_size.y
           << " preview; layout, controls, cancel\n";
 return true;
}

int wmain(int argc,wchar_t** argv){try{
 if(argc!=3&&argc!=4)return 2;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);UINT width=0,height=0;auto bytes=load_pixels(argv[1],width,height);if(width!=768||height!=768)throw std::runtime_error("expected 768px demo input");
 mock::Host host(width,height);host.layers[0].unicodeName=reinterpret_cast<const uint16*>(L"演示照片");host.layers[1].unicodeName=reinterpret_cast<const uint16*>(L"演示参考");
#if GALLERY_MODULE==28
 // Photoshop can mark the first channel of both readable layers as target.
 host.channels[0][0].target=host.channels[1][0].target=true;
#endif
 for(std::size_t n=0;n<host.pixels[0].size();n+=4){host.pixels[0][n]=bytes[n+2]/255.f;host.pixels[0][n+1]=bytes[n+1]/255.f;host.pixels[0][n+2]=bytes[n]/255.f;host.pixels[0][n+3]=bytes[n+3]/255.f;}host.pixels[1]=host.pixels[0];
#if GALLERY_MODULE==0 || GALLERY_MODULE==1 || GALLERY_MODULE==24 || GALLERY_MODULE==25
 proxy_host=&host;host.f.samplingSupport=1;host.f.inLayerPlanes=3;host.f.inTransparencyMask=1;
 if(argc!=4||std::wcscmp(argv[3],L"--missing-legacy-proxy")!=0)host.f.advanceState=read_legacy_proxy;
#endif
 const bool legacy_adjustments=argc==4&&std::wcscmp(argv[3],L"--check-adjustments-legacy")==0;
 const bool check_adjustments=legacy_adjustments||(argc==4&&std::wcscmp(argv[3],L"--check-adjustments")==0);
 auto layers=psui::host_layers(&host.f);effect::Parameters p;
#if GALLERY_MODULE==3
 p=effect::defaults(effect::effect_kind);
#elif GALLERY_MODULE==6 || GALLERY_MODULE==7
 p=effect::defaults_for_host(&host.f);
#elif GALLERY_MODULE==8 || GALLERY_MODULE==10 || GALLERY_MODULE==11 || GALLERY_MODULE==13
 p=effect::defaults(effect::effect_kind,width,height);
#elif GALLERY_MODULE==12
 p=effect::defaults();
#elif GALLERY_MODULE==14
 p=effect::unified_defaults(width,height);
#elif GALLERY_MODULE==15 || GALLERY_MODULE==19
 p=effect::defaults(width,height);
#elif GALLERY_MODULE==28
 p=effect::defaults(effect::effect_shape);
#endif
 configure_gallery(p,layers);
#if GALLERY_MODULE==14
 if(check_adjustments){p.input=0;p.start_x=width*.3;p.start_y=height*.4;p.end_x=width*.7;p.end_y=height*.6;}
#endif
#if GALLERY_MODULE==28
 const bool check_self_overlay=argc==4&&std::wcscmp(argv[3],L"--check-self-overlay")==0;
 const bool current_decal=p.texture_key==0&&effect::can_overlay_current_layer(effect::effect_shape,p);
 const bool enable_current_overlay=current_decal&&(p.keep_current_for_self_decal||check_self_overlay);
 p.keep_current_for_self_decal=false;
 constexpr auto overlay_control="保留当前层效果 / Keep Current Layer Effect";
#endif
 const auto saved=p;
#if GALLERY_MODULE!=0 && GALLERY_MODULE!=1 && GALLERY_MODULE!=6 && GALLERY_MODULE!=7
 psui::LayerPixels native;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,native))return 3;auto source=effect::from_host(std::move(native));
#endif
#if GALLERY_MODULE==0 || GALLERY_MODULE==1 || GALLERY_MODULE==6 || GALLERY_MODULE==7
 auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,p);};
#elif GALLERY_MODULE==2
 effect::Image map;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,layers,source,p,map);};
#elif GALLERY_MODULE==3
 effect::Image mask,texture;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,mask,texture);};
#elif GALLERY_MODULE==4 || GALLERY_MODULE==5 || GALLERY_MODULE==9 || GALLERY_MODULE==10 || GALLERY_MODULE==19 || GALLERY_MODULE==21
 effect::Image map;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,map);};
#elif GALLERY_MODULE==8 || GALLERY_MODULE==11 || GALLERY_MODULE==13 || GALLERY_MODULE==15 || GALLERY_MODULE==16 || GALLERY_MODULE==17 || GALLERY_MODULE==18 || GALLERY_MODULE==20 || GALLERY_MODULE==22 || GALLERY_MODULE==23 || GALLERY_MODULE==24 || GALLERY_MODULE==25 || GALLERY_MODULE==26 || GALLERY_MODULE==27 || GALLERY_MODULE==29 || GALLERY_MODULE==30 || GALLERY_MODULE==32
 auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p);};
#elif GALLERY_MODULE==12
 effect::Prepared prepared;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,prepared);};
#elif GALLERY_MODULE==14
 effect::Inputs inputs;configure_geometry(inputs);auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,inputs);};
#elif GALLERY_MODULE==28 || GALLERY_MODULE==31
 effect::Inputs inputs;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,inputs);};
#endif
 if(check_adjustments){
  const bool capture=std::wcscmp(argv[2],L"-")!=0;
  const bool passed=check_adjustment_controls(show,capture?argv[2]:nullptr,legacy_adjustments);
  CoUninitialize();
  return passed&&std::memcmp(&p,&saved,sizeof(p))==0?0:4;
 }
 psui::PanelTestHook hook;hook.software=true;hook.host_locale="zh_CN";hook.initial_scale=GALLERY_MODULE==13||GALLERY_MODULE==14?1.f:1.25f;bool captured=false;int stable=0;
#if GALLERY_MODULE>=26
 int zoom_start=-1;float complete=0;ImVec4 fit{};
#endif
#if GALLERY_MODULE>=27
 int fit_start=-1;float zoom_complete=0;
#endif
#if GALLERY_MODULE==31
 int editor_start=-1;
 bool editor_checked=false;
#endif
#if GALLERY_MODULE==28
 int pan_start=-1;
 int current_overlay=-1, pick_source=-1, enable_overlay=-1, pick_current=-1;
 float source_full_frames=0;
 bool checked_texture_source=false;
 ImVec4 zoomed{}, cached_pan{};
 bool kept_previous=false;
 const auto same_region=[](ImVec4 a,ImVec4 b){return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w;};
#endif
 hook.before_frame=[&](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(10));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);io.AddMousePosEvent(-100,-100);
#if GALLERY_MODULE==14
  const int n=ui.frame();std::string section;
  if(n==3||n==4)for(const auto& [key,rect]:ui.items)if(key.starts_with("外围畸变与纹理 /"))section=key;
  if(n==5||n==6)section="核心 / Core";if(n==7||n==8)section="发光 / Glow";
  if(!section.empty())libraryqa::point(ui,section);
  if(n==3||n==5||n==7)io.AddMouseButtonEvent(0,true);if(n==4||n==6||n==8)io.AddMouseButtonEvent(0,false);
#elif GALLERY_MODULE==19
  const int n=ui.frame();if(n==3||n==4){libraryqa::point(ui,"designer.open");io.AddMouseButtonEvent(0,n==3);}
  if(n==7||n==8){libraryqa::point(ui,"designer.element/8");io.AddMouseButtonEvent(0,n==7);}
#elif GALLERY_MODULE==26
  if(zoom_start>=0){
   const int n=ui.frame()-zoom_start;
   if(n==3){libraryqa::point(ui,"image");io.AddMouseWheelEvent(0,1);}
   if(n==9||n==10){libraryqa::point(ui,"preview.fit");io.AddMouseButtonEvent(0,n==9);}
  }
#elif GALLERY_MODULE>=27
  const int n=ui.frame();
#if GALLERY_MODULE==31
  if(editor_start>=0&&!editor_checked){
   const int step=n-editor_start;
   if(step==2||step==3){libraryqa::point(ui,"gradient.add");io.AddMouseButtonEvent(0,step==2);}
   if(step>=5&&step<=7){
    const auto strip=ui.items.at("compound-gradient.strip");
    const float pad=7*ui.scale(), position=step==5?ui.items.at("compoundblur.state").z:.47f;
    io.AddMousePosEvent(strip.x+pad+position*(strip.z-strip.x-2*pad),strip.w-8*ui.scale());
    if(step!=6)io.AddMouseButtonEvent(0,step==5);
   }
   if(step==9||step==10){libraryqa::point(ui,"gradient.delete");io.AddMouseButtonEvent(0,step==9);}
   if(step==12||step==13||step==15||step==16){
    libraryqa::point(ui,"gradient.reverse");io.AddMouseButtonEvent(0,step==12||step==15);
   }
  }
#endif
  if(zoom_start>=0&&n==zoom_start+3){
   libraryqa::point(ui,"image");io.AddMouseWheelEvent(0,5);
  }
#if GALLERY_MODULE==28
  if(current_overlay>=0&&pick_source<0){
   const int step=n-current_overlay;
   if(step==2||step==3){
    libraryqa::point(ui,overlay_control);io.AddMouseButtonEvent(0,step==2);
   }
  }
  if(pick_source>=0&&!checked_texture_source){
   if(enable_overlay>=0&&pick_current<0){
    const int step=n-enable_overlay;
    if(step==2||step==3){
     libraryqa::point(ui,overlay_control);io.AddMouseButtonEvent(0,step==2);
    }
   }else{
    const bool restoring=pick_current>=0;
    const int step=n-(restoring?pick_current:pick_source);
    if(step==2||step==3){
     libraryqa::point(ui,"texturewrap.source");io.AddMouseButtonEvent(0,step==2);
    }
    if(step==5||step==6){
     libraryqa::point(ui,restoring?"texturewrap.source/current":"texturewrap.source/layer/1");
     io.AddMouseButtonEvent(0,step==5);
    }
   }
  }
  if(pan_start>=0&&n>=pan_start+2&&n<=pan_start+9){
   const int step=n-pan_start;
   const auto image=ui.items.at("image");
   const float x=step>=6?.08f:step>=3?.56f:.5f;
   io.AddMousePosEvent(image.x+(image.z-image.x)*x,image.y+(image.w-image.y)*.25f);
   if(step==2||step==9)io.AddMouseButtonEvent(0,step==2);
  }
#endif
  if(fit_start>=0&&(n==fit_start+3||n==fit_start+4)){
   libraryqa::point(ui,"preview.fit");io.AddMouseButtonEvent(0,n==fit_start+3);
  }
#endif
 };
 hook.tick=[&](psui::LibraryPanel& ui){
  bool ready=false;
#if GALLERY_MODULE==13
  const auto a=ui.items.at("lightfx.state"),b=ui.items.at("lightfx.timing");ready=a.y>=1&&a.z==0&&a.w==0&&b.z==0;
#else
  const auto a=ui.items.at("adaptive.state");ready=a.y>=1&&a.z==0&&a.w==0;
#endif
#if GALLERY_MODULE==26
  if(ready&&zoom_start<0){zoom_start=ui.frame();complete=a.y;fit=ui.items.at("preview.source");}
  if(zoom_start>=0){
   const auto n=ui.frame()-zoom_start;
   const auto roi=ui.items.at("preview.source");
   if(n==7)libraryqa::require(roi.z-roi.x<fit.z-fit.x,"zoom crops display without rebuilding the full preview");
   if(n==13){
    libraryqa::require(roi.x==fit.x&&roi.y==fit.y&&roi.z==fit.z&&roi.w==fit.w,"Fit restores the original view");
    libraryqa::require(a.y==complete&&a.w==0,"zoom and Fit reuse the same full composited frame");
   }
   if(n<13)return;
  }
#elif GALLERY_MODULE>=27
#if GALLERY_MODULE==29
  libraryqa::require(ui.items.contains("天气 / Weather")&&ui.items.contains("输出 / Output")&&
                     ui.items.contains("雨线颜色 / Rain Color")&&
                     ui.items.contains("近景虚化（像素） / Near Defocus (px)"),"rain/snow controls are present");
#elif GALLERY_MODULE==30
  libraryqa::require(ui.items.contains("亮度（像素） / Luminance (px)")&&ui.items.contains("色度（像素） / Chroma (px)")&&
                     ui.items.contains("透明度（像素） / Alpha (px)"),"luminance/chroma and alpha controls are present");
#elif GALLERY_MODULE==31
  libraryqa::require(ui.items.contains("渐变类型 / Gradient Type")&&ui.items.contains("compound-gradient.strip")&&
                     ui.items.contains("gradient.add")&&ui.items.contains("gradient.delete"),"gradient shape and stop controls are present");
  if(!editor_checked){
   if(ready&&editor_start<0)editor_start=ui.frame();
   const auto state=ui.items.at("compoundblur.state");
   if(editor_start>=0){
    const int step=ui.frame()-editor_start;
    if(step==4)libraryqa::require(state.y==p.gradient.count+1,"adding a stop updates the ramp");
    if(step==8)libraryqa::require(state.y==p.gradient.count+1&&std::abs(state.z-.47f)<.005f,"dragging reorders a stop");
    if(step==11)libraryqa::require(state.y==p.gradient.count,"deleting removes the selected stop");
    if(step==14)libraryqa::require(std::abs(state.z-.35f)<.005f,"reverse mirrors the selected stop");
    if(step>=17&&ready){editor_checked=true;std::cout<<"PASS: gradient stop insertion, drag, deletion and reversal\n";}
   }
   if(!editor_checked)return;
  }
#elif GALLERY_MODULE==32
  libraryqa::require(ui.items.contains("条带方向 / Stripe Direction")&&ui.items.contains("亮度下限（%） / Lower Luminance (%)")&&
                     ui.items.contains("最大条带长度（像素） / Max Stripe Length (px)"),"sorting controls are present");
#endif
  libraryqa::require(!ui.items.contains(GALLERY_MODULE==27?"noise.detail":"texturewrap.detail"),"separate zoom shortcut removed");
#if GALLERY_MODULE==28
  if(!checked_texture_source){
   const auto source_state=ui.items.at("texturewrap.source.state");
   libraryqa::require(source_state.y==0,"texture source is readable");
   if(ready&&pick_source<0){
    libraryqa::require(source_state.x==0&&ui.items.contains(overlay_control)==current_decal,
                       "current layer shows the overlay control when the shape supports a decal");
    if(enable_current_overlay){
     if(current_overlay<0){
      libraryqa::require(source_state.z==0,"current layer overlay defaults to off");
      current_overlay=ui.frame();source_full_frames=a.y;
     }
     if(source_state.z!=1||a.y<=source_full_frames)return;
     std::cout<<"PASS: "<<gallery_slug<<" current layer decal toggle renders a complete frame\n";
    }
#ifdef ZAYU_CYLINDER
    const float source_bottom=ui.items.at("texturewrap.source").w;
    for(const char* control:{"包裹角度（度） / Wrap Angle (deg)", "周向位置（度） / Decal Angle (deg)",
                             "上下位置（%） / Decal Position (%)", "贴花高度（%） / Decal Height (%)"}){
     const auto row=ui.items.at(control);
     libraryqa::require(row.y>=source_bottom&&row.w<=ImGui::GetIO().DisplaySize.y,
                        "cylinder decal controls are visible below the texture source");
    }
#endif
    pick_source=ui.frame();source_full_frames=a.y;
   }else if(ready&&enable_overlay<0&&source_state.x==1&&a.y>source_full_frames){
    libraryqa::require(source_state.z==0,"overlay is off when an external source is selected");
    enable_overlay=ui.frame();source_full_frames=a.y;
   }else if(ready&&pick_current<0&&source_state.z==1&&a.y>source_full_frames){
    pick_current=ui.frame();source_full_frames=a.y;
   }else if(ready&&pick_current>=0&&source_state.x==0&&a.y>source_full_frames){
    checked_texture_source=true;
    libraryqa::require(source_state.z==(enable_current_overlay?1:0),
                       "returning to the current layer restores its own overlay setting");
    std::cout<<"PASS: "<<gallery_slug<<" current layer, external overlay, then current layer\n";
   }
   if(!checked_texture_source)return;
  }
#endif
  if(ready&&zoom_start<0){zoom_start=ui.frame();complete=a.y;fit=ui.items.at("preview.source");}
  if(zoom_start>=0){
   const auto roi=ui.items.at("preview.source");
   const auto transform=ui.items.at("preview.transform");
   if(fit_start<0){
#if GALLERY_MODULE==28
    if(pan_start<0){
     if(ready&&a.y>complete&&transform.x>=100&&roi.z-roi.x<fit.z-fit.x){
      pan_start=ui.frame();zoom_complete=a.y;zoomed=roi;
     }
    }else{
     const int step=ui.frame()-pan_start;
     if(step==5){
      libraryqa::require(!same_region(roi,zoomed)&&a.y==zoom_complete&&a.w==0,
                        "small pan uses the completed native pixels without a new render");
      cached_pan=roi;
     }
     if(step>=7&&a.y==zoom_complete){
      libraryqa::require(same_region(roi,cached_pan),"pending crop keeps the previous effect image and transform");
      kept_previous=true;
     }
     if(step>=10&&ready&&a.y>zoom_complete){
      libraryqa::require(kept_previous&&!same_region(roi,cached_pan),"larger pan replaces the effect with a completed crop");
      fit_start=ui.frame();zoom_complete=a.y;
      std::cout<<"PASS: "<<gallery_slug<<" cached and pending pan frames\n";
     }
    }
#else
    if(ready&&a.y>complete&&transform.x>=100&&roi.z-roi.x<fit.z-fit.x){fit_start=ui.frame();zoom_complete=a.y;}
#endif
    return;
   }
   libraryqa::require(ui.frame()-fit_start<480,"Fit settles within the preview timeout");
   if(!ready||a.y<=zoom_complete||transform.w==0)return;
   libraryqa::require(roi.x==fit.x&&roi.y==fit.y&&roi.z==fit.z&&roi.w==fit.w,
                     "wheel zoom renders a native crop and Fit restores the view");
  }
#endif
  if(ready&&ui.frame()>10){if(++stable==6){libraryqa::actions_fit(ui);
   const auto geometry=ui.items.find("preview.source");
   if(geometry==ui.items.end()||geometry->second.z<=geometry->second.x||geometry->second.w<=geometry->second.y)throw std::runtime_error("preview source is empty; capture rejected");
#if GALLERY_MODULE==0 || GALLERY_MODULE==1 || GALLERY_MODULE==24 || GALLERY_MODULE==25
   if(!proxy_reads||proxy_pixels.empty())throw std::runtime_error("legacy preview pixels were not read");
#endif
#if GALLERY_MODULE==14
   unsigned visible=0;for(const auto& [key,rect]:ui.items)if(key.ends_with(".slider")&&rect.y>=0&&rect.w<=ImGui::GetIO().DisplaySize.y){if(key.find("Core Width (px)")!=std::string::npos)visible|=1;if(key.find("Glow Intensity")!=std::string::npos)visible|=2;}
   if(visible!=3)throw std::runtime_error("Saber core/glow controls must be expanded and visible");
#endif
   if(!capture_window(ui,std::filesystem::path(argv[2])))throw std::runtime_error("window capture failed");captured=true;ui.close(false);}}else stable=0;
  if(ui.frame()>2400)ui.close(false);
 };
 psui::set_panel_test_hook(&hook);const auto answer=show();psui::set_panel_test_hook(nullptr);
 if(answer!=IDCANCEL||!captured||!hook.error.empty()||std::memcmp(&p,&saved,sizeof(p))!=0){std::cerr<<"FAIL "<<gallery_slug<<" "<<hook.error<<'\n';return 4;}
 CoUninitialize();std::cout<<"PASS: "<<gallery_slug<<" production parameter window; configured values; full preview; safe cancel\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 5;}}
