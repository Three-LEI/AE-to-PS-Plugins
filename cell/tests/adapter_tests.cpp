#include "../../displacement/tests/mock_host.hpp"
#include "cell_host.hpp"
#include <iostream>
#include <cstdlib>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
int checks=0;
void check(bool v,const char* m){++checks;if(!v){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
struct Saved {unsigned long magic=0x5A434531;cell::Parameters p;};
void parameters(mock::Host& host,cell::Parameters p){Saved saved;saved.p=p;if(host.f.parameters)mock::dispose(host.f.parameters);host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));}
void run(Entry entry,int depth,bool modern,bool flat,cell::Mode mode,bool locked=false){
 const bool procedural=!cell::samples_source(mode);
 mock::Host host(530,75,depth);if(!modern)host.f.bigDocumentData=nullptr;
 if(flat){host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageWithSelection;host.doc.targetTransparency=nullptr;}
 if(locked)host.f.filterCase=filterCaseProtectedTransparencyWithSelection;
 auto p=cell::defaults(530,75);p.mode=mode;parameters(host,p);
 cell::Image native;check(cell::read_source(&host.f,0,native)==noErr,"independent source snapshot");
 const auto source=cell::source_view(native,host.doc.bounds);cell::Image expected;const VRect area{3,4,72,527};host.big.filterRect32=area;host.f.filterRect={3,4,72,527};
 check(cell::render(p,{4,3,1},523,69,expected,source,{},locked),"independent whole-region reference");
 if(procedural&&!locked){host.f.channelPortProcs=nullptr;host.doc.targetCompositeChannels=nullptr;}
 intptr_t state=0;auto call=[&](int n){int16 result=999;entry(int16(n),&host.f,&state,&result);return result;};
 check(call(filterSelectorPrepare)==noErr&&call(filterSelectorStart)==noErr&&state>1,"native DLL starts");
 const int n=flat||locked?3:4,bytes=depth/8,stride=530*n*bytes+8;std::vector<std::uint8_t> output(std::size_t(stride)*75,0xCD);int tiles=0;
 check(host.f.outHiPlane==n-1&&host.f.autoMask,"host-managed selection and correct alpha layout");
 while(true){const auto r=modern?host.big.outRect32:VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};if(r.bottom<=r.top)break;
  check(++tiles<=4,"bounded irregular tiles");host.f.outData=output.data()+std::size_t(r.top)*stride+r.left*n*bytes;host.f.outRowBytes=stride;host.f.outColumnBytes=n*bytes;host.f.outPlaneBytes=bytes;check(call(filterSelectorContinue)==noErr,"native output tile");
  // Simulate an in-place host: early output overwrites the source ports. Later
  // crystallize tiles must still sample the frozen, original target snapshot.
  if(!procedural)for(int y=r.top;y<r.bottom;++y)for(int x=r.left;x<r.right;++x)for(int k=0;k<4;++k)host.pixels[0][(std::size_t(y)*530+x)*4+k]=0;
 }
 check(tiles==4,"all four output tiles processed");
 for(int y=0;y<75;++y)for(int b=0;b<stride;++b){const int x=b/(n*bytes),k=(b%(n*bytes))/bytes,sub=b%bytes;const auto offset=std::size_t(y)*stride+b;
  if(y<area.top||y>=area.bottom||x<area.left||x>=area.right){check(output[offset]==0xCD,"selection exterior and padding untouched");continue;}
  const auto i=(std::size_t(y-area.top)*expected.width+x-area.left)*4;const float value=expected.rgba[i+k]*(flat&&k<3?expected.rgba[i+3]:1);std::uint8_t wanted[4]{};
  if(bytes==1)wanted[0]=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));else if(bytes==2){const auto v=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(wanted,&v,2);}else std::memcpy(wanted,&value,4);
  check(output[offset]==wanted[sub],"native sample conversion equals independent reference");
 }
 check(call(filterSelectorFinish)==noErr&&state==0,"session freed");
 mock::cancel=true;check(call(filterSelectorStart)==userCanceledErr&&state==0,"cancel before render");mock::cancel=false;
}
int main(){
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::wstring file(path);file.resize(file.find_last_of(L"\\/")+1);file+=L"ZaYuCellPattern.8bf";
 auto dll=LoadLibraryW(file.c_str());check(dll!=nullptr,"standalone DLL loads without AE");auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(entry!=nullptr,"exported PluginMain");
 for(int depth:{8,16,32})for(bool modern:{false,true})for(bool flat:{false,true})for(auto mode:{cell::Mode::crystals,cell::Mode::crystallize_hq})run(entry,depth,modern,flat,mode);
 for(int depth:{8,16,32})for(int mode=0;mode<int(cell::Mode::count);++mode)run(entry,depth,true,false,cell::Mode(mode));
 for(int depth:{8,16,32})run(entry,depth,true,false,cell::Mode::crystals,true);
 mock::Host host(64,64);auto p=cell::defaults(64,64);p.mode=cell::Mode::crystallize;parameters(host,p);intptr_t state=0;int16 result;
 mock::fail_read=true;entry(filterSelectorStart,&host.f,&state,&result);check(result==-123&&state==0,"source read error preserved without session leak");mock::fail_read=false;
 host.f.channelPortProcs=nullptr;entry(filterSelectorStart,&host.f,&state,&result);check(result!=noErr&&state==0,"unavailable crystallize source rejected");
 p.mode=cell::Mode::bubbles;parameters(host,p);host.doc.bounds={0,0,100000,100000};host.big.filterRect32={50000,50000,50010,50010};host.big.imageSize32={100000,100000};
 entry(filterSelectorStart,&host.f,&state,&result);check(result==noErr&&state>1,"large blank document procedural path has no full snapshot");std::vector<unsigned char> small(400);host.f.outData=small.data();host.f.outRowBytes=40;host.f.outColumnBytes=4;host.f.outPlaneBytes=1;
 entry(filterSelectorContinue,&host.f,&state,&result);check(result==noErr,"large-coordinate output");entry(filterSelectorFinish,&host.f,&state,&result);check(state==0,"large session cleanup");
 FreeLibrary(dll);std::cout<<"PASS: "<<checks<<" assertions; actual DLL, RGB8/PS16/HDR32; blank/source/flat/protected alpha; irregular tiles/selection; immutable source cache; large coordinates; errors/cancel; no AE dependency.\n";
}
