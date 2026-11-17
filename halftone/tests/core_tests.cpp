#include "halftone_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace halftone;
int checks=0;void check(bool x,const char* why){++checks;if(!x)throw std::runtime_error(why);}
Image solid(int w,int h,float gray,float alpha=1){Image a{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<a.rgba.size();i+=4){for(int k=0;k<3;k++)a.rgba[i+k]=gray;a.rgba[i+3]=alpha;}return a;}
bool pixels(const Image& a,const Image& b){return a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*sizeof(float));}
double mean(const Image& a,int channel=0){double sum=0;for(std::size_t i=channel;i<a.rgba.size();i+=4)sum+=a.rgba[i];return sum/(a.width*a.height);}
int main(int argc,char** argv){try{
 Parameters p;check(valid(p),"defaults valid");Image out;
 for(int shape=0;shape<4;shape++)for(int polarity=0;polarity<2;polarity++){
  p={};p.shape=shape;p.polarity=polarity;p.spacing=16;p.angle=0;p.smooth=0;p.softness=0;
  double previous=-1;for(float level:{0.f,.1f,.25f,.5f,.75f,.9f,1.f}){auto src=solid(128,128,level);check(render(src,p,out),"constant tone");double m=mean(out);if(std::abs(m-level)>=.035){std::cerr<<"shape="<<shape<<" polarity="<<polarity<<" tone="<<level<<" measured="<<m<<'\n';check(false,"area coverage tracks tone within pixel sampling error");}check(m>=previous,"tone is monotonic");previous=m;for(std::size_t i=0;i<out.rgba.size();i+=4)check(out.rgba[i+3]==1,"opaque paper alpha");}
 }
 auto source=solid(65,47,.4f,.6f);p={};p.background=1;p.smooth=0;p.dark={.2,.4,.7};check(render(source,p,out),"transparent ink");for(std::size_t i=0;i<out.rgba.size();i+=4){check(out.rgba[i+3]<=.600001&&out.rgba[i+3]>=0,"alpha never expands");if(out.rgba[i+3]>0)for(int k=0;k<3;k++)check(std::abs(out.rgba[i+k]-p.dark[k])<1e-6,"unassociated ink colour");}
 auto ink=out;p.mix=37;check(render(source,p,out),"partial mix");for(std::size_t i=0;i<out.rgba.size();i+=4){double a=source.rgba[i+3]*.63+ink.rgba[i+3]*.37;check(std::abs(out.rgba[i+3]-a)<1e-6,"premultiplied mix alpha");for(int k=0;k<3;k++)check(std::abs(out.rgba[i+k]-(source.rgba[i+k]*source.rgba[i+3]*.63+ink.rgba[i+k]*ink.rgba[i+3]*.37)/a)<1e-6,"premultiplied mix rgb");}
 p.mix=0;source.rgba[0]=3.7f;check(render(source,p,out)&&pixels(source,out),"zero mix exact HDR bypass");
 auto visible=solid(97,41,1);for(int y=0;y<visible.height;y++)for(int x=0;x<45;x++){auto i=(std::size_t(y)*visible.width+x)*4;visible.rgba[i]=visible.rgba[i+1]=visible.rgba[i+2]=0;visible.rgba[i+3]=0;}
 p={};p.smooth=12;check(render(visible,p,out),"alpha-aware smoothing");for(int y=0;y<visible.height;y++)for(int x=45;x<visible.width;x++)check(out.rgba[(std::size_t(y)*visible.width+x)*4]>.99,"hidden black does not stain white");
 p={};p.smooth=0;p.angle=23;auto a=solid(65,47,.5);Image first;check(render(a,p,first),"origin base");a.left=-19;a.top=37;p.shift_x=-19;p.shift_y=37;check(render(a,p,out)&&pixels(out,first),"document-coordinate shift invariance");
 p={};p.spacing=.25;check(render(a,p,out)&&std::abs(mean(out)-.5)<1e-6,"subpixel preview integrates to mean");
 for(int shape:{0,3}){p={};p.shape=shape;p.spacing=2;p.aspect=10;p.smooth=0;check(render(a,p,out),"stretched fine screen");float low=1,high=0;for(std::size_t i=0;i<out.rgba.size();i+=4){low=std::min(low,out.rgba[i]);high=std::max(high,out.rgba[i]);}check(high-low>.1,"narrow axis does not erase visible long-axis structure");}
 for(auto size:{std::pair{1,1},std::pair{1,17},std::pair{13,1},std::pair{33,19}}){auto s=solid(size.first,size.second,.3f);p={};p.smooth=200;check(render(s,p,out)&&out.valid(),"tiny image with large smoothing");}
 auto hdr=solid(129,79,.4f);hdr.rgba[0]=std::numeric_limits<float>::infinity();hdr.rgba[3]=std::numeric_limits<float>::quiet_NaN();p={};check(render(hdr,p,out),"nonfinite input");for(float v:out.rgba)check(std::isfinite(v),"finite output");
 Image sentinel=solid(2,2,.7);out=sentinel;int polls=0;p={};auto medium=solid(513,257,.4f);check(!render(medium,p,out,[&]{return ++polls>5;})&&pixels(out,sentinel),"cancel never publishes partial output");
 p.spacing=0;check(!valid(p)&&!render(medium,p,out),"invalid spacing rejected");p={};p.dark[0]=2;check(!valid(p),"invalid colour rejected");Image empty;p={};check(!render(empty,p,out),"empty input rejected");
 std::ofstream raw(argc>1?argv[1]:"halftone-deterministic.f32",std::ios::binary);
 for(int index=0;index<6;index++){p=preset(index);check(valid(p),"preset valid");for(std::size_t i=0;i<medium.rgba.size();i+=4){medium.rgba[i]=float((i*7)%129)/128;medium.rgba[i+1]=float((i*13)%113)/112;medium.rgba[i+2]=float((i*3)%97)/96;medium.rgba[i+3]=float((i*11)%31)/30;}
  check(render(medium,p,out),"parallel deterministic fixture");raw.write(reinterpret_cast<const char*>(out.rgba.data()),std::streamsize(out.rgba.size()*sizeof(float)));
 }
 check(raw.good(),"fixture write");std::cout<<"PASS: "<<checks<<" assertions; tonal area, four shapes, polarity, alpha-aware smoothing, premultiplied mix, HDR, minification, coordinates, cancellation, tiny inputs and presets.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
