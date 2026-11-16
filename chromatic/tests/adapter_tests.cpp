#include "../../displacement/tests/mock_host.hpp"
#include "chromatic_host.hpp"
#include "chromatic_legacy.hpp"
#include "psui/preview_background.hpp"
#include <iostream>
#include <cstdlib>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
void check(bool v,const char* m){if(!v){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
struct Saved {unsigned long magic=0x5A434431;chromatic::Parameters p=chromatic::preset(2);};
void split_and_selection(Entry entry,int depth){
 mock::Host host(96,64,depth);for(int y=0;y<64;++y)for(int x=0;x<96;++x){const auto i=(std::size_t(y)*96+x)*4;for(int k=0;k<4;++k)host.pixels[0][i+k]=x>=32&&x<64?1:0;}
 Saved saved;saved.p=chromatic::preset(1);saved.p.amount=4;
 host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));
 intptr_t state=0;auto call=[&](int n){int16 result;entry(int16(n),&host.f,&state,&result);return result;};
 check(call(filterSelectorStart)==noErr,"non-identity RGB split starts");const int bytes=depth/8;std::vector<std::uint8_t> output(96*64*4*bytes);
 host.f.outData=output.data();host.f.outRowBytes=96*4*bytes;host.f.outColumnBytes=4*bytes;host.f.outPlaneBytes=bytes;
 check(call(filterSelectorContinue)==noErr,"native RGB fringe output");
 auto sample=[&](int x,int k){const auto i=(std::size_t(32)*96+x)*4+k;if(bytes==1)return double(output[i])/255;if(bytes==2){std::uint16_t q;std::memcpy(&q,output.data()+i*2,2);return q/32768.;}float q;std::memcpy(&q,output.data()+i*4,4);return double(q);};
 check(sample(28,2)>.99&&sample(28,0)==0&&sample(28,3)>.99,"native blue fringe and expanded alpha");check(sample(67,0)>.99&&sample(67,2)==0,"native red fringe");check(call(filterSelectorFinish)==noErr&&state==0,"native split session released");
 psui::ProxyImage geometry;geometry.width=4;geometry.height=3;geometry.step=2;
 std::vector<float> selection;check(psui::host_read_preview_channel(&host.f,&host.channels[1][0],geometry,selection)==noErr&&selection.size()==12,"bounded native selection-channel reader");
 for(float value:selection)check(value==1,"native mask depth conversion");
}
void parameters(mock::Host& host,Saved& saved){
 if(host.f.parameters)mock::dispose(host.f.parameters);
 host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));
}
void verify(Entry entry,int depth,bool modern,bool flat){
 mock::Host host(1036,142,depth);if(!modern)host.f.bigDocumentData=nullptr;
 if(flat){host.doc.layersDescriptor=nullptr;host.doc.targetTransparency=nullptr;host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageNoSelection;}
 Saved saved;saved.p.mix=0;parameters(host,saved);
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
 mock::fail_read=true;check(call(filterSelectorStart)==-123&&state==0,"read error preserved");mock::fail_read=false;
 host.doc.targetCompositeChannels=nullptr;check(call(filterSelectorStart)==errReportString&&state==0,"absent channel ports rejected");
}

