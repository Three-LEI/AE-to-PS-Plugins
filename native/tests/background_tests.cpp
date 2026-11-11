#include "../../displacement/tests/mock_host.hpp"
#include "psui/preview_background.hpp"
#include "PITerminology.h"
#include <iostream>
#include <cstdlib>
void check(bool v,const char* m){if(!v){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
void check_near(int value,int expected,const char* m){check(std::abs(value-expected)<=1,m);}
void verify(int depth){
 mock::Host host(8,8,depth);host.layers[0].sheetID=22;host.layers[1].sheetID=11;
 host.layers[1].isVisible=true;host.channels[0][0].target=true;
 for(std::size_t i=0;i<host.pixels[1].size();i+=4){host.pixels[1][i]=0;host.pixels[1][i+1]=0;host.pixels[1][i+2]=1;host.pixels[1][i+3]=1;}
 // A fully opaque green layer above the target must never occlude the preview.
 ReadLayerDesc above;above.sheetID=33;above.maxVersion=3;above.isVisible=true;
 ReadChannelDesc channels[4];mock::Port ports[4];std::vector<float> top(8*8*4,0);
 for(std::size_t i=0;i<top.size();i+=4)top[i+1]=top[i+3]=1;
 for(int k=0;k<4;++k){ports[k]={k,depth,8,8,&top};channels[k].port=reinterpret_cast<PIChannelPort>(&ports[k]);channels[k].depth=depth;channels[k].maxVersion=5;channels[k].bounds=host.doc.bounds;if(k<2)channels[k].next=&channels[k+1];}
 above.compositeChannelsList=&channels[0];above.transparency=&channels[3];above.next=&host.layers[0];host.doc.layersDescriptor=&above;
 psui::ProxyImage g;g.width=g.height=8;psui::PreviewBackground bg;
 bg.prepare(&host.f,g);check(!bg.cached,"disabled does not read or cache host data");
 bg.enabled=true;bg.prepare(&host.f,g);check(bg.status==psui::PreviewBackground::Status::ready,"readable ordered pixel stack");
 const std::array<float,4> red{1,0,0,.5f},empty{0,0,0,0};
 auto p=bg.pixel(0,0,red,red);check_near(p[0],128,"red target opacity");check_near(p[1],0,"upper green layer excluded");check_near(p[2],128,"lower blue visible");
 p=bg.pixel(0,0,red,{1,1,1,1},0);check_near(p[0],128,"selection zero retains source");check_near(p[2],128,"selection retains background");
 p=bg.pixel(0,0,red,{1,1,1,1},1);check(p[0]==255&&p[1]==255&&p[2]==255,"generator opacity covers background");
 p=bg.pixel(0,0,red,{0,1,0,.5f},.5f);check_near(p[0],64,"premultiplied selection red");check_near(p[1],64,"premultiplied selection green");check_near(p[2],128,"selection alpha remains .5");
 const auto original=host.pixels[1];
 host.layers[0].opacity=128;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,red,red);check_near(p[0],64,"target layer opacity");check_near(p[2],191,"opacity reveals background");
 host.layers[0].opacity=255;host.layers[0].blendMode=PIBlendMultiply;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,{1,0,0,1},{1,0,0,1});check(p[0]==0&&p[1]==0&&p[2]==0,"target multiply blend");host.layers[0].blendMode=PIBlendNormal;
 host.layers[0].blendMode=enumMultiply;host.layers[1].blendMode=enumNormal;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,{1,0,0,1},{1,0,0,1});check(p[0]==0&&p[1]==0&&p[2]==0&&bg.status==psui::PreviewBackground::Status::ready,"Action Manager blend IDs normalized");host.layers[0].blendMode=PIBlendNormal;
 host.layers[1].isVisible=false;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,empty,empty);check(p[0]==200&&p[1]==200&&p[2]==200,"hidden lower layers excluded");
 host.layers[1].isVisible=true;
 // The enabled mask halves the lower blue opacity, then the checker shows through.
 auto mask=channels[3];std::vector<float> mask_pixels(8*8*4,.5f);mock::Port mask_port{0,depth,8,8,&mask_pixels};mask.port=reinterpret_cast<PIChannelPort>(&mask_port);mask.isEnabled=true;
 host.layers[1].layerMask=&mask;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,empty,empty);check_near(p[0],100,"lower mask applies transparency");check_near(p[2],228,"lower mask plus checker");
 mask.isEnabled=false;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,empty,empty);check(p[0]==0&&p[2]==255,"disabled mask ignored");
 host.layers[1].layerMask=nullptr;host.layers[0].layerMask=&mask;mask.isEnabled=true;bg.cached=false;bg.prepare(&host.f,g);p=bg.pixel(0,0,{1,0,0,1},{1,0,0,1});check_near(p[0],128,"target mask");check_near(p[2],128,"target mask reveals lower layer");host.layers[0].layerMask=nullptr;
 // Cache reuse and viewport geometry changes.
 host.pixels[1][0]=1;bg.prepare(&host.f,g);p=bg.pixel(0,0,empty,empty);check(p[0]==0,"same viewport uses cached snapshot");
 g.origin_x=g.origin_y=2;g.step=2;g.width=g.height=2;bg.prepare(&host.f,g);check(bg.width==2&&bg.step==2,"zoom/pan regenerate aligned background");
 host.layers[1].isAdjustor=true;bg.cached=false;bg.prepare(&host.f,g);check(bg.status==psui::PreviewBackground::Status::approximate,"adjustments explicitly marked approximate");
 // No target identity: never substitute the merged document with its top layers.
 host.channels[0][0].target=false;host.doc.targetCompositeChannels=nullptr;bg.cached=false;bg.prepare(&host.f,g);check(bg.status==psui::PreviewBackground::Status::unavailable,"ambiguous target fails safely");
 host.doc.layersDescriptor=nullptr;bg.cached=false;bg.prepare(&host.f,g);check(bg.status==psui::PreviewBackground::Status::ready,"flat document has empty lower stack");
 bg.enabled=false;p=bg.pixel(0,0,red,red);check_near(p[0],228,"off restores checker preview");check_near(p[2],100,"off ignores backdrop");
 check(host.pixels[1].size()==original.size(),"host buffers unchanged in size");
}
int main(){for(int d:{8,16,32})verify(d);std::cout<<"PASS: background RGB8/PS16/float32; exclude upper + hidden layers; lower alpha; target opacity; masks/disabled masks; multiply; premultiplied selection; cache/zoom/pan; unsupported/absent target; checker fallback.\n";}
