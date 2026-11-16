#include "energy_host.hpp"
#include "parameter_schema.hpp"
#include "PIProperties.h"
#include <cstring>
namespace energy {
namespace {
struct Property {
 FilterRecord* f;Handle handle=nullptr;std::vector<std::uint8_t> bytes;
 explicit Property(FilterRecord* host):f(host){}
 ~Property(){if(handle&&f->handleProcs&&f->handleProcs->disposeProc)f->handleProcs->disposeProc(handle);}
 bool get(unsigned32 key,int index){
  if(!f||!f->propertyProcs||!f->propertyProcs->getPropertyProc||!f->handleProcs||!f->handleProcs->getSizeProc||!f->handleProcs->lockProc||!f->handleProcs->unlockProc||!f->handleProcs->disposeProc)return false;
  if(f->propertyProcs->getPropertyProc(0x3842494d,key,index,nullptr,&handle)||!handle)return false;
  const auto n=f->handleProcs->getSizeProc(handle);if(n<0||n>2*1024*1024)return false;auto* p=f->handleProcs->lockProc(handle,false);if(!p&&n)return false;if(n)bytes.assign(reinterpret_cast<std::uint8_t*>(p),reinterpret_cast<std::uint8_t*>(p)+n);f->handleProcs->unlockProc(handle);return true;
 }
};
std::string unicode_name(const std::vector<std::uint8_t>& b){
 if(b.size()%2||b.size()>16384)return {};std::wstring text(b.size()/2,L'\0');if(!b.empty())std::memcpy(text.data(),b.data(),b.size());while(!text.empty()&&text.back()==L'\0')text.pop_back();if(text.find(L'\0')!=std::wstring::npos)return {};
 const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);if(n<=0)return {};std::string out(n,'\0');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),out.data(),n,nullptr,nullptr);return out;
}
}
std::vector<HostPath> host_paths(FilterRecord* f){
 std::vector<HostPath> result;if(!f||!f->propertyProcs||!f->propertyProcs->getPropertyProc)return result;intptr_t count=0;
 if(f->propertyProcs->getPropertyProc(0x3842494d,propNumberOfPaths,0,&count,nullptr)||count<0||count>4096)return result;
 for(int i=0;i<count;i++){Property name(f);std::string text;if(name.get(propUnicodePathName,i))text=unicode_name(name.bytes);if(text.empty())text="Path "+std::to_string(i+1);result.push_back({i,std::move(text)});}return result;
}
bool import_path(FilterRecord* f,int index,Inputs& out,std::string& error){
 const auto paths=host_paths(f);const HostPath* chosen=nullptr;for(const auto& p:paths)if(p.index==index)chosen=&p;if(!chosen){error="Path is unavailable";return false;}
 Property data(f);if(!data.get(propPathContents,index)){error="Photoshop did not expose this path";return false;}
 const double w=f->bigDocumentData?f->bigDocumentData->imageSize32.h:f->imageSize.h,h=f->bigDocumentData?f->bigDocumentData->imageSize32.v:f->imageSize.v;
 Inputs next;next.name=chosen->name;if(!parse_path(data.bytes,w,h,next.path,error))return false;out=std::move(next);return true;
}
OSErr read_settings(FilterRecord* f,Parameters& p,Inputs& inputs,bool& interactive){
 auto* dp=f?f->descriptorParameters:nullptr;if(!dp||!dp->descriptor)return noErr;
 using namespace psfilter::parameters;Suite q(f,true);if(!q.d||!q.z||q.d->HandleToDescriptor(dp->descriptor,&q.desc)||!q.desc)return filterBadParameters;
 Boolean present=false;if(q.d->HasKey(q.desc,id("ZpSv"),&present))return filterBadParameters;if(!present)return noErr;
 int32 version=0,identity=0;if(q.d->GetInteger(q.desc,id("ZpSv"),&version)||version<1||version>6||q.d->GetInteger(q.desc,id("ZpId"),&identity)||std::uint32_t(identity)!=magic)return filterBadParameters;
 Parameters next=p;reset_extension(next);reset_glow(next);reset_electric(next);reset_burning(next);reset_unified(next);const std::vector<psui::HostLayer> layers;Reader a{q,layers};Schema<Parameters>::visit(a,next);if(a.error||!valid(next))return filterBadParameters;
 if((version<6&&next.engine!=0)||(version==6&&next.engine!=1))return filterBadParameters;
 if(version<6&&((version<3&&next.glow_model!=0)||(version>=3&&next.glow_model!=1)||(version<=3&&next.style>2)||(version==4&&next.style!=3)||(version==5&&next.style!=4)))return filterBadParameters;
 if(version==1&&(next.input>=2||next.style!=0))return filterBadParameters;
 Inputs snapshot;Boolean has=false;if(q.d->HasKey(q.desc,id("ZePG"),&has))return filterBadParameters;if(has){int32 size=0;if(q.d->GetDataLength(q.desc,id("ZePG"),&size)||size<8||size>2*1024*1024)return filterBadParameters;std::vector<std::uint8_t> bytes(size);if(q.d->GetData(q.desc,id("ZePG"),bytes.data())||!decode_path(bytes,snapshot.path))return filterBadParameters;}
 if(q.d->HasKey(q.desc,id("ZePN"),&has))return filterBadParameters;if(has&&!text(q,q.desc,id("ZePN"),snapshot.name))return filterBadParameters;
 // Silent playback must use its own geometry. Interactive import is explicit.
 p=next;inputs=std::move(snapshot);interactive=dp->playInfo==plugInDialogDisplay;return noErr;
}
OSErr write_settings(FilterRecord* f,const Parameters& p,const Inputs& inputs){
 if(!f||!f->descriptorParameters)return noErr;if(!valid(p)||!valid_path(inputs.path)||(p.input==1&&inputs.path.curves.empty()))return filterBadParameters;
 using namespace psfilter::parameters;Suite q(f,true);if(!q.d||!q.z||q.d->Make(&q.desc)||!q.desc)return filterBadParameters;const std::vector<psui::HostLayer> layers;Writer a{q,layers};a.integer("ZpSv",p.engine?6:p.style==4?5:p.style==3?4:p.glow_model?3:p.input>=2||p.style!=0?2:1);a.integer("ZpId",magic);Schema<Parameters>::visit(a,p);if(a.error)return a.error;
 auto bytes=encode_path(inputs.path);if(bytes.empty()||q.d->PutData(q.desc,id("ZePG"),int32(bytes.size()),bytes.data())||put_text(q,q.desc,id("ZePN"),inputs.name.c_str()))return filterBadParameters;
 return commit(f,q,q.desc);
}
}
