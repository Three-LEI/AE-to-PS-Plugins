#include "../../displacement/tests/mock_host.hpp"
#include "roughen_host.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
int checks=0;void check(bool x,const char* why){++checks;if(!x){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
struct Saved{unsigned long magic=roughen::magic;roughen::Parameters p;};
void run(Entry entry,int depth,bool modern,bool flat,bool locked){
 mock::Host host(530,75,depth);if(!modern)host.f.bigDocumentData=nullptr;
 if(flat){host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageWithSelection;host.doc.targetTransparency=nullptr;}
 if(locked)host.f.filterCase=filterCaseProtectedTransparencyWithSelection;
 auto p=roughen::defaults();p.border=18;p.scale=37;p.complexity=2;p.sharpness=.7;p.type=5;p.color={1,.5,.1};
 auto layers=psui::host_layers(&host.f);check(!layers.empty(),"host layers");

 Saved saved;saved.p=p;host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));
 psui::LayerPixels read;
 psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;++k,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;
 check(psui::host_read_layer(&host.f,target,host.doc.bounds,0,read)==noErr,"source snapshot");auto source=roughen::from_host(std::move(read));roughen::Image expected;
 check(roughen::render(source,p,expected),"independent full source");
 const VRect area{3,4,72,527};host.big.filterRect32=area;host.f.filterRect={3,4,72,527};intptr_t state=0;
 auto call=[&](int s){int16 result=999;entry(int16(s),&host.f,&state,&result);return result;};
 check(call(filterSelectorPrepare)==noErr&&call(filterSelectorStart)==noErr&&state>1,"DLL starts");
 const int n=flat||locked?3:4,bytes=depth/8,stride=530*n*bytes+8;std::vector<unsigned char> output(std::size_t(stride)*75,0xCD);int tiles=0;
 while(true){const auto r=modern?host.big.outRect32:VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};if(r.bottom<=r.top)break;
  check(++tiles<=4,"bounded tiles");host.f.outData=output.data()+std::size_t(r.top)*stride+r.left*n*bytes;host.f.outRowBytes=stride;host.f.outColumnBytes=n*bytes;host.f.outPlaneBytes=bytes;check(call(filterSelectorContinue)==noErr,"tile write");}
 check(tiles==4,"irregular tiles");
 for(int y=0;y<75;++y)for(int b=0;b<stride;++b){const int x=b/(n*bytes),k=(b%(n*bytes))/bytes,sub=b%bytes;auto actual=output[std::size_t(y)*stride+b];
  if(y<3||y>=72||x<4||x>=527){check(actual==0xCD,"selection/padding guard");continue;}
  auto i=(std::size_t(y)*530+x)*4;const float value=expected.rgba[i+k]*(flat&&k<3?expected.rgba[i+3]:1);unsigned char wanted[4]{};
  if(bytes==1)wanted[0]=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));else if(bytes==2){auto q=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(wanted,&q,2);}else std::memcpy(wanted,&value,4);
  check(actual==wanted[sub],"native RGBA equals whole-image reference");
 }
 check(call(filterSelectorFinish)==noErr&&state==0,"session cleanup");mock::cancel=true;check(call(filterSelectorStart)==userCanceledErr&&state==0,"cancel cleanup");mock::cancel=false;
 mock::fail_read=true;check(call(filterSelectorStart)!=noErr&&state==0,"read-error cleanup");mock::fail_read=false;
 auto invalid=saved;invalid.p.scale=0;std::memcpy(mock::lock(host.f.parameters,true),&invalid,sizeof(invalid));check(call(filterSelectorStart)==filterBadParameters&&state==0,"invalid saved parameters rejected");
}
int main(){wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::filesystem::path file(path);
 file=file.parent_path()/(roughen::binary_name);auto dll=LoadLibraryW(file.c_str());check(bool(dll),"standalone DLL loads");auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(bool(entry),"PluginMain");
 for(int depth:{8,16,32})for(bool modern:{false,true})for(bool flat:{false,true})run(entry,depth,modern,flat,false);for(int depth:{8,16,32})run(entry,depth,true,false,true);
 FreeLibrary(dll);std::cout<<"PASS: "<<checks<<" assertions; actual DLL RGB8/PS16/HDR32; eight-mode alpha/color roughening; flat/protected alpha; irregular tiles; old/new coordinates; padding/selection; cancel/read-error cleanup.\n";}
