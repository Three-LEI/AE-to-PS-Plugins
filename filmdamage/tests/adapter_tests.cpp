#include "../../displacement/tests/mock_host.hpp"
#include "filmdamage_host.hpp"
#include <iostream>
#include <cstdlib>
#include <filesystem>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
int checks=0;void check(bool x,const char* why){++checks;if(!x){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
struct Saved{unsigned long magic=filmdamage::magic;filmdamage::Parameters p;};
void run(Entry entry,int depth,bool modern,bool flat,bool locked,int legacy=0){
 mock::Host host(530,75,depth);if(!modern)host.f.bigDocumentData=nullptr;
 if(flat){host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageWithSelection;host.doc.targetTransparency=nullptr;}
 if(locked)host.f.filterCase=filterCaseProtectedTransparencyWithSelection;
 filmdamage::Parameters p=filmdamage::preset(4);p.grain=16;p.seed=91;p.defocus=1;p.shake=1;
 if(legacy!=1){p.grain_softness=60;p.halation=48;p.halation_radius=4;p.halation_threshold=25;p.damage_variation=317;}
 if(!legacy||legacy>=3)p.stain_texture=75;
 if(!legacy||legacy>=4)p.stain_edge_roughness=82;
 if(!legacy)p.stain_edge_size=28;
 auto layers=psui::host_layers(&host.f);check(!layers.empty(),"host layers");

 constexpr std::size_t v01_size=384;
 constexpr std::size_t v02_size=424;
 static_assert(v01_size==offsetof(Saved,p)+offsetof(filmdamage::Parameters,grain_softness));
 static_assert(v02_size==offsetof(Saved,p)+offsetof(filmdamage::Parameters,stain_texture));
 constexpr std::size_t v03_size=432;
 static_assert(v03_size==offsetof(Saved,p)+offsetof(filmdamage::Parameters,stain_edge_roughness));
 constexpr std::size_t v04_size=440;
 static_assert(v04_size==offsetof(Saved,p)+offsetof(filmdamage::Parameters,stain_edge_size));
 Saved saved;saved.p=p;const auto bytes_to_copy=legacy==1?v01_size:legacy==2?v02_size:legacy==3?v03_size:legacy==4?v04_size:sizeof(saved);
 host.f.parameters=mock::allocate(bytes_to_copy);std::memcpy(mock::lock(host.f.parameters,true),&saved,bytes_to_copy);
 psui::LayerPixels read;
 psui::HostLayer target;auto* channel=host.doc.targetCompositeChannels;for(int k=0;k<3;++k,channel=channel->next)target.rgb[k]=channel;target.alpha=host.doc.targetTransparency;
 check(psui::host_read_layer(&host.f,target,host.doc.bounds,0,read)==noErr,"source snapshot");auto source=filmdamage::from_host(std::move(read));filmdamage::Image expected;
 check(filmdamage::render(source,p,expected),"independent full source");
 const VRect area{3,4,72,527};host.big.filterRect32=area;host.f.filterRect={3,4,72,527};intptr_t state=0;
 auto call=[&](int s){int16 result=999;entry(int16(s),&host.f,&state,&result);return result;};
 check(call(filterSelectorPrepare)==noErr&&call(filterSelectorStart)==noErr&&state>1,"DLL starts");
 check(host.f.handleProcs->getSizeProc(host.f.parameters)==sizeof(saved),"current handle size after start");
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
 auto invalid=saved;invalid.p.grain=-1;std::memcpy(mock::lock(host.f.parameters,true),&invalid,sizeof(invalid));check(call(filterSelectorStart)==filterBadParameters&&state==0,"invalid saved parameters rejected");
}
int main(){wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::filesystem::path file(path);
 file=file.parent_path()/(filmdamage::binary_name);auto dll=LoadLibraryW(file.c_str());check(bool(dll),"standalone DLL loads");auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(bool(entry),"PluginMain");
 for(int depth:{8,16,32})for(bool modern:{false,true})for(bool flat:{false,true})run(entry,depth,modern,flat,false);for(int depth:{8,16,32})run(entry,depth,true,false,true);
 run(entry,16,true,false,false,1);
 run(entry,16,true,false,false,2);
 run(entry,16,true,false,false,3);
 run(entry,16,true,false,false,4);
 FreeLibrary(dll);std::cout<<"PASS: "<<checks<<" assertions; actual DLL RGB8/PS16/HDR32; film grain, dust, stains, hairs, scratches, weighted blur and gate weave; flat/protected alpha; irregular tiles; old/new coordinates; padding/selection; cancel/read-error cleanup.\n";}
