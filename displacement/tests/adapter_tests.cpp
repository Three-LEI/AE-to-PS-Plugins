#include "mock_host.hpp"
#include <cstdlib>
#include <iostream>
void check(bool q,const char* what){if(!q){std::cerr<<"FAIL: "<<what<<'\n';std::exit(1);}}
using Entry=void(*)(int16,FilterRecord*,intptr_t*,int16*);
void verify(Entry entry,int depth,bool modern){
 mock::Host host(1036,142,depth);if(!modern)host.f.bigDocumentData=nullptr;
 ReadChannelDesc target_channels[3];mock::Port target_ports[3];
 if(modern){for(int c=0;c<3;++c){target_ports[c]=host.data[0][c];target_channels[c]=host.channels[0][c];target_channels[c].port=reinterpret_cast<PIChannelPort>(&target_ports[c]);target_channels[c].next=c<2?&target_channels[c+1]:nullptr;}host.doc.targetCompositeChannels=&target_channels[0];host.channels[0][0].target=true;}
 auto layers=psui::host_layers(&host.f);check(layers.size()==2&&layers[0].target&&!layers[1].target,"enumerate target and hidden map layer");
 check(layers[1].name=="采样图层（隐藏）"&&layers[1].id==22,"Unicode names and invocation sheet IDs");
 psui::LayerPixels proxy;check(!psui::host_read_layer(&host.f,layers[1],host.doc.bounds,32,proxy)&&proxy.width==32&&proxy.height<=32,"bounded channel-port proxy");
 intptr_t state=0;auto call=[&](int n){int16 r=0;entry(int16(n),&host.f,&state,&r);return r;};
 const VRect area{5,7,136,1032};host.big.filterRect32=area;host.f.filterRect={5,7,136,1032};
 check(call(filterSelectorPrepare)==noErr&&call(filterSelectorStart)==noErr,"start with a native map layer and no PSD file");
 check(state>1&&host.f.parameters&&host.f.autoMask,"session and host-managed selection mask");
 const int bytes=depth/8,stride=1036*4*bytes+16;
 std::vector<std::uint8_t> output(std::size_t(stride)*142,0xCD);
 int tiles=0;
 while(true){const auto r=modern?host.big.outRect32:VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};if(r.bottom<=r.top)break;
  check(++tiles<=9,"bounded tile loop");check(host.f.outHiPlane==3,"displaced editable alpha plane requested");
  check(r.left>=area.left&&r.top>=area.top&&r.right<=area.right&&r.bottom<=area.bottom,"output respects selection bounds");
  host.f.outData=output.data()+std::size_t(r.top)*stride+r.left*4*bytes;host.f.outRowBytes=stride;host.f.outColumnBytes=4*bytes;host.f.outPlaneBytes=bytes;
  check(call(filterSelectorContinue)==noErr,"render tile");
 }
 check(tiles==9,"nine irregular spatial tiles");
 for(int y=0;y<142;++y)for(int byte=0;byte<stride;++byte){
  const int x=byte/(4*bytes),component=(byte%(4*bytes))/bytes,sub=byte%bytes;
  const auto actual=output[std::size_t(y)*stride+byte];
  if(y<area.top||y>=area.bottom||x<area.left||x>=area.right){check(actual==0xCD,"selection exterior and padding untouched");continue;}
  float value=host.pixels[0][(std::size_t(y)*1036+x-5)*4+component];
  if(component<3&&host.pixels[0][(std::size_t(y)*1036+x-5)*4+3]==0)value=0;
  std::uint8_t expected[4]{};
  if(bytes==1)expected[0]=std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255));
  else if(bytes==2){auto q=std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768));std::memcpy(expected,&q,2);}
  else std::memcpy(expected,&value,4);
  check(actual==expected[sub],"native output matches independent five-pixel reference shift");
 }
 check(call(filterSelectorFinish)==noErr&&state==0,"finish releases session");
 host.layers[0].sheetID=22;host.layers[1].sheetID=11;host.doc.layersDescriptor=&host.layers[1];host.layers[1].next=&host.layers[0];host.layers[0].next=nullptr;
 check(call(filterSelectorStart)==noErr,"saved map name survives changed sheet index and layer order");
 const auto first=modern?host.big.outRect32:VRect{host.f.outRect.top,host.f.outRect.left,host.f.outRect.bottom,host.f.outRect.right};
 host.f.outData=output.data()+std::size_t(first.top)*stride+first.left*4*bytes;
 check(call(filterSelectorContinue)==noErr,"repeat renders after reordering");
 if(bytes==1)check(*static_cast<std::uint8_t*>(host.f.outData)==2,"repeat uses original white map, not reordered source");
 else if(bytes==2){std::uint16_t value;std::memcpy(&value,host.f.outData,2);check(value==std::lround(2./255*32768),"PS16 repeat map reference");}
 else{float value;std::memcpy(&value,host.f.outData,4);check(std::abs(value-(2+2.f/255))<1e-6,"HDR repeat map reference");}
 check(call(filterSelectorFinish)==noErr,"finish partial repeated render");
 check(call(filterSelectorStart)==noErr,"repeat rereads native map layer");mock::cancel=true;
 check(call(filterSelectorContinue)==userCanceledErr&&state==0,"cancel releases snapshots");mock::cancel=false;
 check(call(filterSelectorFinish)==noErr,"finish after cancel");
 mock::fail_read=true;check(call(filterSelectorStart)==-123&&state==0,"channel read error propagates cleanly");mock::fail_read=false;
 const auto original_name=host.layers[1].unicodeName;host.layers[1].unicodeName=reinterpret_cast<const uint16*>(L"Renamed map");
 check(call(filterSelectorStart)==errReportString&&state==0,"renamed/missing map requires explicit reselection");host.layers[1].unicodeName=original_name;
 host.layers[0].unicodeName=host.layers[1].unicodeName;check(call(filterSelectorStart)==errReportString&&state==0,"duplicate map names are not silently rebound");
 layers[1].rgb[0]->depth=1;psui::LayerPixels invalid;check(psui::host_read_layer(&host.f,layers[1],host.doc.bounds,0,invalid)==filterBadMode,"unsupported channel depth rejected");
}
void flat_image(Entry entry,int depth,int version){
 mock::Host host(64,32,depth);host.doc.maxVersion=version;host.doc.layersDescriptor=nullptr;host.doc.targetTransparency=nullptr;
 host.f.outTransparencyMask=0;host.f.filterCase=filterCaseFlatImageNoSelection;
 auto layers=psui::host_layers(&host.f);check(layers.size()==1&&layers[0].target,"flat image exposes current target as readable map without a layer list");
 intptr_t state=0;auto call=[&](int n){int16 result;entry(int16(n),&host.f,&state,&result);return result;};
 check(call(filterSelectorPrepare)==noErr&&call(filterSelectorStart)==noErr&&state>1,"first call on flat RGB image succeeds");
 const int bytes=depth/8;std::vector<std::uint8_t> output(64*32*3*bytes);
 host.f.outData=output.data();host.f.outRowBytes=64*3*bytes;host.f.outColumnBytes=3*bytes;host.f.outPlaneBytes=bytes;
 check(host.f.outHiPlane==2&&call(filterSelectorContinue)==noErr,"flat output retains RGB plane layout");
 check(call(filterSelectorFinish)==noErr&&state==0,"flat call session cleaned");
 host.doc.targetCompositeChannels=nullptr;check(psui::host_layers(&host.f).empty(),"absent target channels are not fabricated");
 check(call(filterSelectorStart)==errReportString&&state==0,"actual missing RGB source still fails cleanly");
}
int main(){
 wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);std::wstring file(path);file.resize(file.find_last_of(L"\\/")+1);file+=L"ZaYuDisplacementMap.8bf";
 auto dll=LoadLibraryW(file.c_str());check(dll!=nullptr,"Windows loads standalone plugin");auto entry=reinterpret_cast<Entry>(GetProcAddress(dll,"PluginMain"));check(entry!=nullptr,"PluginMain export");
 for(int depth:{8,16,32})for(bool modern:{false,true})verify(entry,depth,modern);
 for(int depth:{8,16,32})for(int version:{0,6})flat_image(entry,depth,version);
 FreeLibrary(dll);std::cout<<"PASS: real DLL; distinct target ports; Unicode/hidden layer; bounded proxy; RGB8/PS16/HDR32 + alpha; 9 tiles; reordered map reference; missing/duplicate names; selection guards; cancel/read errors; 16/32-bit coordinates.\n";
}
