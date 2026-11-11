// Real typed schemas and kernels; fake host-owned descriptors exercise failures
// without depending on Photoshop's process-global Action Manager state.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "parameter-test-includes.inc"
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <iostream>
#include <map>
#include <variant>
#include <stdexcept>
namespace fake {
using Value=std::variant<int32,double,bool,std::string>;
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
std::string quote(const std::string& value){std::string out="\"";for(unsigned char c:value){if(c=='"'||c=='\\'){out+='\\';out+=char(c);}else if(c<32){char b[7];std::snprintf(b,sizeof(b),"\\u%04x",c);out+=b;}else out+=char(c);}return out+'"';}
void json(std::ostream& out,const Model& value){out<<'[';bool comma=false;for(const auto& [key,v]:value){if(comma)out<<',';comma=true;char id[5]={char(key>>24),char(key>>16),char(key>>8),char(key),0};out<<"{\"id\":"<<quote(id)<<",\"type\":";if(auto p=std::get_if<int32>(&v))out<<"\"integer\",\"value\":"<<*p;else if(auto p=std::get_if<double>(&v))out<<"\"number\",\"value\":"<<std::setprecision(17)<<*p;else if(auto p=std::get_if<bool>(&v))out<<"\"flag\",\"value\":"<<(*p?"true":"false");else out<<"\"layer\",\"value\":"<<quote(std::get<std::string>(v));out<<'}';}out<<']';}
}
template<class P,auto Valid,std::uint32_t Magic>struct Effect {using Parameters=P;static constexpr auto magic=Magic;static bool valid(const P& p){return Valid(p);}};
template<class P,auto Valid>struct Mutator {
 P& p;int variant;
 bool valid(){return Valid(p);}
 void number(const char*,double& v){auto old=v;for(double next:{old+.125*variant,old-.125*variant,old*.75,old*.5}){v=next;if(valid()&&v!=old)return;}v=old;}
 template<class T>void integer(const char*,T& v){if constexpr(requires{p.kind;})if(static_cast<const void*>(&v)==static_cast<const void*>(&p.kind))return;auto old=v;if constexpr(std::is_same_v<T,std::uint32_t>){if(variant==2){v=0xffffffffu;if(valid())return;}}for(int delta:{variant,-variant,1,-1}){v=static_cast<T>(static_cast<std::int64_t>(old)+delta);if(valid()&&v!=old)return;}v=old;}
 void flag(const char*,bool& v){v=!v;if(!valid())v=!v;}
 void layer(const char*,std::uint64_t& key,std::uint32_t* transient=nullptr){key=psui::layer_name_key("鎏金_参考图层");if(transient)*transient=77;}
};
bool tint_valid(const tint::Parameters& p){tint::ConstImage in;tint::Image out;return tint::process(in,out,p)==tint::Status::ok;}
template<class P,auto Valid,std::uint32_t Magic>void run(const char* event,int kind,std::ostream& fixtures,bool& comma){
 using E=Effect<P,Valid,Magic>;using namespace fake;using namespace psfilter::parameters;
 P source{};if constexpr(requires{source.kind;})source.kind=static_cast<decltype(source.kind)>(kind);
 std::vector<psui::HostLayer> layers(1);layers[0].name="鎏金_参考图层";layers[0].name_key=psui::layer_name_key(layers[0].name);layers[0].id=77;
 SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;HandleProcs hp{};hp.disposeProc=dispose;PIDescriptorParameters dp{};FilterRecord host{};host.sSPBasic=&basic;host.handleProcs=&hp;host.descriptorParameters=&dp;dp.playInfo=plugInDialogDontDisplay;
 for(int variant=1;variant<=2;++variant){
  P value=source;Mutator<P,Valid> mutate{value,variant};Schema<P>::visit(mutate,value);require(Valid(value),"non-default valid fixture");
  step=0;fail_at=0;require(write<E>(&host,layers,value)==noErr,"typed parameter serialization");const int write_steps=step;clean(1);
  const auto expected=reinterpret_cast<Saved*>(dp.descriptor)->data;
  if(comma)fixtures<<',';comma=true;fixtures<<"{\"event\":"<<quote(event)<<",\"variant\":"<<variant<<",\"fields\":";json(fixtures,expected);fixtures<<'}';
  P restored=source;bool interactive=true;step=0;require(read<E>(&host,layers,restored,interactive)==noErr&&!interactive,"restore without opening UI");const int read_steps=step;clean(1);
  require(write<E>(&host,layers,restored)==noErr&&reinterpret_cast<Saved*>(dp.descriptor)->data==expected,"all typed fields and Unicode layer selections round trip");
  for(int fail=1;fail<=write_steps;++fail){step=0;fail_at=fail;auto old=dp.descriptor;require(write<E>(&host,layers,value)!=noErr,"write failure propagated");require(dp.descriptor==old&&reinterpret_cast<Saved*>(old)->data==expected,"failed writes leave previous descriptor intact");clean(1);}
  for(int fail=1;fail<=read_steps;++fail){step=0;fail_at=fail;P next=source;interactive=true;require(read<E>(&host,layers,next,interactive)!=noErr,"read failure propagated");clean(1);}
  fail_at=0;auto& data=reinterpret_cast<Saved*>(dp.descriptor)->data;data[id("ZpSv")]=int32(99);restored=source;require(read<E>(&host,layers,restored,interactive)!=noErr,"future schema rejected");data=expected;data[id("ZpId")]=int32(0);require(read<E>(&host,layers,restored,interactive)!=noErr,"wrong effect identity rejected");data=expected;
 }
 dispose(reinterpret_cast<Handle>(dp.descriptor));dp.descriptor=nullptr;clean(0);
}
int main(int argc,char** argv){try{if(argc!=2)return 2;fake::init();std::ofstream out(argv[1],std::ios::binary);out<<'[';bool comma=false;
#include "parameter-test-cases.inc"
 out<<"]\n";fake::require(out.good(),"fixture file written");std::cout<<"PASS: "<<fake::checks<<" checks; thirteen typed schemas; two full non-default fixtures per effect; Unicode refs; descriptor failures/ownership; identity/version guards.\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<" step="<<fake::step<<" injected="<<fake::fail_at<<'\n';return 1;}}
