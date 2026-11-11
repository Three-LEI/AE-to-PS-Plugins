#pragma once
#include "PIActions.h"
#include "psui/photoshop_host.hpp"
#include <cstring>
#include <stdexcept>
#include <string>
namespace localeqa {
inline int acquired=0,released=0,live_refs=0,live_desc=0,stage=0,fail_stage=0,fail_acquire=0;
inline std::string locale;
inline PSBasicActionControlProcsOld control{};
inline PSActionReferenceProcsOld reference{};
inline PSActionDescriptorProcs2 descriptor{};
inline PIActionReference ref(){return reinterpret_cast<PIActionReference>(0x100);}
inline PIActionDescriptor root(){return reinterpret_cast<PIActionDescriptor>(0x200);}
inline PIActionDescriptor prefs(){return reinterpret_cast<PIActionDescriptor>(0x300);}
inline void require(bool v){if(!v)throw std::runtime_error("locale suite contract");}
inline OSErr next(){return ++stage==fail_stage?paramErr:noErr;}
inline OSErr string_id(const char* key,DescriptorTypeID* value){
 require(!std::strcmp(key,"interfacePrefs")||!std::strcmp(key,"uiLanguageKey"));*value=!std::strcmp(key,"interfacePrefs")?101:102;return next();
}
inline OSErr make(PIActionReference* value){*value=ref();++live_refs;return next();}
inline OSErr free_ref(PIActionReference value){require(value==ref());--live_refs;return noErr;}
inline OSErr put_property(PIActionReference value,DescriptorClassID type,DescriptorKeyID key){require(value==ref()&&type==0x50727072&&key==101);return next();}
inline OSErr put_enum(PIActionReference value,DescriptorClassID type,DescriptorEnumTypeID form,DescriptorEnumID target){require(value==ref()&&type==0x63617070&&form==0x4f72646e&&target==0x54726774);return next();}
inline OSErr get(PIActionDescriptor* value,PIActionReference target){require(target==ref());*value=root();++live_desc;return next();}
inline OSErr free_desc(PIActionDescriptor value){require(value==root()||value==prefs());--live_desc;return noErr;}
inline OSErr get_object(PIActionDescriptor source,DescriptorKeyID key,DescriptorClassID* type,PIActionDescriptor* value){require(source==root()&&key==101);*type=101;*value=prefs();++live_desc;return next();}
inline OSErr get_length(PIActionDescriptor source,DescriptorKeyID key,uint32* length){require(source==prefs()&&key==102);*length=static_cast<uint32>(locale.size());return next();}
inline OSErr get_string(PIActionDescriptor source,DescriptorKeyID key,char* value,uint32 size){require(source==prefs()&&key==102&&size>locale.size());std::memcpy(value,locale.c_str(),locale.size()+1);return next();}
inline SPErr acquire(const char* name,int32 version,const void** value){
 ++acquired;if(acquired==fail_acquire){*value=nullptr;return -1;}
 if(!std::strcmp(name,kPSBasicActionControlSuite)){require(version==kPSBasicActionControlSuitePrevVersion);*value=&control;}
 else if(!std::strcmp(name,kPSActionReferenceSuite)){require(version==kPSActionReferenceSuitePrevVersion);*value=&reference;}
 else{require(!std::strcmp(name,kPSActionDescriptorSuite)&&version==kPSActionDescriptorSuiteVersion2);*value=&descriptor;}
 return 0;
}
inline SPErr release(const char*,int32){++released;return 0;}
inline void reset(){acquired=released=live_refs=live_desc=stage=fail_stage=fail_acquire=0;}
inline void run(){
 control.Get=get;control.StringIDToTypeID=string_id;
 reference.Make=make;reference.Free=free_ref;reference.PutProperty=put_property;reference.PutEnumerated=put_enum;
 descriptor.Free=free_desc;descriptor.GetObject=get_object;descriptor.GetStringLength=get_length;descriptor.GetString=get_string;
 SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
 FilterRecord host{};host.sSPBasic=&basic;
 for(const auto* value:{"zh_CN","zh_TW","en_US","de_DE"}){
  reset();locale=value;require(psui::host_locale(&host)==locale);require(acquired==3&&released==3&&live_refs==0&&live_desc==0);
 }
 for(const auto& value:{std::string(),std::string(129,'x'),std::string("zh_CN;bad")}){
  reset();locale=value;require(psui::host_locale(&host).empty());require(acquired==released&&live_refs==0&&live_desc==0);
 }
 for(int fail=1;fail<=9;++fail){
  reset();locale="zh_CN";fail_stage=fail;require(psui::host_locale(&host).empty());require(acquired==released&&live_refs==0&&live_desc==0);
 }
 for(int fail=1;fail<=3;++fail){
  reset();fail_acquire=fail;require(psui::host_locale(&host).empty());require(acquired==released+1&&live_refs==0&&live_desc==0);
 }
 reset();locale="zh_CN";control.Get=nullptr;require(psui::host_locale(&host).empty());require(acquired==released);control.Get=get;
}
} // namespace localeqa
