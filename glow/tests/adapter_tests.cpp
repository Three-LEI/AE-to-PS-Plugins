#include "../../displacement/tests/mock_host.hpp"
#include "glow_host.hpp"
#include <iostream>
#include <cstdlib>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
void check(bool v,const char* m){if(!v){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
struct Saved {unsigned long magic=glow::effect_kind==glow::Kind::deep?0x5A444731:0x5A474C31;glow::Parameters p=glow::defaults(glow::effect_kind);};
void parameters(mock::Host& host,Saved& saved){
 if(host.f.parameters)mock::dispose(host.f.parameters);
 host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));
}
void verify(Entry entry,int depth,bool modern,bool flat){
 mock::Host host(1036,142,depth);if(!modern)host.f.bigDocumentData=nullptr;
 if(flat){host.doc.layersDescriptor=nullptr;host.doc.targetTransparency=nullptr;host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageNoSelection;}
 Saved saved;saved.p.intensity=0;parameters(host,saved);
 intptr_t state=0;auto call=[&](int n){int16 result=999;entry(int16(n),&host.f,&state,&result);return result;};
 const VRect area{5,7,136,1032};host.big.filterRect32=area;host.f.filterRect={5,7,136,1032};
 check(call(filterSelectorPrepare)==noErr&&call(filterSelectorStart)==noErr&&state>1,"start actual DLL on readable target");
 const int n=flat?3:4,bytes=depth/8,stride=1036*n*bytes+16;std::vector<std::uint8_t> output(std::size_t(stride)*142,0xCD);int tiles=0;
 check(host.f.outHiPlane==n-1&&host.f.autoMask,"RGBA/flat layout and host-managed selection");
 while(true){const auto r=modern?host.big.outRect32:VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};if(r.bottom<=r.top)break;
  check(++tiles<=9,"tile count bounded");host.f.outData=output.data()+std::size_t(r.top)*stride+r.left*n*bytes;
  host.f.outRowBytes=stride;host.f.outColumnBytes=n*bytes;host.f.outPlaneBytes=bytes;check(call(filterSelectorContinue)==noErr,"write native tile");
 }
 check(tiles==9,"irregular tiles processed");
 for(int y=0;y<142;++y)for(int byte=0;byte<stride;++byte){
  const int x=byte/(n*bytes),k=(byte%(n*bytes))/bytes,sub=byte%bytes;
  if(y<area.top||y>=area.bottom||x<area.left||x>=area.right){check(output[std::size_t(y)*stride+byte]==0xCD,"selection exterior and row padding untouched");continue;}
  const auto i=(std::size_t(y)*1036+x)*4;float value=host.pixels[0][i+k];if(!flat&&k<3&&host.pixels[0][i+3]==0)value=0;
  std::uint8_t expected[4]{};if(bytes==1)expected[0]=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));
  else if(bytes==2){const auto q=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(expected,&q,2);}else std::memcpy(expected,&value,4);
  check(output[std::size_t(y)*stride+byte]==expected[sub],"identity matches independent native sample conversion");
 }
 check(call(filterSelectorFinish)==noErr&&state==0,"session cleanup");
 mock::cancel=true;check(call(filterSelectorStart)==userCanceledErr&&state==0,"cancel during source read");mock::cancel=false;
 mock::fail_read=true;check(call(filterSelectorStart)==-123&&state==0,"read error preserved");mock::fail_read=false;
 host.doc.targetCompositeChannels=nullptr;check(call(filterSelectorStart)==errReportString&&state==0,"absent channel ports rejected");
}

