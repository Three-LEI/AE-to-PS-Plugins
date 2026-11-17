#pragma once
#include "flare_host.hpp"
#include "flare_design.hpp"
#include "parameter_schema.hpp"
namespace flare {
struct SettingsEffect {using Parameters=flare::Parameters;static constexpr auto magic=flare::magic;static bool valid(const Parameters& p){return flare::valid(p);}};
inline OSErr read_settings(FilterRecord* f,const std::vector<psui::HostLayer>& layers,Parameters& p,Design& d,bool& interactive){
 auto next=p;auto mode=interactive;auto error=psfilter::parameters::read<SettingsEffect>(f,layers,next,mode);if(error)return error;
 Design design;auto* dp=f?f->descriptorParameters:nullptr;
 if(dp&&dp->descriptor){using namespace psfilter::parameters;Suite q(f);if(!q.d||q.d->HandleToDescriptor(dp->descriptor,&q.desc)||!q.desc)return filterBadParameters;Boolean version=false,has=false;
  if(q.d->HasKey(q.desc,id("ZpSv"),&version)||q.d->HasKey(q.desc,id("ZfEd"),&has))return filterBadParameters;
  if(has){if(!version)return filterBadParameters;int32 size=0;if(q.d->GetDataLength(q.desc,id("ZfEd"),&size)||size<16||size>int32(max_design_bytes))return filterBadParameters;std::vector<std::uint8_t> bytes(size);if(q.d->GetData(q.desc,id("ZfEd"),bytes.data())||!decode_design(bytes,design))return filterBadParameters;}
 }
 p=next;d=std::move(design);interactive=mode;return noErr;
}
inline OSErr write_settings(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const Parameters& p,const Design& d){
 if(!f||!f->descriptorParameters)return noErr;if(!valid(p)||!valid_design(d))return filterBadParameters;
 using namespace psfilter::parameters;Suite q(f,true);if(!q.d||!q.z||q.d->Make(&q.desc)||!q.desc)return filterBadParameters;
 Writer a{q,layers};a.integer("ZpSv",1);a.integer("ZpId",magic);Schema<Parameters>::visit(a,p);if(a.error)return a.error;
 auto data=encode_design(d);if(data.empty()||q.d->PutData(q.desc,id("ZfEd"),int32(data.size()),data.data()))return filterBadParameters;
 return commit(f,q,q.desc);
}
}
