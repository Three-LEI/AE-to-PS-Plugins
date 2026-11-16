#pragma once
#include "parameter_schema.hpp"
#include <map>
#include <variant>
#include <stdexcept>
namespace fake {
using Value=std::variant<int32,double,bool,std::string,std::vector<std::uint8_t>>;
using Model=std::map<DescriptorKeyID,Value>;
int objects=0,handles=0,strings=0,leases=0,step=0,fail_at=0,checks=0;
struct Saved{Model data;};struct Text{std::string data;};
PSActionDescriptorProcs2 d{};ASZStringSuite2 z{};
bool fail(){return ++step==fail_at;}
void require(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
Model& model(PIActionDescriptor p){return *reinterpret_cast<Model*>(p);}
OSErr make(PIActionDescriptor* out){*out=reinterpret_cast<PIActionDescriptor>(new Model);++objects;return fail()?paramErr:noErr;}
OSErr free_object(PIActionDescriptor p){delete reinterpret_cast<Model*>(p);--objects;return noErr;}
OSErr has(PIActionDescriptor p,DescriptorKeyID key,Boolean* out){*out=model(p).count(key)!=0;return fail()?paramErr:noErr;}
template<class T>OSErr put(PIActionDescriptor p,DescriptorKeyID key,T value){model(p)[key]=value;return fail()?paramErr:noErr;}
template<class T>OSErr get(PIActionDescriptor p,DescriptorKeyID key,T* out){if(fail())return paramErr;auto it=model(p).find(key);if(it==model(p).end()||!std::holds_alternative<T>(it->second))return paramErr;*out=std::get<T>(it->second);return noErr;}
OSErr put_int(PIActionDescriptor p,DescriptorKeyID k,int32 v){return put(p,k,v);}
OSErr put_float(PIActionDescriptor p,DescriptorKeyID k,real64 v){return put(p,k,v);}
OSErr put_bool(PIActionDescriptor p,DescriptorKeyID k,Boolean v){return put(p,k,v!=0);}
OSErr get_int(PIActionDescriptor p,DescriptorKeyID k,int32* v){return get(p,k,v);}
OSErr get_float(PIActionDescriptor p,DescriptorKeyID k,real64* v){return get(p,k,v);}
OSErr get_bool(PIActionDescriptor p,DescriptorKeyID k,Boolean* v){bool b=false;auto e=get(p,k,&b);*v=b;return e;}
OSErr as_handle(PIActionDescriptor p,PIDescriptorHandle* out){*out=reinterpret_cast<PIDescriptorHandle>(new Saved{model(p)});++handles;return fail()?paramErr:noErr;}
OSErr from_handle(PIDescriptorHandle p,PIActionDescriptor* out){*out=reinterpret_cast<PIActionDescriptor>(new Model(reinterpret_cast<Saved*>(p)->data));++objects;return fail()?paramErr:noErr;}
void dispose(Handle p){delete reinterpret_cast<Saved*>(p);--handles;}
ASErr make_text(const unsigned char* bytes,ASZString* out){*out=reinterpret_cast<ASZString>(new Text{reinterpret_cast<const char*>(bytes)});++strings;return fail()?paramErr:noErr;}
ASErr free_text(ASZString p){delete reinterpret_cast<Text*>(p);--strings;return noErr;}
ASUInt32 text_length(ASZString p){return fail()?0:static_cast<ASUInt32>(reinterpret_cast<Text*>(p)->data.size()+1);}
ASErr text_bytes(ASZString p,unsigned char* bytes,ASUInt32 n,ASBoolean){if(fail())return paramErr;const auto& s=reinterpret_cast<Text*>(p)->data;if(n<s.size()+1)return paramErr;std::memcpy(bytes,s.c_str(),s.size()+1);return noErr;}
OSErr put_text(PIActionDescriptor p,DescriptorKeyID key,ASZString value){return put(p,key,reinterpret_cast<Text*>(value)->data);}
OSErr get_text(PIActionDescriptor p,DescriptorKeyID key,ASZString* out){std::string value;auto e=get(p,key,&value);if(e)return e;*out=reinterpret_cast<ASZString>(new Text{value});++strings;return noErr;}
SPErr acquire(const char* name,int32 version,const void** out){if(fail())return -1;if(!std::strcmp(name,kPSActionDescriptorSuite)){require(version==2,"old descriptor suite version");*out=&d;}else{require(!std::strcmp(name,kASZStringSuite)&&version==2,"Unicode suite version");*out=&z;}++leases;return 0;}
SPErr release(const char*,int32){--leases;return 0;}
void init(){d.Make=make;d.Free=free_object;d.HasKey=has;d.PutInteger=put_int;d.PutFloat=put_float;d.PutBoolean=put_bool;d.GetInteger=get_int;d.GetFloat=get_float;d.GetBoolean=get_bool;d.AsHandle=as_handle;d.HandleToDescriptor=from_handle;d.PutZString=put_text;d.GetZString=get_text;z.MakeFromUTF8String=make_text;z.Release=free_text;z.LengthAsUTF8String=text_length;z.AsUTF8String=text_bytes;}
void clean(int expected_handles){require(!objects&&!strings&&!leases&&handles==expected_handles,"host descriptor/string/suite ownership");}
}
