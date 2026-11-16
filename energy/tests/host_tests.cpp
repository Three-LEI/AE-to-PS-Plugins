#include "../../displacement/tests/mock_host.hpp"
#include "energy_host.hpp"
#include "PIProperties.h"
#include "descriptor_mock.hpp"
#include "../src/filter.cpp"
#include <iostream>
#include <cstdio>
namespace energy {INT_PTR show_dialog(HINSTANCE,HWND,FilterRecord*,const Image&,Parameters&,Inputs&){return IDCANCEL;}}
namespace {
std::vector<std::uint8_t> resource;
OSErr property(unsigned32 signature,unsigned32 key,int32 index,intptr_t* value,Handle* h){
 fake::require(signature==0x3842494d&&index==0,"legacy indexed property signature");
 if(key==propNumberOfPaths){*value=1;return noErr;}
 if(key==propUnicodePathName){const wchar_t name[]=L"鎏金_钢笔路径";*h=mock::allocate(sizeof(name)-2);std::memcpy(mock::lock(*h,false),name,sizeof(name)-2);return noErr;}
 if(key==propPathContents){*h=mock::allocate(int32(resource.size()));std::memcpy(mock::lock(*h,false),resource.data(),resource.size());return noErr;}return paramErr;
}
OSErr put_data(PIActionDescriptor p,DescriptorKeyID k,int32 n,void* v){if(n<0)return paramErr;return fake::put(p,k,std::vector<std::uint8_t>(static_cast<std::uint8_t*>(v),static_cast<std::uint8_t*>(v)+n));}
OSErr get_length(PIActionDescriptor p,DescriptorKeyID k,int32* n){std::vector<std::uint8_t> b;auto e=fake::get(p,k,&b);*n=int32(b.size());return e;}
OSErr get_data(PIActionDescriptor p,DescriptorKeyID k,void* v){std::vector<std::uint8_t> b;auto e=fake::get(p,k,&b);if(!e)std::memcpy(v,b.data(),b.size());return e;}
}
int main(){try{
 using namespace energy;using fake::require;
 auto p=defaults(300,200);p.input=1;p.color={.2,.6,1};p.style=2;p.detail=4;p.seed=12345;p.evolution=78.5;p.fire_height=81;p.fire_spread=33;p.fire_direction=-75;p.tip_color={1,.1,.2};Inputs input{line_path(p),"鎏金_钢笔路径"};
 fake::init();fake::d.PutData=put_data;fake::d.GetDataLength=get_length;fake::d.GetData=get_data;SPBasicSuite basic{};basic.AcquireSuite=fake::acquire;basic.ReleaseSuite=fake::release;HandleProcs hp{};hp.disposeProc=fake::dispose;PIDescriptorParameters dp{};FilterRecord host{};host.sSPBasic=&basic;host.handleProcs=&hp;host.descriptorParameters=&dp;dp.playInfo=plugInDialogDontDisplay;
 require(write_settings(&host,p,input)==noErr,"write settings and geometry");int writes=fake::step;auto saved=dp.descriptor;auto expected=reinterpret_cast<fake::Saved*>(saved)->data;fake::clean(1);
 Parameters restored{};Inputs snapshot;bool interactive=true;fake::step=0;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==p&&snapshot.path==input.path&&snapshot.name==input.name&&!interactive,"all parameters, Unicode name and geometry roundtrip");int reads=fake::step;fake::clean(1);
 for(int i=1;i<=writes;i++){fake::step=0;fake::fail_at=i;require(write_settings(&host,p,input)!=noErr&&dp.descriptor==saved,"write failure retains original descriptor");fake::clean(1);}
 for(int i=1;i<=reads;i++){fake::step=0;fake::fail_at=i;auto values=defaults(32,24);auto before=values;Inputs old{line_path(values),"unchanged"};const auto geometry=old.path;bool show=true;require(read_settings(&host,values,old,show)!=noErr&&values==before&&old.path==geometry&&old.name=="unchanged"&&show,"read failure transactional");fake::clean(1);}
 fake::fail_at=0;auto& model=reinterpret_cast<fake::Saved*>(saved)->data;require(std::get<int32>(model[psfilter::parameters::id("ZpSv")])==2,"new materials use schema 2 so older Saber rejects silent downgrade");model[psfilter::parameters::id("ZePG")]=std::vector<std::uint8_t>{0,1,2};require(read_settings(&host,restored,snapshot,interactive)!=noErr,"reject broken snapshot");model=expected;
 model[psfilter::parameters::id("ZpSv")]=int32(1);require(read_settings(&host,restored,snapshot,interactive)!=noErr,"reject new material tagged as legacy format");
 for(unsigned key=0x5a653134;key<=0x5a653232;key++){char id[5]={char(key>>24),char(key>>16),char(key>>8),char(key),0};model.erase(psfilter::parameters::id(id));}
 auto old_expected=p;reset_extension(old_expected);restored=p;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==old_expected,"0.1 descriptor ignores last-used material and restores original defaults");
 model=expected;fake::dispose(reinterpret_cast<Handle>(dp.descriptor));dp.descriptor=nullptr;fake::clean(0);
 // 0.3 fields roundtrip independently and cannot be replayed as an old model.
 auto light=calibrated_defaults(300,200);light.input=1;light.halo_intensity=.73;light.halo_radius=7.25;light.glow_gamma=1.4;light.glow_bias=.8;light.glow_sizes={1.3,.62,.21,.07};light.glow_gains={1.7,.65,.31,.11};
 require(write_settings(&host,light,input)==noErr,"write layered model");auto light_saved=dp.descriptor;auto& light_data=reinterpret_cast<fake::Saved*>(light_saved)->data;auto light_expected=light_data;require(std::get<int32>(light_data[psfilter::parameters::id("ZpSv")])==3,"layered light uses schema 3");
 restored=defaults(20,20);require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==light,"all 48 fields roundtrip with four band records");
 light_data[psfilter::parameters::id("ZpSv")]=int32(2);require(read_settings(&host,restored,snapshot,interactive)!=noErr,"new model cannot be mislabeled as schema 2");
 light_data=light_expected;light_data.erase(psfilter::parameters::id("Ze23"));require(read_settings(&host,restored,snapshot,interactive)!=noErr,"schema 3 must identify its lighting model");
 light_data=expected;for(int key=0x23;key<=0x2f;key++){char id[5];std::snprintf(id,sizeof(id),"Ze%02X",key);light_data.erase(psfilter::parameters::id(id));}
 restored=light;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==p,"old descriptors override a new last-used model with classic defaults");
 fake::dispose(reinterpret_cast<Handle>(light_saved));dp.descriptor=nullptr;fake::clean(0);
 auto electric=material_preset(3,light);electric.glow_seed=987654;electric.glow_evolution=71;electric.glow_detail=5;electric.filament_count=4;electric.glow_noise_scale=53;electric.glow_width=9;electric.filament_strength=.63;
 require(write_settings(&host,electric,input)==noErr,"write independent electric fields");auto electric_saved=dp.descriptor;auto& electric_data=reinterpret_cast<fake::Saved*>(electric_saved)->data;auto electric_expected=electric_data;require(std::get<int32>(electric_data[psfilter::parameters::id("ZpSv")])==4,"electric material uses schema 4");
 restored=defaults(20,20);require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==electric,"all 57 fields including independent seeds roundtrip");
 electric_data[psfilter::parameters::id("ZpSv")]=int32(3);require(read_settings(&host,restored,snapshot,interactive)!=noErr,"new material cannot masquerade as 0.3");electric_data=electric_expected;electric_data.erase(psfilter::parameters::id("Ze14"));require(read_settings(&host,restored,snapshot,interactive)!=noErr,"schema 4 needs its own material identity");
 electric_data=light_expected;for(int key=0x30;key<=0x38;key++){char id[5];std::snprintf(id,sizeof(id),"Ze%02X",key);electric_data.erase(psfilter::parameters::id(id));}
 restored=electric;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==light,"0.3 descriptors discard ambient new-material settings");fake::dispose(reinterpret_cast<Handle>(electric_saved));dp.descriptor=nullptr;fake::clean(0);
 auto burning=material_preset(4,light);burning.burn_detail=4;burning.burn_seed=876543;burning.burn_width=8.2;burning.burn_turbulence=27;burning.burn_scale=61;burning.burn_density=.73;burning.burn_filaments=.55;burning.burn_wind=13;burning.burn_evolution=91;
 require(write_settings(&host,burning,input)==noErr,"write burning fields");auto burning_saved=dp.descriptor;auto& burning_data=reinterpret_cast<fake::Saved*>(burning_saved)->data;const auto burning_expected=burning_data;require(std::get<int32>(burning_data[psfilter::parameters::id("ZpSv")])==5,"burning uses schema 5");
 restored=defaults(20,20);require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==burning,"all 66 effect fields including burning roundtrip");burning_data[psfilter::parameters::id("ZpSv")]=int32(4);require(read_settings(&host,restored,snapshot,interactive)!=noErr,"new burning cannot masquerade as old electric");burning_data=burning_expected;burning_data.erase(psfilter::parameters::id("Ze14"));require(read_settings(&host,restored,snapshot,interactive)!=noErr,"schema 5 requires its own style identity");
 burning_data=electric_expected;for(int key=0x39;key<=0x41;key++){char id[5];std::snprintf(id,sizeof(id),"Ze%02X",key);burning_data.erase(psfilter::parameters::id(id));}restored=burning;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==electric,"0.4 descriptors discard ambient burning settings");fake::dispose(reinterpret_cast<Handle>(burning_saved));dp.descriptor=nullptr;fake::clean(0);
 auto common=unified_preset(3,unified_defaults(300,200));common.input=1;common.core_noise=0;common.glow_noise=2;common.color_mix=.73;
 require(write_settings(&host,common,input)==noErr,"write common controls");auto common_saved=dp.descriptor;auto& common_data=reinterpret_cast<fake::Saved*>(common_saved)->data;const auto common_expected=common_data;require(std::get<int32>(common_data[psfilter::parameters::id("ZpSv")])==6,"common pipeline uses schema 6");
 restored=burning;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==common,"all 70 controls and independent noise types roundtrip");common_data[psfilter::parameters::id("ZpSv")]=int32(5);require(read_settings(&host,restored,snapshot,interactive)!=noErr,"common pipeline cannot masquerade as old burning");common_data=common_expected;common_data.erase(psfilter::parameters::id("Ze42"));require(read_settings(&host,restored,snapshot,interactive)!=noErr,"schema 6 requires common engine identity");
 common_data=burning_expected;for(int key=0x42;key<=0x45;key++){char id[5];std::snprintf(id,sizeof(id),"Ze%02X",key);common_data.erase(psfilter::parameters::id(id));}restored=common;require(read_settings(&host,restored,snapshot,interactive)==noErr&&restored==burning&&restored.engine==0,"old descriptors never inherit the new engine");fake::dispose(reinterpret_cast<Handle>(common_saved));dp.descriptor=nullptr;fake::clean(0);
 // SDK property parser: geometry follows image dimensions, not selection bounds.
 resource.resize(26*4);resource[1]=6;resource[27]=3;resource[29]=2;
 for(int i=0;i<2;i++){auto* r=resource.data()+(i+2)*26;r[1]=5;for(int j=0;j<3;j++){auto y=std::uint32_t(0x00800000),x=std::uint32_t(i?0x00c00000:0x00400000);for(int k=0;k<4;k++){r[2+j*8+k]=std::uint8_t(y>>(24-k*8));r[6+j*8+k]=std::uint8_t(x>>(24-k*8));}}}
 mock::Host mh(300,200);PropertyProcs props{};props.getPropertyProc=property;mh.f.propertyProcs=&props;auto paths=host_paths(&mh.f);require(paths.size()==1&&paths[0].name==input.name,"Unicode legacy path enumeration");Inputs imported;std::string error;require(import_path(&mh.f,0,imported,error),"legacy path contents import");require(imported.path.curves[0].knots[0].anchor==energy::Point{75,100}&&imported.path.curves[0].knots[1].anchor==energy::Point{225,100},"document coordinate dimensions");auto unchanged=imported;require(!import_path(&mh.f,9,imported,error)&&imported.path==unchanged.path,"bad path index preserves snapshot");
 int cases=0;for(int depth:{8,16,32})for(bool selection:{false,true})for(bool flat:{false,true}){
  mock::Host h(529,129,depth);if(selection)h.big.filterRect32={10,17,119,518};if(flat){h.f.outTransparencyMask=0;h.f.filterCase=selection?filterCaseFlatImageWithSelection:filterCaseFlatImageNoSelection;}
  intptr_t data=0;int16 result=0;PluginMain(filterSelectorPrepare,&h.f,&data,&result);PluginMain(filterSelectorStart,&h.f,&data,&result);require(result==noErr&&data>1,"native adapter start");int tiles=0;
  const auto* session=reinterpret_cast<psfilter::CachedRuntime<Effect>::Session*>(data);const auto& expected_image=session->output;
  while(h.big.outRect32.bottom>h.big.outRect32.top){const auto r=h.big.outRect32;int width=r.right-r.left,height=r.bottom-r.top,planes=flat?3:4,bytes=depth/8;std::vector<std::uint8_t> output(std::size_t(width)*height*planes*bytes);h.f.outData=output.data();h.f.outRowBytes=width*planes*bytes;h.f.outColumnBytes=planes*bytes;h.f.outPlaneBytes=bytes;PluginMain(filterSelectorContinue,&h.f,&data,&result);require(result==noErr,"native tile output");
   for(int y=0;y<height;y++)for(int x=0;x<width;x++)for(int k=0;k<planes;k++){std::size_t a=(std::size_t(y)*width+x)*planes+k,b=(std::size_t(r.top+y)*529+r.left+x)*4;float value=expected_image.rgba[b+k]*(flat?expected_image.rgba[b+3]:1);if(depth==8)require(output[a]==std::uint8_t(std::lround(std::clamp(value,0.f,1.f)*255)),"RGB8 native encoding");else if(depth==16){std::uint16_t v;std::memcpy(&v,output.data()+a*2,2);require(v==std::uint16_t(std::lround(std::clamp(value,0.f,1.f)*32768)),"PS16 native encoding");}else{float v;std::memcpy(&v,output.data()+a*4,4);require(v==value,"float32 native encoding");}}
   tiles++;
  }PluginMain(filterSelectorFinish,&h.f,&data,&result);require(data==0&&tiles>=2,"native lifecycle and tile coverage");cases++;
 }
 mock::cancel=true;intptr_t data=0;int16 result=0;PluginMain(filterSelectorStart,&mh.f,&data,&result);require(result==userCanceledErr&&data==0,"host cancellation clean");mock::cancel=false;
 mock::Host legacy_host(96,64);legacy::SavedV1 legacy_block;legacy_block.values.start_x=10;legacy_block.values.end_x=86;legacy_block.values.start_y=legacy_block.values.end_y=32;legacy_host.f.parameters=mock::allocate(sizeof(legacy_block));std::memcpy(mock::lock(legacy_host.f.parameters,false),&legacy_block,sizeof(legacy_block));PluginMain(filterSelectorStart,&legacy_host.f,&data,&result);require(result==noErr&&data>1&&mock::size(legacy_host.f.parameters)==sizeof(psfilter::Runtime<Effect>::Saved),"real host parameter handle upgrades 0.1 block");PluginMain(filterSelectorFinish,&legacy_host.f,&data,&result);require(data==0&&result==noErr,"upgraded legacy session finalizes");
 std::cout<<"PASS "<<fake::checks<<" host checks; "<<cases<<" depth/selection/transparency cases; descriptor failure injection, saved geometry and legacy Unicode paths\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
