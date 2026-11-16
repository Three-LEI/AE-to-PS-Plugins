#include "chromatic_core.hpp"
#include "chromatic_legacy.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <cstdlib>
namespace {
using namespace chromatic;int assertions=0;void require(bool v,const char* message){++assertions;if(!v){std::cerr<<"FAIL extended: "<<message<<'\n';std::exit(1);}}
Image image(int w,int h,float alpha=0){Image im{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=3;i<im.rgba.size();i+=4)im.rgba[i]=alpha;return im;}
double value(const Image& im,int x,int y,int k=0){auto i=(std::size_t(y)*im.width+x)*4;return im.rgba[i+k]*im.rgba[i+3];}
void near(double a,double b,const char* why){require(std::abs(a-b)<2e-5,why);}
std::array<double,3> energy(const Image& im){std::array<double,3> sum{};for(std::size_t i=0;i<im.rgba.size();i+=4)for(int k=0;k<3;k++)sum[k]+=im.rgba[i+k]*im.rgba[i+3];return sum;}
std::array<double,2> moment(const Image& im){std::array<double,2> sum{};for(int y=0;y<im.height;y++)for(int x=0;x<im.width;x++){const double e=value(im,x,y),dx=x-im.width/2,dy=y-im.height/2;sum[0]+=e*dx*dx;sum[1]+=e*dy*dy;}return sum;}
}
void extended_tests(){using namespace chromatic;
 auto src=image(129,129);for(int k=0;k<4;k++)src.rgba[(64*129+64)*4+k]=1;Image out,again;
 auto p=preset(4);p.warp_amount=0;require(render(src,p,out)&&out.rgba==src.rgba,"zero warp exact identity");p=preset(4);p.warp={Transform{},Transform{}};require(render(src,p,out)&&out.rgba==src.rgba,"neutral endpoint transforms exact identity");
 auto gradient=image(33,33,1);for(int y=0;y<33;y++)for(int x=0;x<33;x++){auto i=(y*33+x)*4;gradient.rgba[i]=x/32.f;gradient.rgba[i+1]=y/32.f;gradient.rgba[i+2]=(x+y)/64.f;}
 p=preset(4);p.warp={Transform{100,0,2,-1},Transform{100,0,2,-1}};p.linear_light=false;p.edge=Edge::repeat;
 require(render(gradient,p,out),"equal endpoint transform");for(int k=0;k<3;k++)near(value(out,10,10,k),value(gradient,8,11,k),"all bands follow identical transform");
 auto mask=image(3,2,.5);p.mask_mode=0;require(render(gradient,p,out,&mask),"gray amplitude mask");for(int k=0;k<3;k++)near(value(out,10,10,k),(value(gradient,9,10,k)+value(gradient,9,11,k))*.5,"mask scales transform coordinates continuously");
 p.warp={Transform{100,-20,0,0},Transform{100,25,0,0}};require(render(gradient,p,out),"spectral rotation");for(int k=0;k<3;k++)near(value(out,16,16,k),value(gradient,16,16,k),"rotation center stays fixed");
 p=preset(4);p.warp={Transform{100,0,10,0},Transform{100,0,-10,0}};p.samples=64;p.linear_light=false;require(render(src,p,out),"spectral endpoint split");double positions[3]{},weights[3]{};int visible=0;for(int x=0;x<129;x++){double total=0;for(int k=0;k<3;k++){double e=value(out,x,64,k);positions[k]+=x*e;weights[k]+=e;total+=e;}visible+=total>.001;}require(positions[0]/weights[0]>positions[1]/weights[1]&&positions[1]/weights[1]>positions[2]/weights[2],"red/green/blue endpoint order");require(visible>12,"more than three spectral copies");
 p=preset(6);p.defocus_radius=24;p.defocus_radii={100,100,100};p.ring_width=15;p.defocus_quality=2;p.linear_light=false;
 require(render(src,p,out),"true aperture ring");near(value(out,64,64),0,"ring has an empty center");require(value(out,87,64)>.0002,"ring carries energy at its radius");auto sum=energy(out);for(double e:sum)require(std::abs(e-1)<.003,"aperture conserves impulse energy");
 for(int d=0;d<27;d++){near(value(out,64+d,64),value(out,64-d,64),"ring horizontal symmetry");near(value(out,64,64+d),value(out,64,64-d),"ring vertical symmetry");}
 p.ring_width=100;require(render(src,p,out)&&value(out,64,64)>.0002&&value(out,84,64)>.0002,"filled circular aperture");p.aperture_shape=2;require(render(src,p,again)&&value(again,64,64)>.0002&&value(again,84,64)==0,"square aperture is not a Gaussian circle");
 p.aperture_shape=0;p.ring_width=20;p.defocus_radii={150,100,50};require(render(src,p,out),"different RGB ring radii");double radial[3]{},total[3]{};for(int y=0;y<129;y++)for(int x=0;x<129;x++)for(int k=0;k<3;k++){double e=value(out,x,y,k);radial[k]+=std::hypot(x-64.,y-64.)*e;total[k]+=e;}require(radial[0]/total[0]>radial[1]/total[1]+8&&radial[1]/total[1]>radial[2]/total[2]+8,"RGB rings have distinct measured radii");
 p.defocus_radii={100,100,100};p.aperture_aspect=4;require(render(src,p,out),"elliptical aperture");auto m=moment(out);require(m[0]>m[1]*8,"aspect stretches aperture horizontally");p.aperture_rotation=90;require(render(src,p,out),"rotated ellipse");m=moment(out);require(m[1]>m[0]*8,"aperture rotation changes its orientation");
 auto strip=image(1,257);for(int k=0;k<4;k++)strip.rgba[128*4+k]=1;p=preset(6);p.defocus_radius=24;p.defocus_radii={100,100,100};p.aperture_aspect=9;p.ring_width=100;p.defocus_quality=2;p.linear_light=false;require(render(strip,p,out),"narrow native-coordinate ellipse");auto short_m=moment(out);p.aperture_rotation=90;require(render(strip,p,again),"narrow rotated ellipse");auto long_m=moment(again);double short_e=energy(out)[0],long_e=energy(again)[0];require((long_m[1]/long_e)/(short_m[1]/short_e)>25&&(long_m[1]/long_e)/(short_m[1]/short_e)<160,"rotation is applied before nonuniform proxy scaling");near(value(again,0,218),0,"narrow rotated ellipse keeps its native footprint");
 auto flat=image(31,19,.4);for(std::size_t i=0;i<flat.rgba.size();i+=4){flat.rgba[i]=.2f;flat.rgba[i+1]=.7f;flat.rgba[i+2]=1.6f;}
 for(auto mode:{Mode::warp,Mode::defocus})for(bool linear:{false,true}){
  p=preset(mode==Mode::warp?5:7);p.edge=Edge::repeat;p.linear_light=linear;require(render(flat,p,out),"constant HDR/alpha field");for(std::size_t i=0;i<flat.rgba.size();i++)near(out.rgba[i],flat.rgba[i],"white balance/HDR/alpha unchanged on constant field");
  p=preset(mode==Mode::warp?4:6);p.linear_light=linear;auto black=src;for(std::size_t i=3;i<black.rgba.size();i+=4)black.rgba[i]=1;require(render(src,p,out)&&render(black,p,again),"black and transparent reference");for(std::size_t i=0;i<out.rgba.size();i+=4)for(int k=0;k<3;k++)near(out.rgba[i+k]*out.rgba[i+3],again.rgba[i+k],"associated interpolation has no transparent dark halo");
  p.mix=0;require(render(flat,p,out)&&out.rgba==flat.rgba,"new modes zero mix exact");p.mix=100;p.alpha=Alpha::preserve;require(render(src,p,out),"source alpha mode");for(std::size_t i=3;i<src.rgba.size();i+=4)near(out.rgba[i],src.rgba[i],"source alpha preserved exactly");
  p.alpha=Alpha::black;require(render(src,p,out),"black alpha mode");for(std::size_t i=3;i<out.rgba.size();i+=4)near(out.rgba[i],1,"opaque on black");
  p.alpha=Alpha::expand;auto empty=image(7,5);empty.rgba[0]=99;require(render(empty,p,out),"hidden RGB");for(float v:out.rgba)near(v,0,"hidden RGB does not leak");
  out=flat;int polls=0;require(!render(src,p,out,nullptr,[&]{return ++polls>4;})&&out.rgba==flat.rgba,"cancel is atomic");
 }
 for(int shape=0;shape<7;shape++){p=preset(6);p.aperture_shape=shape;p.aperture_roundness=35;p.defocus_radius=1024;p.aperture_aspect=10;auto tiny=image(3,2,1);require(render(tiny,p,out),"large radius/polygon/tiny dimensions");for(float v:out.rgba)require(std::isfinite(v),"extreme aperture stays finite");}
 for(double radius:{.05,.1,.25,.4,.8})for(double aspect:{.1,10.})for(int shape:{0,1,6}){p=preset(6);p.defocus_radius=radius;p.aperture_aspect=aspect;p.aperture_shape=shape;p.ring_width=1;require(render(gradient,p,out),"valid subpixel/skinny apertures never fail");for(float v:out.rgba)require(std::isfinite(v),"subpixel ring remains finite");}
 p=preset(6);p.defocus_radius=0;require(render(src,p,out)&&out.rgba==src.rgba,"zero defocus exact identity");p.aperture_aspect=std::numeric_limits<double>::quiet_NaN();require(!valid(p),"new nonfinite field rejected");p=preset(4);p.warp[0].scale=0;require(!valid(p),"singular endpoint rejected");
 SavedV1 old;old.values.amount=77;old.values.channels[2].rotation=13;old.values.mask_key=0x1234567890123456ULL;Parameters restored;require(upgrade_parameters(&old,sizeof(old),restored),"0.8 raw handle upgrades");ParametersV1 legacy_values;std::memcpy(&legacy_values,&restored,sizeof(legacy_values));require(legacy_values==old.values,"all legacy parameter values retained");require(restored.warp==Parameters{}.warp&&restored.defocus_radius==24,"new fields get stable defaults");const auto kept=restored;old.magic=0;require(!upgrade_parameters(&old,sizeof(old),restored)&&restored==kept,"wrong identity rejected without mutation");old.magic=0x5A434431;old.values.mode=Mode::warp;require(!upgrade_parameters(&old,sizeof(old),restored),"invalid legacy mode rejected");require(!upgrade_parameters(nullptr,sizeof(old),restored)&&!upgrade_parameters(&old,1,restored),"null/truncated legacy handle rejected");
 std::cout<<"PASS extended: "<<assertions<<" assertions; continuous endpoint transforms; true aperture holes/polygons/rotation; energy and RGB radii; HDR/alpha; cancel; legacy raw upgrade.\n";
}
