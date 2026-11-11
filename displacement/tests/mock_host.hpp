#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include "displacement_host.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
namespace mock {
inline bool cancel=false,fail_read=false;
inline Boolean abort(){return cancel;}
struct HandleData{std::vector<char> bytes;};
inline Handle allocate(int32 n){return reinterpret_cast<Handle>(new HandleData{std::vector<char>(n)});}
inline void dispose(Handle h){delete reinterpret_cast<HandleData*>(h);}
inline int32 size(Handle h){return int32(reinterpret_cast<HandleData*>(h)->bytes.size());}
inline Ptr lock(Handle h,Boolean){return reinterpret_cast<HandleData*>(h)->bytes.data();}
inline void unlock(Handle){}
struct Port{int component=0,depth=8,width=0,height=0;const std::vector<float>* pixels=nullptr;};
inline OSErr read(ChannelReadPort p,const PSScaling* scaling,const VRect* area,const PixelMemoryDesc* memory,VRect* wrote){
 if(fail_read)return -123;const auto& port=*reinterpret_cast<Port*>(p);
 const auto s=scaling->sourceRect,d=scaling->destinationRect;
 for(int y=area->top;y<area->bottom;++y)for(int x=area->left;x<area->right;++x){
  const int xx=s.left+int((x-d.left+.5)*(s.right-s.left)/(d.right-d.left));
  const int yy=s.top+int((y-d.top+.5)*(s.bottom-s.top)/(d.bottom-d.top));
  if(xx<0||xx>=port.width||yy<0||yy>=port.height)continue;
  const float v=(*port.pixels)[(std::size_t(yy)*port.width+xx)*4+port.component];
  auto* dest=static_cast<std::uint8_t*>(memory->data)+(std::size_t(y)*memory->rowBits+x*memory->colBits+memory->bitOffset)/8;
  if(memory->depth==8)*dest=std::uint8_t(std::lround(std::clamp(v,0.f,1.f)*255));
  else if(memory->depth==16){const auto value=std::uint16_t(std::lround(std::clamp(v,0.f,1.f)*32768));std::memcpy(dest,&value,2);}
  else std::memcpy(dest,&v,4);
 }
 *wrote=*area;return noErr;
}
struct Host {
 FilterRecord f{};BigDocumentStruct big{};ReadImageDocumentDesc doc{};ChannelPortProcs ports{};HandleProcs handles{};
 ReadLayerDesc layers[2];ReadChannelDesc channels[2][4];Port data[2][4];
 std::vector<float> pixels[2];int width,height;
 Host(int w,int h,int depth=8):width(w),height(h){
  f.depth=depth;f.imageMode=depth==8?plugInModeRGBColor:depth==16?plugInModeRGB48:plugInModeRGB96;f.planes=4;
  f.bigDocumentData=&big;big.filterRect32={0,0,h,w};big.imageSize32={h,w};
  f.filterRect={0,0,int16(h),int16(w)};f.imageSize={int16(h),int16(w)};
  f.outLayerPlanes=3;f.outTransparencyMask=1;f.filterCase=filterCaseEditableTransparencyNoSelection;
  f.abortProc=abort;f.documentInfo=&doc;f.channelPortProcs=&ports;ports.readPixelsProc=read;
  handles.newProc=allocate;handles.disposeProc=dispose;handles.lockProc=lock;handles.unlockProc=unlock;handles.getSizeProc=size;f.handleProcs=&handles;
  doc.maxVersion=6;doc.imageMode=f.imageMode;doc.depth=depth;doc.bounds={0,0,h,w};doc.layersDescriptor=&layers[0];doc.targetCompositeChannels=&channels[0][0];
  for(int l=0;l<2;++l){pixels[l].resize(std::size_t(w)*h*4);layers[l].maxVersion=3;layers[l].sheetID=l?22:11;
   layers[l].unicodeName=reinterpret_cast<const uint16*>(l?L"采样图层（隐藏）":L"当前图层");layers[l].isVisible=l==0;
   layers[l].compositeChannelsList=&channels[l][0];layers[l].transparency=&channels[l][3];
   for(int c=0;c<4;++c){data[l][c]={c,depth,w,h,&pixels[l]};channels[l][c].port=reinterpret_cast<PIChannelPort>(&data[l][c]);channels[l][c].depth=depth;channels[l][c].bounds=doc.bounds;channels[l][c].maxVersion=5;if(c<2)channels[l][c].next=&channels[l][c+1];}
  }
  layers[0].next=&layers[1];doc.targetTransparency=&channels[0][3];
  for(int y=0;y<h;++y)for(int x=0;x<w;++x){const auto i=(std::size_t(y)*w+x)*4;
   pixels[0][i]=float(x%239)/255+(depth==32?2:0);pixels[0][i+1]=float(y%127)/255;pixels[0][i+2]=float((x*y)%255)/255;pixels[0][i+3]=(x%7)?1:0;
   pixels[1][i]=1;pixels[1][i+1]=depth==8?128.f/255:.5f;pixels[1][i+2]=.5f;pixels[1][i+3]=1;
  }
 }
 ~Host(){if(f.parameters)dispose(f.parameters);}
};
}
