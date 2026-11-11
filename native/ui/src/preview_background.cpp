#include "psui/preview_background.hpp"
#include "psui/library_panel.hpp"
#include "PIProperties.h"
#include "PITerminology.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
namespace psui {
namespace {
using Color=std::array<float,3>;
using Pixel=std::array<float,4>;
// Some hosts return Action Manager blend IDs (Nrml/Mltp), although the SDK
// describes the layer field using the channel-port IDs (norm/mul ).
PIType normalize(PIType m){
 switch(m){
 case enumNormal:return PIBlendNormal;case enumMultiply:return PIBlendMultiply;case enumScreen:return PIBlendScreen;
 case enumDarken:return PIBlendDarken;case enumLighten:return PIBlendLighten;case enumOverlay:return PIBlendOverlay;
 case enumHardLight:return PIBlendHardLight;case enumSoftLight:return PIBlendSoftLight;case enumDifference:return PIBlendDifference;
 case enumExclusion:return PIBlendExclusion;case enumColorDodge:return PIBlendColorDodge;case enumColorBurn:return PIBlendColorBurn;
 case enumHue:return PIBlendHue;case enumSaturation:return PIBlendSaturation;case enumColor:return PIBlendColor;case enumLuminosity:return PIBlendLuminosity;
 default:return m;
 }
}
float unit(float x){return std::clamp(std::isfinite(x)?x:0.f,0.f,1.f);}
float lum(Color c){return .3f*c[0]+.59f*c[1]+.11f*c[2];}
Color set_lum(Color c,float l){
 const float d=l-lum(c);for(auto& x:c)x+=d;
 const float v=lum(c),low=*std::min_element(c.begin(),c.end()),high=*std::max_element(c.begin(),c.end());
 if(low<0&&v>low)for(auto& x:c)x=v+(x-v)*v/(v-low);
 if(high>1&&high>v)for(auto& x:c)x=v+(x-v)*(1-v)/(high-v);
 return c;
}
Color set_sat(Color c,float s){
 const auto mm=std::minmax_element(c.begin(),c.end());const float lo=*mm.first,hi=*mm.second;
 for(auto& x:c)x=hi>lo?(x-lo)*s/(hi-lo):0;return c;
}
float saturation(Color c){return *std::max_element(c.begin(),c.end())-*std::min_element(c.begin(),c.end());}
bool supported(PIType m){
 switch(m){case PIBlendNormal:case PIBlendMultiply:case PIBlendScreen:case PIBlendDarken:case PIBlendLighten:
 case PIBlendOverlay:case PIBlendHardLight:case PIBlendSoftLight:case PIBlendDifference:case PIBlendExclusion:
 case PIBlendColorDodge:case PIBlendColorBurn:case PIBlendLinearDodge:case PIBlendLinearBurn:case PIBlendLinearLight:
 case PIBlendVividLight:case PIBlendPinLight:case PIBlendHardMix:case PIBlendHue:case PIBlendSaturation:
 case PIBlendColor:case PIBlendLuminosity:return true;default:return false;}
}
Color blend(Color b,Color s,PIType mode){
 if(mode==PIBlendHue)return set_lum(set_sat(s,saturation(b)),lum(b));
 if(mode==PIBlendSaturation)return set_lum(set_sat(b,saturation(s)),lum(b));
 if(mode==PIBlendColor)return set_lum(s,lum(b));
 if(mode==PIBlendLuminosity)return set_lum(b,lum(s));
 Color result{};
 for(int k=0;k<3;++k){const float a=b[k],v=s[k];float r=v;
  auto dodge=[](float x,float y){return y>=1?1.f:std::min(1.f,x/(1-y));};
  auto burn=[](float x,float y){return y<=0?0.f:1-std::min(1.f,(1-x)/y);};
  switch(mode){
   case PIBlendMultiply:r=a*v;break;
   case PIBlendScreen:r=1-(1-a)*(1-v);break;
   case PIBlendDarken:r=std::min(a,v);break;case PIBlendLighten:r=std::max(a,v);break;
   case PIBlendOverlay:r=a<=.5f?2*a*v:1-2*(1-a)*(1-v);break;
   case PIBlendHardLight:r=v<=.5f?2*a*v:1-2*(1-a)*(1-v);break;
   case PIBlendSoftLight:{const float d=a<=.25f?((16*a-12)*a+4)*a:std::sqrt(a);r=v<=.5f?a-(1-2*v)*a*(1-a):a+(2*v-1)*(d-a);break;}
   case PIBlendDifference:r=std::abs(a-v);break;case PIBlendExclusion:r=a+v-2*a*v;break;
   case PIBlendColorDodge:r=dodge(a,v);break;case PIBlendColorBurn:r=burn(a,v);break;
   case PIBlendLinearDodge:r=std::min(1.f,a+v);break;case PIBlendLinearBurn:r=std::max(0.f,a+v-1);break;
   case PIBlendLinearLight:r=unit(a+2*v-1);break;
   case PIBlendVividLight:r=v<=.5f?burn(a,2*v):dodge(a,2*v-1);break;
   case PIBlendPinLight:r=v<=.5f?std::min(a,2*v):std::max(a,2*v-1);break;
   case PIBlendHardMix:r=(v<=.5f?burn(a,2*v):dodge(a,2*v-1))<.5f?0:1;break;
   default:break;
  }result[k]=r;
 }return result;
}
Pixel over(Pixel background,Pixel source,PIType mode){
 const float sa=unit(source[3]),ba=unit(background[3]),alpha=sa+ba-sa*ba;
 const Color b{unit(background[0]),unit(background[1]),unit(background[2])},s{unit(source[0]),unit(source[1]),unit(source[2])};
 const auto mix=blend(b,s,mode);Pixel out{0,0,0,alpha};
 if(alpha>0)for(int k=0;k<3;++k)out[k]=((1-sa)*ba*b[k]+(1-ba)*sa*s[k]+sa*ba*mix[k])/alpha;
 return out;
}
OSErr read(FilterRecord* f,ReadChannelDesc* channel,const ProxyImage& g,std::vector<float>& values,std::vector<float>* covered=nullptr){
 if(!channel||!channel->port||channel->minVersion>kCurrentMaxVersReadChannelDesc)return filterBadParameters;
 const int depth=channel->depth;if(depth!=8&&depth!=16&&depth!=32)return filterBadMode;
 const auto size=std::size_t(g.width)*g.height;const int bytes=depth/8;
 std::vector<std::uint8_t> data(size*bytes,0);values.assign(size,0);
 const double right=g.origin_x+g.width*g.step,bottom=g.origin_y+g.height*g.step;
 if(g.origin_x<std::numeric_limits<int32>::min()||g.origin_y<std::numeric_limits<int32>::min()||right>std::numeric_limits<int32>::max()||bottom>std::numeric_limits<int32>::max())return filterBadParameters;
 PSScaling scale{{int32(std::floor(g.origin_y)),int32(std::floor(g.origin_x)),int32(std::ceil(bottom)),int32(std::ceil(right))},{0,0,g.height,g.width}};
 VRect area{0,0,g.height,g.width},wrote{};
 PixelMemoryDesc memory{data.data(),g.width*bytes*8,bytes*8,0,depth};
 const auto error=f->channelPortProcs->readPixelsProc(channel->port,&scale,&area,&memory,&wrote);if(error)return error;
 if(wrote.left<0||wrote.top<0||wrote.right>g.width||wrote.bottom>g.height)return filterBadParameters;
 if(covered)covered->assign(size,0);
 for(int y=wrote.top;y<wrote.bottom;++y)for(int x=wrote.left;x<wrote.right;++x){
  const auto i=std::size_t(y)*g.width+x;float value=0;
  if(depth==8)value=data[i]/255.f;
  else if(depth==16){std::uint16_t v;std::memcpy(&v,data.data()+i*2,2);value=v/32768.f;}
  else std::memcpy(&value,data.data()+i*4,4);
  values[i]=std::isfinite(value)?value:0;if(covered)(*covered)[i]=1;
 }
 if(f->abortProc&&f->abortProc())return userCanceledErr;
 return noErr;
}
bool mask_enabled(ReadChannelDesc* m){return m&&m->port&&(m->maxVersion<4||m->isEnabled);}
OSErr masks(FilterRecord* f,ReadLayerDesc* layer,const ProxyImage& g,std::vector<float>& coverage){
 const auto size=std::size_t(g.width)*g.height;coverage.assign(size,1);
 const std::array<ReadChannelDesc*,2> inputs{layer->layerMask,layer->maxVersion>=2?layer->vectorMask:nullptr};
 for(auto* channel:inputs)if(mask_enabled(channel)){
  std::vector<float> plane;auto error=read(f,channel,g,plane);if(error)return error;
  for(std::size_t i=0;i<size;++i)coverage[i]*=unit(plane[i]);
 }
 return noErr;
}
bool same(const PreviewBackground& b,const ProxyImage& g){return b.cached&&b.width==g.width&&b.height==g.height&&b.left==g.origin_x&&b.top==g.origin_y&&b.step==g.step;}
}
OSErr host_read_preview_channel(FilterRecord* f,ReadChannelDesc* channel,const ProxyImage& geometry,std::vector<float>& output){
 if(!f||!f->channelPortProcs||!f->channelPortProcs->readPixelsProc||geometry.width<=0||geometry.height<=0||std::size_t(geometry.width)*geometry.height>1024*1024||!std::isfinite(geometry.origin_x)||!std::isfinite(geometry.origin_y)||!std::isfinite(geometry.step)||geometry.step<=0)return filterBadParameters;
 return read(f,channel,geometry,output);
}
void PreviewBackground::prepare(FilterRecord* f,const ProxyImage& geometry){
 if(!enabled||same(*this,geometry))return;
 cached=true;width=geometry.width;height=geometry.height;left=geometry.origin_x;top=geometry.origin_y;step=geometry.step;
 error=noErr;status=Status::ready;rgba.clear();coverage.clear();target_blend=PIBlendNormal;target_opacity=1;
 const auto fail=[&](OSErr e){error=e;status=Status::unavailable;rgba.clear();coverage.clear();};
 if(width<=0||height<=0||std::size_t(width)*height>1024*1024||!std::isfinite(step)||step<=0||!std::isfinite(left)||!std::isfinite(top)||!f||!f->documentInfo||!f->channelPortProcs||!f->channelPortProcs->readPixelsProc){fail(filterBadParameters);return;}
 auto* doc=f->documentInfo;
 if(doc->minVersion>kCurrentMaxVersReadImageDocDesc){fail(filterBadParameters);return;}
 const auto size=std::size_t(width)*height;
 if(doc->maxVersion<2||!doc->layersDescriptor){rgba.assign(size*4,0);coverage.assign(size,1);return;}
 intptr_t index=-1;
 if(f->propertyProcs&&f->propertyProcs->getPropertyProc)
  if(f->propertyProcs->getPropertyProc(0x3842494d,propTargetLayerIndex,0,&index,nullptr)!=noErr)index=-1;
 std::vector<ReadLayerDesc*> layers;int count=0;
 for(auto* l=doc->layersDescriptor;l;l=l->next){if(++count>16384||l->minVersion>kCurrentMaxVersReadLayerDesc){fail(filterBadParameters);return;}layers.push_back(l);}
 ReadLayerDesc* target=nullptr;
 if(index>=0)for(auto* l:layers)if(l->sheetID==unsigned32(index)){if(target){fail(filterBadParameters);return;}target=l;}
 if(!target){
  for(auto* l:layers){const auto* channel=l->compositeChannelsList;
   if(channel&&(channel->target||(doc->targetCompositeChannels&&channel->port==doc->targetCompositeChannels->port))){if(target){fail(filterBadParameters);return;}target=l;}
  }
 }
 // Never use mergedCompositeChannels: that would include layers above target.
 if(!target){fail(filterBadParameters);return;}
 if(target->maxVersion>=2){target_blend=normalize(target->blendMode);target_opacity=target->opacity/255.f*target->fillOpacity/255.f;
  if(!supported(target_blend))status=Status::approximate;
 }
 if((error=masks(f,target,geometry,coverage))){fail(error);return;}
 rgba.assign(size*4,0);
 // Photoshop sheetID is the invocation's bottom-to-top layer index, not its
 // persistent Action Manager layer ID. Sorting also avoids list-order reliance.
 std::sort(layers.begin(),layers.end(),[](auto* a,auto* b){return a->sheetID<b->sheetID;});
 for(auto* l:layers){
  if(l->sheetID>=target->sheetID)break;
  if(l->maxVersion>=2&&!l->isVisible)continue;
  if(l->maxVersion>=2&&l->isAdjustor){status=Status::approximate;continue;}
  std::array<ReadChannelDesc*,3> channels{};auto* channel=l->compositeChannelsList;
  for(int k=0;k<3&&channel;++k,channel=channel->next)channels[k]=channel;
  if(!channels[2]){status=Status::approximate;continue;}
  std::array<std::vector<float>,4> planes;std::vector<float> layer_coverage,valid_area;
  for(int k=0;k<3;++k)if((error=read(f,channels[k],geometry,planes[k],k==0?&valid_area:nullptr))){fail(error);return;}
  if(l->transparency){if((error=read(f,l->transparency,geometry,planes[3]))){fail(error);return;}}
  else planes[3]=std::move(valid_area);
  if((error=masks(f,l,geometry,layer_coverage))){fail(error);return;}
  const float opacity=l->maxVersion>=2?l->opacity/255.f*l->fillOpacity/255.f:1;
  const auto mode=l->maxVersion>=2?normalize(l->blendMode):PIBlendNormal;
  if(!supported(mode)||(!l->isPixelBased&&l->maxVersion>=2&&!l->isContentGenerator))status=Status::approximate;
  for(std::size_t i=0;i<size;++i){
   const Pixel background{rgba[i*4],rgba[i*4+1],rgba[i*4+2],rgba[i*4+3]};
   const Pixel source{planes[0][i],planes[1][i],planes[2][i],unit(planes[3][i])*layer_coverage[i]*opacity};
   const auto result=over(background,source,mode);for(int k=0;k<4;++k)rgba[i*4+k]=result[k];
  }
 }
}
std::array<std::uint8_t,3> PreviewBackground::pixel(int x,int y,Pixel original,Pixel processed,float selection) const {
 const float weight=unit(selection),a=unit(original[3]),b=unit(processed[3]);
 const float alpha=a*(1-weight)+b*weight;Pixel target{0,0,0,alpha};
 if(alpha>0)for(int k=0;k<3;++k)target[k]=(original[k]*a*(1-weight)+processed[k]*b*weight)/alpha;
 Pixel color=target;
 if(enabled&&status!=Status::unavailable&&x>=0&&x<width&&y>=0&&y<height&&rgba.size()==std::size_t(width)*height*4){
  const auto i=std::size_t(y)*width+x;target[3]*=target_opacity*(coverage.empty()?1:coverage[i]);
  const Pixel background{rgba[i*4],rgba[i*4+1],rgba[i*4+2],rgba[i*4+3]};color=over(background,target,target_blend);
 }
 const float checker=((x/8+y/8)&1)?156.f:200.f;std::array<std::uint8_t,3> result{};
 for(int k=0;k<3;++k)result[k]=std::uint8_t(std::lround(unit(color[k])*255*unit(color[3])+checker*(1-unit(color[3]))));
 return result;
}
bool preview_background_control(LibraryPanel& ui,PreviewBackground& bg){
 const bool changed=ui.check("显示背景 / Show Background",bg.enabled);
 if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",ui.text("显示当前图层和下方可见图层，不显示上方图层。 / Show this layer and visible layers below; exclude layers above.").c_str());
 if(bg.enabled&&bg.status!=PreviewBackground::Status::ready){
  ImGui::TextDisabled("%s",ui.text(bg.status==PreviewBackground::Status::unavailable?
   "背景暂不可用 / Background unavailable":"背景为近似预览 / Approximate background").c_str());
  if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",ui.text("调整图层、图层组及部分高级混合设置无法完整复现。 / Adjustment layers, groups and some advanced blending settings cannot be reproduced fully.").c_str());
 }
 return changed;
}
}
