#pragma once
#include "PIFilter.h"
#include "PIActions.h"
#include "ASZStringSuite.h"
#include "SPBasic.h"
#include <cstdint>
#include "psui/host_layers.hpp"
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

namespace psfilter::parameters {
constexpr DescriptorKeyID id(const char* s){return (std::uint32_t(s[0])<<24)|(std::uint32_t(s[1])<<16)|(std::uint32_t(s[2])<<8)|std::uint32_t(s[3]);}
// Old suite versions are intentional: the shared path must run in PS2018.
struct Suite {
 SPBasicSuite* basic=nullptr;const PSActionDescriptorProcs2* d=nullptr;const ASZStringSuite2* z=nullptr;PIActionDescriptor desc=nullptr;
 explicit Suite(FilterRecord* f,bool unicode=false):basic(f?f->sSPBasic:nullptr){
  if(!basic||!basic->AcquireSuite||!basic->ReleaseSuite){basic=nullptr;return;}
  if(basic->AcquireSuite(kPSActionDescriptorSuite,kPSActionDescriptorSuiteVersion2,reinterpret_cast<const void**>(&d)))d=nullptr;
  if(unicode&&basic->AcquireSuite(kASZStringSuite,kASZStringSuiteVersion2,reinterpret_cast<const void**>(&z)))z=nullptr;
 }
 Suite(const Suite&)=delete;Suite& operator=(const Suite&)=delete;
 ~Suite(){if(d){if(desc)d->Free(desc);basic->ReleaseSuite(kPSActionDescriptorSuite,kPSActionDescriptorSuiteVersion2);}if(z)basic->ReleaseSuite(kASZStringSuite,kASZStringSuiteVersion2);}
};
struct TextSuites:Suite{explicit TextSuites(FilterRecord* f):Suite(f,true){}};
struct Object {
 const PSActionDescriptorProcs2* d;PIActionDescriptor value=nullptr;
 explicit Object(const PSActionDescriptorProcs2* p):d(p){}
 ~Object(){if(value&&d)d->Free(value);}
};
struct ZText {
 const ASZStringSuite2* z;ASZString value=nullptr;
 explicit ZText(const ASZStringSuite2* p):z(p){}
 ~ZText(){if(value&&z)z->Release(value);}
};
inline bool text(const Suite& q,PIActionDescriptor object,DescriptorKeyID key,std::string& value){
 if(!q.z)return false;ZText z(q.z);if(q.d->GetZString(object,key,&z.value))return false;
 const auto n=q.z->LengthAsUTF8String(z.value);if(!n||n>8193)return false;
 std::vector<unsigned char> bytes(n,0);if(q.z->AsUTF8String(z.value,bytes.data(),n,true))return false;
 if(bytes.back()!=0)return false;value.assign(reinterpret_cast<const char*>(bytes.data()),n-1);return value.find('\0')==std::string::npos;
}
inline OSErr put_text(const Suite& q,PIActionDescriptor object,DescriptorKeyID key,const char* value){
 if(!q.z||!value||std::strlen(value)>8192)return filterBadParameters;ZText z(q.z);
 if(q.z->MakeFromUTF8String(reinterpret_cast<const unsigned char*>(value),&z.value))return filterBadParameters;
 return q.d->PutZString(object,key,z.value);
}
// Publish a fully built descriptor only after every field and handle succeeds.
inline OSErr commit(FilterRecord* f,const Suite& q,PIActionDescriptor object){
 auto* dp=f?f->descriptorParameters:nullptr;if(!dp)return noErr;
 const auto dispose=f->handleProcs?f->handleProcs->disposeProc:nullptr;
 if(dp->descriptor&&!dispose)return filterBadParameters;
 PIDescriptorHandle next=nullptr;const auto error=q.d->AsHandle(object,&next);
 if(error||!next){if(next&&dispose)dispose(reinterpret_cast<Handle>(next));return error?error:filterBadParameters;}
 auto previous=dp->descriptor;dp->descriptor=next;dp->recordInfo=plugInDialogOptional;
 if(previous)dispose(reinterpret_cast<Handle>(previous));return noErr;
}
template<class T>struct Schema{static constexpr bool available=false;};
struct Reader {
 const Suite& q;const std::vector<psui::HostLayer>& layers;OSErr error=noErr;
 bool has(const char* key){Boolean present=false;if(!error)error=q.d->HasKey(q.desc,id(key),&present);return !error&&present;}
 void number(const char* key,double& value){if(has(key)){double next=0;if((error=q.d->GetFloat(q.desc,id(key),&next)))return;if(!std::isfinite(next)){error=filterBadParameters;return;}value=next;}}
 template<class T>void integer(const char* key,T& value){if(has(key)){int32 next=0;if((error=q.d->GetInteger(q.desc,id(key),&next)))return;value=static_cast<T>(next);}}
 void flag(const char* key,bool& value){if(has(key)){Boolean next=false;if((error=q.d->GetBoolean(q.desc,id(key),&next)))return;value=next!=0;}}
 void layer(const char* key,std::uint64_t& value,std::uint32_t* transient=nullptr){
  if(!has(key))return;std::string name;if(!text(q,q.desc,id(key),name)){error=filterBadParameters;return;}
  value=name.empty()?0:psui::layer_name_key(name);if(transient){*transient=0xffffffffu;for(const auto& layer:layers)if(layer.name==name){*transient=layer.id;break;}}
 }
};
struct Writer {
 const Suite& q;const std::vector<psui::HostLayer>& layers;OSErr error=noErr;
 void number(const char* key,double value){if(!error){if(!std::isfinite(value))error=filterBadParameters;else error=q.d->PutFloat(q.desc,id(key),value);}}
 template<class T>void integer(const char* key,T value){if(!error){const auto bits=static_cast<std::uint32_t>(value);error=q.d->PutInteger(q.desc,id(key),std::bit_cast<int32>(bits));}}
 void flag(const char* key,bool value){if(!error)error=q.d->PutBoolean(q.desc,id(key),value);}
 void layer(const char* key,std::uint64_t value,const std::uint32_t* =nullptr){
  if(error)return;std::string name;
  if(value){for(const auto& layer:layers)if(layer.name_key==value){if(!name.empty()){error=filterBadParameters;return;}name=layer.name;}if(name.empty()){error=filterBadParameters;return;}}
  error=put_text(q,q.desc,id(key),name.c_str());
 }
};
template<class Effect>OSErr read(FilterRecord* f,const std::vector<psui::HostLayer>& layers,typename Effect::Parameters& values,bool& interactive){
 using S=Schema<typename Effect::Parameters>;
 if constexpr(!S::available)return noErr;
 else{
  auto* dp=f?f->descriptorParameters:nullptr;if(!dp||!dp->descriptor)return noErr;
  Suite q(f,S::unicode);if(!q.d||(S::unicode&&!q.z)||q.d->HandleToDescriptor(dp->descriptor,&q.desc)||!q.desc)return filterBadParameters;
  Boolean present=false;if(q.d->HasKey(q.desc,id("ZpSv"),&present))return filterBadParameters;
  if(!present)return noErr; // Existing unscripted PSD: preserve the host parameter block.
  int32 version=0,identity=0;if(q.d->GetInteger(q.desc,id("ZpSv"),&version)||version!=1||q.d->GetInteger(q.desc,id("ZpId"),&identity)||static_cast<std::uint32_t>(identity)!=Effect::magic)return filterBadParameters;
  auto next=values;Reader archive{q,layers};S::visit(archive,next);
  if(archive.error||!Effect::valid(next))return archive.error?archive.error:filterBadParameters;
  if constexpr(requires{next.kind;})if(next.kind!=values.kind)return filterBadParameters;
  values=next;interactive=dp->playInfo==plugInDialogDisplay;return noErr;
 }
}
template<class Effect>OSErr write(FilterRecord* f,const std::vector<psui::HostLayer>& layers,const typename Effect::Parameters& values){
 using S=Schema<typename Effect::Parameters>;
 if constexpr(!S::available)return noErr;
 else{
  if(!f||!f->descriptorParameters)return noErr;if(!Effect::valid(values))return filterBadParameters;
  Suite q(f,S::unicode);if(!q.d||(S::unicode&&!q.z)||q.d->Make(&q.desc)||!q.desc)return filterBadParameters;
  Writer archive{q,layers};archive.integer("ZpSv",1);archive.integer("ZpId",Effect::magic);S::visit(archive,values);
  return archive.error?archive.error:commit(f,q,q.desc);
 }
}
} // namespace psfilter::parameters