void verify_extended(Entry entry,int depth,bool flat,int preset){
 mock::Host host(128,96,depth);if(flat){host.doc.targetTransparency=nullptr;host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageNoSelection;}
 for(int y=0;y<96;y++)for(int x=0;x<128;x++){auto i=(std::size_t(y)*128+x)*4;host.pixels[0][i]=x/127.f;host.pixels[0][i+1]=y/95.f;host.pixels[0][i+2]=depth==32?1.6f:.6f;host.pixels[0][i+3]=flat?1.f:(x>20&&x<108?.7f:0.f);}
 Saved saved;saved.p=chromatic::preset(preset);parameters(host,saved);
 psui::HostLayer target;for(int k=0;k<3;k++)target.rgb[k]=&host.channels[0][k];target.alpha=flat?nullptr:host.doc.targetTransparency;psui::LayerPixels pixels;check(!psui::host_read_layer(&host.f,target,host.doc.bounds,0,pixels),"extended native source");auto source=chromatic::from_host(std::move(pixels));chromatic::Image expected;check(chromatic::render(source,saved.p,expected),"extended core reference");
 intptr_t state=0;auto call=[&](int n){int16 result=999;entry(int16(n),&host.f,&state,&result);return result;};check(!call(filterSelectorStart)&&state>1,"extended DLL start");const int channels=flat?3:4,bytes=depth/8,row=128*channels*bytes+16;std::vector<std::uint8_t> output(std::size_t(row)*96,0xCD);
 while(true){auto r=host.big.outRect32;if(r.bottom<=r.top)break;host.f.outData=output.data()+std::size_t(r.top)*row+r.left*channels*bytes;host.f.outRowBytes=row;host.f.outColumnBytes=channels*bytes;host.f.outPlaneBytes=bytes;check(!call(filterSelectorContinue),"extended DLL tile");}
 for(int y=0;y<96;y++){
  for(int x=0;x<128;x++)for(int k=0;k<channels;k++){const auto pixel=(std::size_t(y)*128+x)*4;float value=expected.rgba[pixel+k]*(flat&&k<3?expected.rgba[pixel+3]:1);std::uint8_t native[4]{};if(bytes==1)native[0]=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));else if(bytes==2){auto q=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(native,&q,2);}else std::memcpy(native,&value,4);if(std::memcmp(native,output.data()+std::size_t(y)*row+(x*channels+k)*bytes,bytes)){std::cerr<<"MISMATCH preset="<<preset<<" depth="<<depth<<" flat="<<flat<<" x="<<x<<" y="<<y<<" k="<<k<<" expected="<<value;for(int b=0;b<bytes;b++)std::cerr<<" bytes="<<int(native[b])<<"/"<<int(output[std::size_t(y)*row+(x*channels+k)*bytes+b]);std::cerr<<'\n';check(false,"extended DLL matches core samples");}}
  for(int x=128*channels*bytes;x<row;x++)check(output[std::size_t(y)*row+x]==0xCD,"extended row guard");
 }
 check(!call(filterSelectorFinish)&&state==0,"extended DLL cleanup");
}
void verify_legacy(Entry entry){
 mock::Host host(96,64);chromatic::SavedV1 old;old.values.mode=chromatic::Mode::rgb;old.values.amount=4;old.values.angle=0;old.values.softness=0;
 host.f.parameters=mock::allocate(sizeof(old));std::memcpy(mock::lock(host.f.parameters,true),&old,sizeof(old));auto previous=host.f.parameters;intptr_t state=0;int16 result=999;entry(filterSelectorStart,&host.f,&state,&result);check(!result&&state>1,"actual DLL upgrades legacy memory handle");check(previous!=host.f.parameters&&host.f.handleProcs->getSizeProc(host.f.parameters)==sizeof(Saved),"legacy handle replaced at new size");Saved restored;std::memcpy(&restored,mock::lock(host.f.parameters,true),sizeof(restored));check(restored.p.mode==chromatic::Mode::rgb&&restored.p.amount==4&&restored.p.warp==chromatic::Parameters{}.warp,"legacy values and new defaults retained");entry(filterSelectorFinish,&host.f,&state,&result);check(!result&&state==0,"legacy session cleanup");
}
int main(){
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::wstring file(path);file.resize(file.find_last_of(L"\\/")+1);
file+=L"ZaYuChromaticDispersion.8bf";
 const auto dll=LoadLibraryW(file.c_str());check(dll!=nullptr,"standalone plugin loads");const auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(entry!=nullptr,"PluginMain");
 for(int depth:{8,16,32})for(bool modern:{false,true})for(bool flat:{false,true})verify(entry,depth,modern,flat);
 for(int depth:{8,16,32})split_and_selection(entry,depth);
 for(int depth:{8,16,32})for(bool flat:{false,true})for(int preset:{4,6})verify_extended(entry,depth,flat,preset);verify_legacy(entry);
 std::cout<<"PASS: new modes native nonzero RGB8/PS16/HDR32 flat/alpha output; legacy memory-handle migration.\n";
 FreeLibrary(dll);std::cout<<"PASS: real DLL; RGB8/PS16/HDR32; alpha and flat documents; 9 irregular tiles; selection guards; 16/32-bit coordinates; cancellation/read errors; no AE dependency.\n";
}
