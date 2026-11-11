#include "chromatic_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace chromatic;
void check(bool ok,const char* text){if(!ok){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
Image canvas(int w,int h){Image im{w,h,0,0,{}};im.rgba.resize(std::size_t(w)*h*4);return im;}
double value(const Image& im,int x,int y,int k){return im.rgba[(std::size_t(y)*im.width+x)*4+k];}
void close(double a,double b,const char* text){check(std::abs(a-b)<2e-5,text);}
int main(){
 auto impulse=canvas(65,33);const auto center=(std::size_t(16)*65+32)*4;for(int k=0;k<4;++k)impulse.rgba[center+k]=1;
 auto p=preset(1);p.amount=4;p.edge=Edge::transparent;Image out;
 check(render(impulse,p,out),"RGB render");
 close(value(out,36,16,0),1,"red shifts right four pixels");close(value(out,32,16,1),1,"green remains centered");close(value(out,28,16,2),1,"blue shifts left four pixels");
 close(value(out,36,16,1),0,"red fringe has no green");close(value(out,36,16,3),1,"RGB split expands alpha");
 p.mix=0;check(render(impulse,p,out)&&out.rgba==impulse.rgba,"zero mix exact identity");
 p=preset(2);p.amount=0;p.softness=0;check(render(impulse,p,out)&&out.rgba==impulse.rgba,"zero spectral displacement exact identity");
 auto flat=canvas(31,19);for(std::size_t i=0;i<flat.rgba.size();i+=4){flat.rgba[i]=.2f;flat.rgba[i+1]=.7f;flat.rgba[i+2]=1.6f;flat.rgba[i+3]=.4f;}
 for(Mode mode:{Mode::rgb,Mode::spectrum})for(bool light:{false,true}){
  p=preset(2);p.mode=mode;p.edge=Edge::repeat;p.linear_light=light;p.softness=2;p.channels[0].blur=1.5;p.hue=73;p.saturation=150;
  check(render(flat,p,out),"constant field");for(std::size_t i=0;i<out.rgba.size();++i)close(out.rgba[i],flat.rgba[i],"white balance, HDR and alpha conserved on constant input");
 }
 p=preset(2);p.amount=20;p.angle=0;p.softness=.5;p.edge=Edge::repeat;
 auto black=impulse;for(std::size_t i=3;i<black.rgba.size();i+=4)black.rgba[i]=1;
 Image opaque;check(render(impulse,p,out)&&render(black,p,opaque),"transparent/black spectrum");
 for(std::size_t i=0;i<out.rgba.size();i+=4)for(int k=0;k<3;++k)close(out.rgba[i+k]*out.rgba[i+3],opaque.rgba[i+k],"linear-light interpolation survives transparent compositing");
 p.softness=0;p.samples=48;check(render(impulse,p,out),"continuous spectrum");
 double centroid[3]{},sum[3]{};int colored=0;
 for(int y=0;y<33;++y)for(int x=0;x<65;++x){double maximum=0;for(int k=0;k<3;++k){const double e=value(out,x,y,k)*value(out,x,y,3);sum[k]+=e;centroid[k]+=x*e;maximum=std::max(maximum,e);}if(maximum>.005)++colored;}
 check(centroid[2]/sum[2]<centroid[1]/sum[1]&&centroid[1]/sum[1]<centroid[0]/sum[0],"blue-green-red prism order");check(colored>8,"spectrum produces more than three hard copies");
 auto gradient=canvas(33,33);for(int y=0;y<33;++y)for(int x=0;x<33;++x){const auto i=(std::size_t(y)*33+x)*4;gradient.rgba[i]=x/32.f;gradient.rgba[i+1]=y/32.f;gradient.rgba[i+2]=(x+y)/64.f;gradient.rgba[i+3]=1;}
 p=preset(0);p.amount=15;check(render(gradient,p,out),"radial falloff");for(int k=0;k<4;++k)close(value(out,16,16,k),value(gradient,16,16,k),"radial center is fixed");
 p.amount=0;p.channels[0].scale=110;p.channels[0].rotation=30;check(render(gradient,p,out),"channel transform");close(value(out,16,16,0),value(gradient,16,16,0),"channel rotation and scale share the document center");
 p=preset(1);p.amount=0;p.channels[0].x=2;p.channels[1].y=-1;p.edge=Edge::repeat;check(render(gradient,p,out),"individual channel offsets");close(value(out,10,10,0),value(gradient,8,10,0),"red independent offset");close(value(out,10,10,1),value(gradient,10,11,1),"green independent offset");
 auto ramp=canvas(10,3);for(int y=0;y<3;++y)for(int x=0;x<10;++x){const auto i=(std::size_t(y)*10+x)*4;ramp.rgba[i]=(x+1)/10.f;ramp.rgba[i+3]=1;}
 const double expected[]={0,.1,.3,.9};for(int edge=0;edge<4;++edge){p.edge=Edge(edge);check(render(ramp,p,out),"edge behavior");close(value(out,0,1,0),expected[edge],"transparent/repeat/reflect/wrap positions");}
 p=preset(1);p.amount=4;Image mask=canvas(3,2);check(render(impulse,p,out,&mask)&&out.rgba==impulse.rgba,"transparent mask excludes effect");p.invert_mask=true;check(render(impulse,p,out,&mask)&&out.rgba!=impulse.rgba,"inverted mask enables effect");
 p=preset(3);auto dark=flat;for(std::size_t i=0;i<dark.rgba.size();i+=4){dark.rgba[i]=dark.rgba[i+1]=dark.rgba[i+2]=.1f;dark.rgba[i+3]=1;}check(render(dark,p,out),"highlights-only dark input");for(std::size_t i=0;i<dark.rgba.size();++i)close(out.rgba[i],dark.rgba[i],"dark base retained");
 p=preset(1);p.alpha=Alpha::preserve;check(render(impulse,p,out),"preserve alpha");for(std::size_t i=3;i<out.rgba.size();i+=4)close(out.rgba[i],impulse.rgba[i],"source alpha remains exact");
 p.alpha=Alpha::black;check(render(impulse,p,out),"black output");for(std::size_t i=3;i<out.rgba.size();i+=4)close(out.rgba[i],1,"opaque on black");
 auto hidden=canvas(11,7);hidden.rgba[0]=99;p=preset(2);check(render(hidden,p,out),"hidden RGB");for(float q:out.rgba)close(q,0,"invisible RGB never leaks");
 p=preset(2);int polls=0;out=flat;const auto prior=out.rgba;check(!render(gradient,p,out,nullptr,[&]{return ++polls>3;})&&out.rgba==prior,"cancel cannot publish partial image");
 p.amount=std::numeric_limits<double>::quiet_NaN();check(!render(gradient,p,out),"NaN rejected");p=preset(1);p.samples=999;check(!render(gradient,p,out),"sample budget bounded");
 p=preset(1);p.amount=0;p.channels[0].x=2;p.edge=Edge::repeat;auto inplace=gradient;check(render(inplace,p,inplace),"in-place output supported");close(value(inplace,10,10,0),value(gradient,8,10,0),"in-place source lifetime");
 std::cout<<"PASS: RGB positions; identity; HDR/white balance; gamma/alpha black equivalence; spectral order/continuity; radial center; transforms; four edges; masks; highlight base; alpha modes; hidden RGB; cancellation; bounds; in-place.\n";
}
