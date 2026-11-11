#include "../../displacement/tests/mock_host.hpp"
#include "pattern_host.hpp"
#include "psui/preview_background.hpp"
#include <iostream>
#include <cstdlib>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
void check(bool v,const char* m){if(!v){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
struct Saved {unsigned long magic=(pattern_kind==patterns::Kind::grid?0x5A475231:0x5A434B31);patterns::Parameters p=patterns::Parameters{};};
void parameters(mock::Host& host,Saved& saved){
 if(host.f.parameters)mock::dispose(host.f.parameters);
 host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));
}
void verify(Entry entry,int depth,bool modern,bool flat){
 mock::Host host(1036,142,depth);if(!modern)host.f.bigDocumentData=nullptr;
 if(flat){host.doc.layersDescriptor=nullptr;host.doc.targetTransparency=nullptr;host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageNoSelection;}
 Saved saved;saved.p.opacity=0;saved.p.blend=patterns::Blend::normal;parameters(host,saved);
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
  const auto i=(std::size_t(y)*1036+x)*4;float value=host.pixels[0][i+k];
  std::uint8_t expected[4]{};if(bytes==1)expected[0]=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));
  else if(bytes==2){const auto q=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(expected,&q,2);}else std::memcpy(expected,&value,4);
  check(output[std::size_t(y)*stride+byte]==expected[sub],"identity matches independent native sample conversion");
 }
 check(call(filterSelectorFinish)==noErr&&state==0,"session cleanup");
 mock::cancel=true;check(call(filterSelectorStart)==userCanceledErr&&state==0,"cancel during source read");mock::cancel=false;
 mock::fail_read=true;check(call(filterSelectorStart)==noErr&&call(filterSelectorContinue)==-123&&state==0,"deferred read error preserved");mock::fail_read=false;
 
}
void generator(Entry entry){
 mock::Host host(96,64);Saved saved;saved.p=patterns::defaults(pattern_kind,96,64);saved.p.size_from=patterns::SizeFrom::width;saved.p.width=16;parameters(host,saved);
 host.f.channelPortProcs=nullptr;host.doc.targetCompositeChannels=nullptr;
 intptr_t state=0;auto call=[&](int n){int16 result;entry(int16(n),&host.f,&state,&result);return result;};
 check(call(filterSelectorStart)==noErr,"blank generator starts without source ports");std::vector<unsigned char> output(96*64*4);
 host.f.outData=output.data();host.f.outRowBytes=96*4;host.f.outColumnBytes=4;host.f.outPlaneBytes=1;
 check(call(filterSelectorContinue)==noErr,"blank generation writes RGBA");
 auto alpha=[&](int x,int y){return output[(std::size_t(y)*96+x)*4+3];};
 check(alpha(49,33)>200,"generator creates alpha on a blank layer");check(alpha(56,40)==(pattern_kind==patterns::Kind::grid?0:255),"pattern alpha holes");
 check(call(filterSelectorFinish)==noErr&&state==0,"generator cleanup");
 host.doc.bounds={0,0,100000,100000};host.big.imageSize32={100000,100000};host.big.filterRect32={50000,50000,50010,50010};
 check(call(filterSelectorStart)==noErr,"large procedural document needs no full snapshot");std::vector<unsigned char> small(400);host.f.outData=small.data();host.f.outRowBytes=40;
 check(call(filterSelectorContinue)==noErr&&call(filterSelectorFinish)==noErr,"large-coordinate small tile generated");
}
int main(){
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::wstring file(path);file.resize(file.find_last_of(L"\\/")+1);
file+=pattern_kind==patterns::Kind::grid?L"ZaYuGrid.8bf":L"ZaYuCheckerboard.8bf";
 const auto dll=LoadLibraryW(file.c_str());check(dll!=nullptr,"standalone plugin loads");const auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(entry!=nullptr,"PluginMain");
 for(int depth:{8,16,32})for(bool modern:{false,true})for(bool flat:{false,true})verify(entry,depth,modern,flat);
 generator(entry);
 FreeLibrary(dll);std::cout<<"PASS: real DLL; RGB8/PS16/HDR32; alpha and flat documents; 9 irregular tiles; selection guards; 16/32-bit coordinates; cancellation/read errors; no AE dependency.\n";
}
