#pragma once
#include "roughen_host.hpp"
#include "PIActions.h"
#include "SPBasic.h"
#include "../../native/common/descriptor_parameters.hpp"
namespace roughen::script {
constexpr DescriptorKeyID key(char a,char b,char c,char d){return(std::uint32_t(a)<<24)|(std::uint32_t(b)<<16)|(std::uint32_t(c)<<8)|std::uint32_t(d);}
inline DescriptorKeyID id(const char* s){return key(s[0],s[1],s[2],s[3]);}
struct Number {const char* id;double Parameters::*member;};
constexpr Number numbers[]={{"ZrBo",&Parameters::border},{"ZrSh",&Parameters::sharpness},{"ZrIn",&Parameters::influence},{"ZrSc",&Parameters::scale},{"ZrSt",&Parameters::stretch},{"ZrOX",&Parameters::offset_x},{"ZrOY",&Parameters::offset_y},{"ZrCo",&Parameters::complexity},{"ZrEv",&Parameters::evolution},{"ZrCt",&Parameters::cycles},{"ZrSe",&Parameters::seed},{"ZrMx",&Parameters::mix}};
using Suite=psfilter::parameters::Suite;
inline OSErr read(FilterRecord* f,Parameters& p,bool& interactive){
 auto* dp=f->descriptorParameters;if(!dp||!dp->descriptor)return noErr;Suite q(f);if(!q.d||q.d->HandleToDescriptor(dp->descriptor,&q.desc))return filterBadParameters;
 Boolean present=false;if(q.d->HasKey(q.desc,id("ZrSv"),&present))return filterBadParameters;if(!present)return noErr;
 auto next=p;int32 version=0,type=0,cycle=0;if(q.d->GetInteger(q.desc,id("ZrSv"),&version)||version!=1||q.d->GetInteger(q.desc,id("ZrTy"),&type)||q.d->GetInteger(q.desc,id("ZrCE"),&cycle)||cycle<0||cycle>1)return filterBadParameters;
 for(const auto& n:numbers)if(q.d->GetFloat(q.desc,id(n.id),&(next.*n.member)))return filterBadParameters;
 for(int i=0;i<3;i++)if(q.d->GetFloat(q.desc,key('Z','r','L',"RGB"[i]),&next.color[i]))return filterBadParameters;
 next.type=type;next.cycle=cycle;if(!valid(next))return filterBadParameters;p=next;interactive=dp->playInfo==plugInDialogDisplay;return noErr;
}
inline OSErr write(FilterRecord* f,const Parameters& p){
 auto* dp=f->descriptorParameters;if(!dp)return noErr;Suite q(f);if(!q.d||q.d->Make(&q.desc))return filterBadParameters;OSErr error=noErr;
 auto integer=[&](DescriptorKeyID k,int v){if(!error)error=q.d->PutInteger(q.desc,k,v);};integer(id("ZrSv"),1);integer(id("ZrTy"),p.type);integer(id("ZrCE"),p.cycle);
 for(const auto& n:numbers)if(!error)error=q.d->PutFloat(q.desc,id(n.id),p.*n.member);
 for(int i=0;i<3;i++)if(!error)error=q.d->PutFloat(q.desc,key('Z','r','L',"RGB"[i]),p.color[i]);
 if(error)return error;return psfilter::parameters::commit(f,q,q.desc);
}
}
