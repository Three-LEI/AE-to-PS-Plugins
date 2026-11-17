#include "flare_core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace flare;
int checks=0;void check(bool v,const char* why){++checks;if(!v)throw std::runtime_error(why);}
Image solid(int w,int h,float b=0,float a=1){Image s{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<s.rgba.size();i+=4){s.rgba[i]=s.rgba[i+1]=s.rgba[i+2]=b;s.rgba[i+3]=a;}return s;}
bool same(const Image& a,const Image& b){return a.rgba.size()==b.rgba.size()&&!std::memcmp(a.rgba.data(),b.rgba.data(),a.rgba.size()*sizeof(float));}
double energy(const Image& a){double sum=0;for(std::size_t i=0;i<a.rgba.size();i+=4)sum+=(a.rgba[i]+a.rgba[i+1]+a.rgba[i+2])*a.rgba[i+3];return sum/(a.width*a.height);}
int main(int argc,char** argv){try{
 auto src=solid(320,240,.15f,.65f);for(std::size_t i=0;i<src.rgba.size();i+=4){src.rgba[i]=float(i%113)/112;src.rgba[i+1]=float(i%97)/96;src.rgba[i+2]=float(i%61)/60;}auto p=defaults(320,240);Image out,ref;
 for(int i=0;i<8;i++){p=preset(i,320,240);check(valid(p)&&render(src,p,out)&&out.valid(),"preset renders");for(auto f:out.rgba)check(std::isfinite(f),"finite preset output");check(!same(src,out),"preset is effective");}
 p=defaults(320,240);p.mix=0;src.rgba[0]=5;check(render(src,p,out)&&same(src,out),"zero mix HDR identity");p.mix=100;p.brightness=0;check(render(src,p,out)&&same(src,out),"zero brightness identity");
 p=defaults(320,240);p.preserve_alpha=true;check(render(src,p,out),"locked alpha render");for(std::size_t i=3;i<src.rgba.size();i+=4)check(src.rgba[i]==out.rgba[i],"source alpha preserved");check(out.rgba[0]>=5,"screen never dims HDR highlights");
 p.preserve_alpha=false;auto empty=solid(320,240,7,0);p.blend=2;check(render(empty,p,out),"transparent flare");check(energy(out)>0,"flare generates on empty pixels");for(std::size_t i=0;i<out.rgba.size();i+=4)check(out.rgba[i+3]>=0&&out.rgba[i+3]<=1,"valid generated alpha");ref=out;p.blend=1;check(render(empty,p,out)&&same(out,ref),"add on transparent source agrees with flare-only emission");
 auto black=solid(320,240);p.blend=2;check(render(black,p,ref),"flare only over black");auto colour=solid(320,240,.8f,.3f);check(render(colour,p,out)&&same(out,ref),"flare only ignores original pixels");
 // Flattening straight alpha must recover exactly the same emitted light.
 p.blend=1;check(render(black,p,out),"opaque additive");for(std::size_t i=0;i<out.rgba.size();i+=4)for(int k=0;k<3;k++)check(std::abs(out.rgba[i+k]-ref.rgba[i+k]*ref.rgba[i+3])<2e-7,"transparent radiance matches additive black");
 p=defaults(320,240);check(render(src,p,ref),"coordinate reference");src.left=-51;src.top=73;p.hx-=51;p.px-=51;p.hy+=73;p.py+=73;check(render(src,p,out)&&same(ref,out),"all geometry translates in document coordinates");
 src.left=src.top=0;p=defaults(320,240);auto mask=solid(320,240,1,1);p.occlusion=1;p.occlusion_softness=0;check(visibility(mask,p)==0&&render(src,p,out,&mask)&&same(src,out),"opaque mask occludes all light");p.invert_occlusion=true;check(visibility(mask,p)==1,"invert alpha occlusion");p.invert_occlusion=false;p.occlusion=2;mask=solid(320,240,0,1);check(visibility(mask,p)==1,"black luma permits light");mask=solid(320,240,1,0);check(visibility(mask,p)==1,"transparent luma is clear");
 mask=solid(320,240,1,1);for(int y=0;y<240;y++)for(int x=0;x<160;x++)mask.rgba[(y*320+x)*4+3]=0;p.occlusion=1;p.hx=160;p.hy=120;p.occlusion_softness=20;const double vis=visibility(mask,p);check(vis>.4&&vis<.6,"soft hotspot at occluding edge");p.hx=-1000;check(visibility(mask,p)==1,"out of bounds mask is clear");
 // Components can be independently disabled; preset names never select a kernel.
 p=defaults(320,240);p.hot_bright=p.glow=p.ray_bright=p.ghost_bright=p.ring_bright=p.streak_bright=0;check(render(black,p,out)&&energy(out)==0,"all components off");
 for(int c=0;c<6;c++){auto q=p;switch(c){case 0:q.hot_bright=1;break;case 1:q.glow=1;break;case 2:q.ray_bright=1;break;case 3:q.ghost_bright=1;break;case 4:q.ring_bright=1;break;default:q.streak_bright=1;}check(render(black,q,out)&&energy(out)>0,"independent component");}
 p=defaults(320,240);p.ghosts=24;p.blades=16;p.atmosphere=100;p.seed=0xffffffff;check(render(black,p,out),"maximum counts and seed");p.blades=2;check(!valid(p),"invalid aperture");p=defaults(320,240);p.gamma=0;check(!valid(p),"zero gamma rejected");p=defaults(320,240);p.hx=std::numeric_limits<double>::quiet_NaN();check(!valid(p),"NaN rejected");
 p=defaults(320,240);out=solid(2,2,.7f);auto sentinel=out;int calls=0;check(!render(black,p,out,nullptr,[&]{return ++calls>3;})&&same(out,sentinel),"cancel never publishes partial image");
 auto native=solid(384,256),small=solid(192,128);p=defaults(384,256);p.ghosts=0;p.rays=0;p.streak_bright=p.ring_bright=0;p.hot_bright=0;check(render(native,p,ref)&&render(small,p,out,nullptr,{},RenderView{0,0,384,256,2}),"native and draft geometry");check(std::abs(energy(ref)-energy(out))<.0001,"draft keeps broad light energy");
 p=defaults(384,256);p.scale=1;p.blend=2;check(render(small,p,out,nullptr,{},RenderView{0,0,384,256,2}),"subpixel emitter remains finite");
 p=defaults(384,256);p.hot_bright=p.glow=p.ghost_bright=p.ring_bright=p.streak_bright=0;p.ray_bright=1;p.rays=12;p.rotation=13;p.blend=1;
 check(render(native,p,ref)&&render(small,p,out,nullptr,{},RenderView{0,0,384,256,2}),"thin rays at two resolutions");
 check(std::abs(energy(ref)-energy(out))/energy(ref)<.2,"draft thin-ray energy stays within 20 percent");
 for(auto size:{std::pair{1,1},std::pair{1,17},std::pair{31,1}}){auto tiny=solid(size.first,size.second);p=defaults(size.first,size.second);check(render(tiny,p,out)&&out.valid(),"tiny source");}
 std::ofstream raw(argc>1?argv[1]:"flare-deterministic.f32",std::ios::binary);auto medium=solid(513,257,.17f,.4f);
 for(int i=0;i<8;i++){p=preset(i,513,257);p.blend=i%3;p.occlusion=i==6?2:0;p.edge_width=25;p.center_radius=25;check(render(medium,p,out),"deterministic preset");raw.write(reinterpret_cast<const char*>(out.rgba.data()),std::streamsize(out.rgba.size()*sizeof(float)));}check(raw.good(),"float fixture write");
 // PPM is produced by the real kernel for targeted visual review.
 if(argc>2){auto demo=solid(640,400);for(int i=0;i<8;i++){p=preset(i,640,400);p.hx=200;p.hy=140;p.px=360;p.py=230;check(render(demo,p,out),"preview image");std::ofstream ppm(std::string(argv[2])+"/preset-"+std::to_string(i)+".ppm",std::ios::binary);ppm<<"P6\n640 400\n255\n";for(std::size_t n=0;n<out.rgba.size();n+=4)for(int k=0;k<3;k++){const auto b=char(std::lround(std::clamp(out.rgba[n+k],0.f,1.f)*255));ppm.write(&b,1);}}
 }
 const auto t=std::chrono::steady_clock::now();p=defaults(768,768);auto bench=solid(768,768);check(render(bench,p,out),"benchmark");const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();
 std::cout<<"PASS: "<<checks<<" assertions; 8 presets, independent elements, straight-alpha emission, RGB/HDR blending, coordinates, occlusion, native/draft geometry, cancellation; 768px render "<<ms<<"ms.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
