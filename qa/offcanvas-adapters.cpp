// Exercise the actual DLL entry points with canvas and raster-layer rectangles
// deliberately different. No Photoshop session or personal document is used.
#include "../displacement/tests/mock_host.hpp"
#include "psui/image_bounds.hpp"
#include "../cell/include/cell_core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
std::size_t cases=0,checks=0;mock::Host* active=nullptr;int origin_x=0,origin_y=0;
void check(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
OSErr world_read(ChannelReadPort port,const PSScaling* scaling,const VRect* area,const PixelMemoryDesc* memory,VRect* wrote){
 auto next=*scaling;const auto& data=*reinterpret_cast<mock::Port*>(port);
 if(active&&data.pixels==&active->pixels[0]){next.sourceRect.left-=origin_x;next.sourceRect.right-=origin_x;next.sourceRect.top-=origin_y;next.sourceRect.bottom-=origin_y;}
 return mock::read(port,&next,area,memory,wrote);
}
void run(Entry entry,int depth,bool modern,int dx,int dy,bool selected,bool source_mode=false,bool locked=false,bool identity=false){
 mock::Host host(129,97,depth);active=&host;origin_x=dx;origin_y=dy;host.ports.readPixelsProc=world_read;
 const VRect layer{dy,dx,dy+97,dx+129},canvas{0,0,80,96},extent=psui::union_bounds(canvas,layer);host.doc.bounds=canvas;
 // Real Photoshop reports Channel Ports in document space, but rebases the
 // FilterRecord image to the union's top-left. A selection resets floatCoord;
 // it does not remove that coordinate translation.
 for(int k=0;k<4;k++)host.channels[0][k].bounds=extent;
 auto area=extent;if(selected){area=canvas;host.f.filterCase=filterCaseEditableTransparencyWithSelection;}
 const auto document_area=area;
 area={area.top-extent.top,area.left-extent.left,area.bottom-extent.top,area.right-extent.left};
 if(locked)host.f.filterCase=selected?filterCaseProtectedTransparencyWithSelection:filterCaseProtectedTransparencyNoSelection;
 if(source_mode){struct Saved{unsigned long magic=0x5A434531;cell::Parameters p;};Saved saved;saved.p.mode=cell::Mode::crystallize;host.f.parameters=mock::allocate(sizeof(saved));std::memcpy(mock::lock(host.f.parameters,true),&saved,sizeof(saved));}
 host.big.filterRect32=area;host.f.filterRect={int16(area.top),int16(area.left),int16(area.bottom),int16(area.right)};
 host.big.imageSize32={extent.bottom-extent.top,extent.right-extent.left};host.f.imageSize={int16(host.big.imageSize32.v),int16(host.big.imageSize32.h)};
 host.big.floatCoord32=selected?VPoint{}:VPoint{extent.top,extent.left};if(!modern)host.f.bigDocumentData=nullptr;
 const auto translated=psui::filter_document_bounds(&host.f);
 check(std::memcmp(&translated,&document_area,sizeof(VRect))==0,"filter/document coordinate conversion disagrees with native host");
 const auto expected=psui::union_bounds(canvas,layer),bounds=psui::target_read_bounds(&host.f);
 check(bounds.left==expected.left&&bounds.top==expected.top&&bounds.right==expected.right&&bounds.bottom==expected.bottom,"read scope omits hidden raster pixels");
 intptr_t state=0;auto call=[&](int selector){int16 error=999;entry(int16(selector),&host.f,&state,&error);if(error){std::cerr<<" selector="<<selector<<" error="<<error;throw std::runtime_error("DLL lifecycle failed");}};
 call(filterSelectorPrepare);call(filterSelectorStart);
 const int aw=area.right-area.left,ah=area.bottom-area.top,bytes=depth/8,stride=aw*4*bytes+16,guard=32;
 std::vector<std::uint8_t> output(std::size_t(stride)*ah+guard*2,0xCD),input;int tiles=0,planes=0;
 auto rectangle=[&](bool source){return modern?(source?host.big.inRect32:host.big.outRect32):source?VRect{host.f.inRect.top,host.f.inRect.left,host.f.inRect.bottom,host.f.inRect.right}:VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};};
 for(;;){const auto r=rectangle(false);if(!psui::has_pixels(r))break;check(++tiles<12,"unbounded output requests");check(r.left>=area.left&&r.top>=area.top&&r.right<=area.right&&r.bottom<=area.bottom,"output escaped filter scope");
  const auto ir=rectangle(true);if(psui::has_pixels(ir)){
   const int iw=ir.right-ir.left,ih=ir.bottom-ir.top;input.assign(std::size_t(iw)*ih*3*bytes,0);
   for(int y=0;y<ih;y++)for(int x=0;x<iw;x++)for(int k=0;k<3;k++){
    const int xx=ir.left+extent.left+x-dx,yy=ir.top+extent.top+y-dy;float value=0;if(xx>=0&&yy>=0&&xx<129&&yy<97)value=host.pixels[0][(yy*129+xx)*4+k];auto* p=input.data()+((y*iw+x)*3+k)*bytes;
    if(bytes==1)*p=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));else if(bytes==2){auto n=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(p,&n,2);}else std::memcpy(p,&value,4);
   }
   host.f.inData=input.data();host.f.inRowBytes=iw*3*bytes;host.f.inColumnBytes=3*bytes;host.f.inPlaneBytes=bytes;
  }
  planes=host.f.outHiPlane+1;host.f.outData=output.data()+guard+std::size_t(r.top-area.top)*stride+(r.left-area.left)*planes*bytes;
  host.f.outRowBytes=stride;host.f.outColumnBytes=planes*bytes;host.f.outPlaneBytes=bytes;call(filterSelectorContinue);
 }
 check(tiles>0,"no output tiles");call(filterSelectorFinish);check(state==0,"session leaked");
 if(identity)for(int y=0;y<ah;y++)for(int x=0;x<aw;x++)for(int k=0;k<planes;k++){
  const int xx=document_area.left+x-dx,yy=document_area.top+y-dy;
  float expected=0;if(xx>=0&&yy>=0&&xx<129&&yy<97)expected=host.pixels[0][(yy*129+xx)*4+k];
  const auto* p=output.data()+guard+std::size_t(y)*stride+(x*planes+k)*bytes;
  bool same=false;if(bytes==1)same=*p==std::uint8_t(std::lround(std::clamp(expected,0.f,1.f)*255));
  else if(bytes==2){std::uint16_t value;std::memcpy(&value,p,2);same=value==std::uint16_t(std::lround(std::clamp(expected,0.f,1.f)*32768));}
  else{float value;std::memcpy(&value,p,4);same=value==expected;}
  check(same,"identity output moved, clipped or lost hidden source pixels");
 }
 for(int i=0;i<guard;i++)check(output[i]==0xCD&&output[output.size()-1-i]==0xCD,"buffer guard overwritten");
 for(int y=0;y<ah;y++)for(int i=aw*planes*bytes;i<stride;i++)check(output[guard+std::size_t(y)*stride+i]==0xCD,"row padding overwritten");
 ++cases;active=nullptr;
}
int wmain(int argc,wchar_t** argv){try{
 if(argc!=3)return 2;const std::filesystem::path root=argv[1];std::ifstream list{std::filesystem::path(argv[2])};std::string path;
 while(std::getline(list,path)){if(path.empty())continue;const auto binary=root/std::filesystem::path(path);auto dll=LoadLibraryW(binary.c_str());check(bool(dll),"candidate DLL failed to load");auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(bool(entry),"entry missing");const auto before=cases;
  try{for(int depth:{8,16,32}){if(path.find("ZaYuTint.8bf")!=std::string::npos&&depth!=8)continue;for(bool modern:{false,true})for(auto offset:{std::pair{-48,-24},std::pair{48,24},std::pair{-24,24},std::pair{24,-24}})for(bool selection:{false,true}){run(entry,depth,modern,offset.first,offset.second,selection,false,false,path.find("ZaYuOpticsCompensation.8bf")!=std::string::npos);if(path.find("ZaYuCellPattern.8bf")!=std::string::npos){run(entry,depth,modern,offset.first,offset.second,selection,true);run(entry,depth,modern,offset.first,offset.second,selection,false,true);}}}}
  catch(...){std::cerr<<" in "<<path<<'\n';FreeLibrary(dll);throw;}
  FreeLibrary(dll);std::cout<<"PASS "<<path<<": "<<cases-before<<" cases\n"<<std::flush;
 }
 std::cout<<"PASS: "<<cases<<" actual-DLL cases; "<<checks<<" bounds/guard checks; RGB8/16/32, four canvas edges, selections and old/new coordinates\n";
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