#ifdef ZAYU_DEEP_GLOW
void verify_nonzero(Entry entry,int depth,bool flat,bool hdr=false){
 mock::Host host(224,128,depth);if(flat){host.doc.targetTransparency=nullptr;host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageNoSelection;}
 for(int y=0;y<128;y++)for(int x=0;x<224;x++){auto i=(std::size_t(y)*224+x)*4;host.pixels[0][i]=hdr?2.f:.1f+.8f*x/223;host.pixels[0][i+1]=.2f+.7f*y/127;host.pixels[0][i+2]=x>100&&x<130?.95f:.4f;host.pixels[0][i+3]=flat?1.f:(x>30&&x<194?.75f:0.f);}
 Saved saved;if(hdr)saved.p.blend=glow::Blend::add;parameters(host,saved);
 psui::HostLayer target;for(int k=0;k<3;k++)target.rgb[k]=&host.channels[0][k];target.alpha=flat?nullptr:host.doc.targetTransparency;
 psui::LayerPixels pixels;check(!psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels),"native reference source read");auto source=glow::from_host(std::move(pixels));glow::Image expected;check(glow::render(source,saved.p,expected),"nonzero default reference render");
 intptr_t state=0;auto call=[&](int selector){int16 result=999;entry(int16(selector),&host.f,&state,&result);return result;};check(!call(filterSelectorPrepare)&&!call(filterSelectorStart),"nonzero DLL start");
 const int channels=flat?3:4,bytes=depth/8,row=224*channels*bytes+16;std::vector<std::uint8_t> output(std::size_t(row)*128,0xCD);
 while(true){const auto r=host.big.outRect32;if(r.bottom<=r.top)break;host.f.outData=output.data()+std::size_t(r.top)*row+r.left*channels*bytes;host.f.outRowBytes=row;host.f.outColumnBytes=channels*bytes;host.f.outPlaneBytes=bytes;check(!call(filterSelectorContinue),"nonzero DLL tile");}
 for(int y=0;y<128;y++){
  for(int x=0;x<224;x++){for(int k=0;k<channels;k++){
  const auto i=(std::size_t(y)*224+x)*4+k;float v=expected.rgba[i];std::uint8_t native[4]{};
  if(bytes==1)native[0]=std::uint8_t(std::lround(std::clamp(v,0.f,1.f)*255));else if(bytes==2){auto q=std::uint16_t(std::lround(std::clamp(v,0.f,1.f)*32768));std::memcpy(native,&q,2);}else std::memcpy(native,&v,4);
  check(!std::memcmp(native,output.data()+std::size_t(y)*row+(x*channels+k)*bytes,bytes),"nonzero radius100/intensity1 DLL samples equal core");
 }}
  for(int x=224*channels*bytes;x<row;x++)check(output[std::size_t(y)*row+x]==0xCD,"nonzero padding guard");}
 if(hdr)check(expected.rgba[0]>1,"native HDR additive output remains unclipped");check(!call(filterSelectorFinish)&&state==0,"nonzero DLL finish");
}
#endif
int main(){
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::wstring file(path);file.resize(file.find_last_of(L"\\/")+1);
#ifdef ZAYU_DEEP_GLOW
 file+=L"ZaYuDeepGlow.8bf";
#else
 file+=L"ZaYuGlow.8bf";
#endif
 const auto dll=LoadLibraryW(file.c_str());check(dll!=nullptr,"standalone plugin loads");const auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(entry!=nullptr,"PluginMain");
 for(int depth:{8,16,32})for(bool modern:{false,true})for(bool flat:{false,true})verify(entry,depth,modern,flat);

#ifdef ZAYU_DEEP_GLOW
 for(int depth:{8,16,32})for(bool flat:{false,true})verify_nonzero(entry,depth,flat);verify_nonzero(entry,32,true,true);
 std::cout<<"PASS: nonzero default Deep Glow DLL/core samples; RGB8/PS16/HDR32; flat/alpha; HDR Add; tile guards.\n";
#endif
 FreeLibrary(dll);std::cout<<"PASS: real DLL; RGB8/PS16/HDR32; alpha and flat documents; 9 irregular tiles; selection guards; 16/32-bit coordinates; cancellation/read errors; no AE dependency.\n";
}
