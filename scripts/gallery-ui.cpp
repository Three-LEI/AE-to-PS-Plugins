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
#endif
#include GALLERY_SETTINGS
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
int wmain(int argc,wchar_t** argv){try{
 if(argc!=3&&argc!=4)return 2;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);UINT width=0,height=0;auto bytes=load_pixels(argv[1],width,height);if(width!=768||height!=768)throw std::runtime_error("expected 768px demo input");
 mock::Host host(width,height);host.layers[0].unicodeName=reinterpret_cast<const uint16*>(L"演示照片");host.layers[1].unicodeName=reinterpret_cast<const uint16*>(L"演示参考");
 for(std::size_t n=0;n<host.pixels[0].size();n+=4){host.pixels[0][n]=bytes[n+2]/255.f;host.pixels[0][n+1]=bytes[n+1]/255.f;host.pixels[0][n+2]=bytes[n]/255.f;host.pixels[0][n+3]=bytes[n+3]/255.f;}host.pixels[1]=host.pixels[0];
#if GALLERY_MODULE==0 || GALLERY_MODULE==1
 proxy_host=&host;host.f.samplingSupport=1;host.f.inLayerPlanes=3;host.f.inTransparencyMask=1;
 if(argc!=4||std::wcscmp(argv[3],L"--missing-legacy-proxy")!=0)host.f.advanceState=read_legacy_proxy;
#endif
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
#elif GALLERY_MODULE==15
 p=effect::defaults(width,height);
#endif
 configure_gallery(p,layers);const auto saved=p;
#if GALLERY_MODULE!=0 && GALLERY_MODULE!=1 && GALLERY_MODULE!=6 && GALLERY_MODULE!=7
 psui::LayerPixels native;if(psui::host_read_layer(&host.f,layers[0],host.doc.bounds,0,native))return 3;auto source=effect::from_host(std::move(native));
#endif
#if GALLERY_MODULE==0 || GALLERY_MODULE==1 || GALLERY_MODULE==6 || GALLERY_MODULE==7
 auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,p);};
#elif GALLERY_MODULE==2
 effect::Image map;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,layers,source,p,map);};
#elif GALLERY_MODULE==3
 effect::Image mask,texture;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,mask,texture);};
#elif GALLERY_MODULE==4 || GALLERY_MODULE==5 || GALLERY_MODULE==9 || GALLERY_MODULE==10
 effect::Image map;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,layers,p,map);};
#elif GALLERY_MODULE==8 || GALLERY_MODULE==11 || GALLERY_MODULE==13 || GALLERY_MODULE==15
 auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p);};
#elif GALLERY_MODULE==12
 effect::Prepared prepared;auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,prepared);};
#elif GALLERY_MODULE==14
 effect::Inputs inputs;configure_geometry(inputs);auto show=[&]{return effect::show_dialog(GetModuleHandleW(nullptr),nullptr,&host.f,source,p,inputs);};
#endif
 psui::PanelTestHook hook;hook.software=true;hook.host_locale="zh_CN";hook.initial_scale=GALLERY_MODULE==13||GALLERY_MODULE==14?1.f:1.25f;bool captured=false;int stable=0;
 hook.before_frame=[](psui::LibraryPanel& ui){std::this_thread::sleep_for(std::chrono::milliseconds(10));auto& io=ImGui::GetIO();io.ConfigInputTrickleEventQueue=false;io.ClearEventsQueue();io.AddFocusEvent(true);io.AddMousePosEvent(-100,-100);
#if GALLERY_MODULE==14
  const int n=ui.frame();std::string section;
  if(n==3||n==4)for(const auto& [key,rect]:ui.items)if(key.starts_with("外围畸变与纹理 /"))section=key;
  if(n==5||n==6)section="核心 / Core";if(n==7||n==8)section="发光 / Glow";
  if(!section.empty())libraryqa::point(ui,section);
  if(n==3||n==5||n==7)io.AddMouseButtonEvent(0,true);if(n==4||n==6||n==8)io.AddMouseButtonEvent(0,false);
#endif
 };
 hook.tick=[&](psui::LibraryPanel& ui){
  bool ready=false;
#if GALLERY_MODULE==13
  const auto a=ui.items.at("lightfx.state"),b=ui.items.at("lightfx.timing");ready=a.y>=1&&a.z==0&&a.w==0&&b.z==0;
#else
  const auto a=ui.items.at("adaptive.state");ready=a.y>=1&&a.z==0&&a.w==0;
#endif
  if(ready&&ui.frame()>10){if(++stable==6){libraryqa::actions_fit(ui);
   const auto geometry=ui.items.find("preview.source");
   if(geometry==ui.items.end()||geometry->second.z<=geometry->second.x||geometry->second.w<=geometry->second.y)throw std::runtime_error("preview source is empty; capture rejected");
#if GALLERY_MODULE==0 || GALLERY_MODULE==1
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
