#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "flare_settings.hpp"
#include "../../energy/tests/descriptor_mock.hpp"
#include "../src/preset_files.hpp"
#include <iostream>
namespace fake {
OSErr put_data(PIActionDescriptor p,DescriptorKeyID k,int32 n,void* bytes){auto* b=static_cast<std::uint8_t*>(bytes);return put(p,k,std::vector<std::uint8_t>(b,b+n));}
OSErr data_length(PIActionDescriptor p,DescriptorKeyID k,int32* n){std::vector<std::uint8_t> bytes;auto e=get(p,k,&bytes);if(!e)*n=int32(bytes.size());return e;}
OSErr get_data(PIActionDescriptor p,DescriptorKeyID k,void* bytes){std::vector<std::uint8_t> value;auto e=get(p,k,&value);if(!e)std::memcpy(bytes,value.data(),value.size());return e;}
}
int main(int argc,char** argv){try{
 using namespace flare;using namespace fake;using psfilter::parameters::id;init();d.PutData=put_data;d.GetDataLength=data_length;d.GetData=get_data;
 SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;HandleProcs handles{};handles.disposeProc=dispose;PIDescriptorParameters dp{};FilterRecord host{};host.sSPBasic=&basic;host.handleProcs=&handles;host.descriptorParameters=&dp;dp.playInfo=plugInDialogDontDisplay;
 auto p=defaults(512,320);auto design=design_from_legacy(p);design.elements[0].name="鎏金_热点";design.elements[0].custom_name=true;design.elements.back().x=12;std::vector<psui::HostLayer> layers(1);layers[0].name="鎏金_遮挡";layers[0].name_key=psui::layer_name_key(layers[0].name);p.map_key=layers[0].name_key;p.occlusion=1;
 require(write_settings(&host,layers,p,design)==noErr,"complete descriptor write");const auto write_steps=step;clean(1);const auto expected=reinterpret_cast<Saved*>(dp.descriptor)->data;
 auto restored=defaults(512,320);Design copy;bool interactive=true;step=0;require(read_settings(&host,layers,restored,copy,interactive)==noErr&&restored==p&&copy==design&&!interactive,"complete native+editor+Unicode round trip");const auto read_steps=step;clean(1);
 for(int n=1;n<=write_steps;n++){step=0;fail_at=n;auto old=dp.descriptor;require(write_settings(&host,layers,p,design)!=noErr,"every failed write propagated");require(old==dp.descriptor&&reinterpret_cast<Saved*>(old)->data==expected,"failed write does not replace old descriptor");clean(1);}
 for(int n=1;n<=read_steps;n++){step=0;fail_at=n;restored=p;copy=design;interactive=true;require(read_settings(&host,layers,restored,copy,interactive)!=noErr,"every failed read propagated");require(restored==p&&copy==design&&interactive,"failed read is atomic");clean(1);}
 fail_at=0;auto& values=reinterpret_cast<Saved*>(dp.descriptor)->data;values.erase(id("ZfEd"));copy=design;interactive=true;require(read_settings(&host,layers,restored,copy,interactive)==noErr&&!copy.enabled&&copy.elements.empty(),"0.1 descriptor loads classic path");values=expected;auto blob=std::get<std::vector<std::uint8_t>>(values[id("ZfEd")]);blob[4]=99;values[id("ZfEd")]=blob;copy=design;require(read_settings(&host,layers,restored,copy,interactive)!=noErr&&copy==design,"future editor version rejected atomically");dispose(reinterpret_cast<Handle>(dp.descriptor));dp.descriptor=nullptr;clean(0);
 if(argc>1){std::filesystem::path folder=argv[1];files::test_directory=folder;FlarePreset preset{p,design};auto saved=files::save_new("鎏金_镜头",preset);require(!saved.empty(),"Unicode user preset saved");FlarePreset loaded;require(files::read(saved,loaded)&&loaded.design==design,"Unicode file reloaded");require(!files::write(saved,preset),"existing file not overwritten");auto second=files::save_new("鎏金_镜头",preset);require(!second.empty()&&second!=saved,"duplicate preset gets another name");require(files::list().size()>=2,"library scans presets");auto invalid=preset;invalid.design.elements[0].size=-1;require(!files::write(saved,invalid,true)&&files::read(saved,loaded)&&loaded.design==design,"failed export preserves prior file");std::cout<<"Preset library test files: "<<files::list().size()<<'\n';}
 if(argc>1){auto blocked=std::filesystem::path(argv[1])/L"blocked-parent";{std::ofstream marker(blocked);marker<<"owned unavailable preset folder";}files::test_directory=blocked/L"Presets";require(files::save_new("Unavailable",FlarePreset{p,design}).empty(),"unavailable preset folder fails without throwing");files::test_directory=argv[1];}
 std::cout<<"PASS: "<<checks<<" settings assertions; blob+scalar ownership/failure atomicity; old descriptors; Unicode preset files; safe duplicate saves\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
