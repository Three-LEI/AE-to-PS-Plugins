#pragma once
#include "surface_host.hpp"
#include "PIActions.h"
#include "ASZStringSuite.h"
#include "SPBasic.h"
#include "../../native/common/descriptor_parameters.hpp"
#include <string>
#include <vector>
namespace surface::script {
constexpr DescriptorKeyID key(char a,char b,char c,char d){return (std::uint32_t(a)<<24)|(std::uint32_t(b)<<16)|(std::uint32_t(c)<<8)|std::uint32_t(d);}
constexpr auto version_key=key('Z','y','S','v'),kind_key=key('Z','y','K','d'),name_key=key('Z','y','N','m'),id_key=key('Z','y','I','d'),revision_key=key('Z','y','R','f');
struct Number {DescriptorKeyID key;double Parameters::*member;};
constexpr Number numbers[]={
 {key('Z','y','S','f'),&Parameters::softness},{key('Z','y','C','t'),&Parameters::cut},{key('Z','y','H','t'),&Parameters::height},{key('Z','y','D','p'),&Parameters::displacement},
 {key('Z','y','I','n'),&Parameters::intensity},{key('Z','y','L','H'),&Parameters::light_height},{key('Z','y','L','X'),&Parameters::light_x},{key('Z','y','L','Y'),&Parameters::light_y},{key('Z','y','D','r'),&Parameters::direction},
 {key('Z','y','A','m'),&Parameters::ambient},{key('Z','y','D','f'),&Parameters::diffuse},{key('Z','y','S','p'),&Parameters::specular},{key('Z','y','R','o'),&Parameters::roughness},{key('Z','y','M','e'),&Parameters::metal}};
using Suites=psfilter::parameters::TextSuites;
using Desc=psfilter::parameters::Object;
using psfilter::parameters::text;
using psfilter::parameters::put_text;
// Channel Ports exports invocation-local sheet indexes, not Action Manager IDs.
// Native rendering resolves only a unique name. The companion refresh script
// validates persistent IDs outside the filter and synchronizes the saved name.
inline bool capture(FilterRecord*,const psui::HostLayer& selected,MapBinding& binding){
 if(binding.name!=selected.name)binding.layer_id=0;binding.name=selected.name;return true;
}
inline const psui::HostLayer* resolve(FilterRecord* f,const std::vector<psui::HostLayer>& layers,Parameters& p,MapBinding& binding){
 if(!p.map_key){binding.layer_id=0;binding.name.clear();return nullptr;}
 const psui::HostLayer* found=nullptr;for(const auto& layer:layers)if(layer.name_key==p.map_key){if(found)return nullptr;found=&layer;}
 if(found)capture(f,*found,binding);return found;
}
inline OSErr read(FilterRecord* f,Parameters& values,MapBinding& binding,bool& interactive){
 auto* parameters=f->descriptorParameters;if(!parameters||!parameters->descriptor)return noErr;
 Suites q(f);if(!q.d||!q.z)return filterBadParameters;Desc desc(q.d);if(q.d->HandleToDescriptor(parameters->descriptor,&desc.value))return filterBadParameters;
 Boolean present=false;if(q.d->HasKey(desc.value,version_key,&present))return filterBadParameters;if(!present)return noErr;
 Parameters next=values;MapBinding reference;int32 version=0,kind=0,property=0,type=0;
 if(q.d->GetInteger(desc.value,version_key,&version)||version!=1||q.d->GetInteger(desc.value,kind_key,&kind)||kind!=int(effect_kind)||q.d->GetInteger(desc.value,key('Z','y','P','r'),&property)||q.d->GetInteger(desc.value,key('Z','y','L','t'),&type))return filterBadParameters;
 next.kind=effect_kind;next.property=property;next.light_type=type;
 for(const auto& field:numbers)if(q.d->GetFloat(desc.value,field.key,&(next.*field.member)))return filterBadParameters;
 for(int i=0;i<3;++i)if(q.d->GetFloat(desc.value,key('Z','y','L',"RGB"[i]),&next.light_color[i]))return filterBadParameters;
 if(!text(q,desc.value,name_key,reference.name)||q.d->GetInteger(desc.value,id_key,&reference.layer_id)||q.d->GetInteger(desc.value,revision_key,&reference.revision)||reference.layer_id<0||reference.revision<0||reference.revision>=1000000000)return filterBadParameters;
 next.map_key=reference.name.empty()?0:psui::layer_name_key(reference.name);if(next.map_key==0&&reference.layer_id!=0)return filterBadParameters;if(!valid(next))return filterBadParameters;
 values=next;binding=std::move(reference);interactive=parameters->playInfo==plugInDialogDisplay;return noErr;
}
inline OSErr write(FilterRecord* f,const Parameters& values,const MapBinding& binding){
 auto* parameters=f->descriptorParameters;if(!parameters)return noErr;if(values.map_key&&binding.name.empty())return filterBadParameters;Suites q(f);if(!q.d||!q.z)return filterBadParameters;Desc desc(q.d);if(q.d->Make(&desc.value))return memFullErr;
 OSErr error=noErr;auto integer=[&](DescriptorKeyID k,int32 v){if(!error)error=q.d->PutInteger(desc.value,k,v);};
 integer(version_key,1);integer(kind_key,int32(values.kind));integer(key('Z','y','P','r'),values.property);integer(key('Z','y','L','t'),values.light_type);
 for(const auto& field:numbers)if(!error)error=q.d->PutFloat(desc.value,field.key,values.*field.member);
 for(int i=0;i<3;++i)if(!error)error=q.d->PutFloat(desc.value,key('Z','y','L',"RGB"[i]),values.light_color[i]);
 if(!error)error=put_text(q,desc.value,name_key,values.map_key?binding.name.c_str():"");integer(id_key,values.map_key?binding.layer_id:0);integer(revision_key,binding.revision);
 if(error)return error;return psfilter::parameters::commit(f,q,desc.value);
}
}
