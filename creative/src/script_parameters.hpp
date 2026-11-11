#pragma once
#include "creative_host.hpp"
#include "PIActions.h"
#include "SPBasic.h"
namespace creative::script {
constexpr DescriptorKeyID key(char a,char b,char c,char d){return (std::uint32_t(a)<<24)|(std::uint32_t(b)<<16)|(std::uint32_t(c)<<8)|std::uint32_t(d);}
struct Number {const char* id;double Parameters::*member;};
constexpr Number numbers[]={{"ZcCX",&Parameters::cx},{"ZcCY",&Parameters::cy},{"ZcAm",&Parameters::amount},{"ZcSz",&Parameters::size},{"ZcCo",&Parameters::complexity},{"ZcEv",&Parameters::evolution},{"ZcCy",&Parameters::cycles},{"ZcSe",&Parameters::seed},{"ZcHX",&Parameters::horizontal},{"ZcVY",&Parameters::vertical},{"ZcMx",&Parameters::mix},{"ZcDr",&Parameters::direction},{"ZcWi",&Parameters::width},{"ZcIn",&Parameters::intensity},{"ZcEI",&Parameters::edge_intensity},{"ZcTh",&Parameters::thickness},{"ZcRo",&Parameters::rotation}};
struct Integer {const char* id;int Parameters::*member;};
constexpr Integer integers[]={{"ZcTy",&Parameters::type},{"ZcPi",&Parameters::pinning},{"ZcEd",&Parameters::edge},{"ZcSh",&Parameters::shape},{"ZcRe",&Parameters::reception},{"ZcMi",&Parameters::mirroring},{"ZcQu",&Parameters::quality}};
inline DescriptorKeyID id(const char* s){return key(s[0],s[1],s[2],s[3]);}
struct Suite {SPBasicSuite* basic;const PSActionDescriptorProcs* d=nullptr;PIActionDescriptor desc=nullptr;explicit Suite(FilterRecord* f):basic(f->sSPBasic){if(basic&&basic->AcquireSuite(kPSActionDescriptorSuite,kPSActionDescriptorSuiteVersion,reinterpret_cast<const void**>(&d)))d=nullptr;}~Suite(){if(d){if(desc)d->Free(desc);basic->ReleaseSuite(kPSActionDescriptorSuite,kPSActionDescriptorSuiteVersion);}}};
inline OSErr read(FilterRecord* f,Parameters& p,bool& interactive){
 auto* dp=f->descriptorParameters;if(!dp||!dp->descriptor)return noErr;Suite q(f);if(!q.d||q.d->HandleToDescriptor(dp->descriptor,&q.desc))return filterBadParameters;
 Boolean present=false;if(q.d->HasKey(q.desc,id("ZcSv"),&present))return filterBadParameters;if(!present)return noErr;
 auto next=p;int32 version=0,kind=0,cycle=0,floating=0;if(q.d->GetInteger(q.desc,id("ZcSv"),&version)||version!=1||q.d->GetInteger(q.desc,id("ZcKd"),&kind)||kind!=int(effect_kind))return filterBadParameters;
 for(const auto& n:numbers)if(q.d->GetFloat(q.desc,id(n.id),&(next.*n.member)))return filterBadParameters;
 for(const auto& n:integers)if(q.d->GetInteger(q.desc,id(n.id),&(next.*n.member)))return filterBadParameters;
 for(int i=0;i<3;i++)if(q.d->GetFloat(q.desc,key('Z','c','L',"RGB"[i]),&next.color[i]))return filterBadParameters;
 if(q.d->GetInteger(q.desc,id("ZcCE"),&cycle)||q.d->GetInteger(q.desc,id("ZcFC"),&floating)||cycle<0||cycle>1||floating<0||floating>1)return filterBadParameters;
 next.cycle=cycle;next.floating=floating;next.kind=effect_kind;if(!valid(next))return filterBadParameters;p=next;interactive=dp->playInfo==plugInDialogDisplay;return noErr;
}
inline OSErr write(FilterRecord* f,const Parameters& p){
 auto* dp=f->descriptorParameters;if(!dp)return noErr;Suite q(f);if(!q.d||q.d->Make(&q.desc))return filterBadParameters;OSErr error=noErr;
 auto integer=[&](DescriptorKeyID k,int v){if(!error)error=q.d->PutInteger(q.desc,k,v);};integer(id("ZcSv"),1);integer(id("ZcKd"),int(p.kind));
 for(const auto& n:numbers)if(!error)error=q.d->PutFloat(q.desc,id(n.id),p.*n.member);
 for(const auto& n:integers)integer(id(n.id),p.*n.member);for(int i=0;i<3;i++)if(!error)error=q.d->PutFloat(q.desc,key('Z','c','L',"RGB"[i]),p.color[i]);integer(id("ZcCE"),p.cycle);integer(id("ZcFC"),p.floating);
 if(error)return error;PIDescriptorHandle handle=nullptr;if((error=q.d->AsHandle(q.desc,&handle)))return error;if(dp->descriptor&&f->handleProcs&&f->handleProcs->disposeProc)f->handleProcs->disposeProc(reinterpret_cast<Handle>(dp->descriptor));dp->descriptor=handle;dp->recordInfo=plugInDialogOptional;return noErr;
}
}
