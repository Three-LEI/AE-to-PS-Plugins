#include "jpeg_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <chrono>
using namespace jpegfx;
int checks=0;void check(bool v,const char* why){++checks;if(!v){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
Image fixture(int w,int h){Image a{w,h,-7,13,std::vector<float>(std::size_t(w)*h*4)};for(int y=0;y<h;++y)for(int x=0;x<w;++x){auto n=(std::size_t(y)*w+x)*4;a.rgba[n]=float((x*19+y*3)%101)/100;a.rgba[n+1]=float((x*3+y*17)%97)/96;a.rgba[n+2]=float((x^y)%31)/30;a.rgba[n+3]=float((x+y)%5)/4;}return a;}
double mse(const Image& a,const Image& b){double sum=0;for(std::size_t i=0;i<a.rgba.size();++i)if(i%4!=3)sum+=std::pow(a.rgba[i]-b.rgba[i],2);return sum/(a.width*a.height*3);}
void ppm(const std::filesystem::path& path,const Image& a){std::ofstream f(path,std::ios::binary);f<<"P6\n"<<a.width<<' '<<a.height<<"\n255\n";for(std::size_t i=0;i<a.rgba.size();i+=4)for(int k=0;k<3;++k){const auto q=static_cast<unsigned char>(std::lround(std::clamp(a.rgba[i+k],0.f,1.f)*255));f.write(reinterpret_cast<const char*>(&q),1);}}
int main(int argc,char** argv){
 Image src=fixture(137,91),out,again;Parameters p;
 p.mix=0;check(render(src,p,out)&&out.rgba==src.rgba,"zero mix is exact identity");
 p={};p.quality=100;p.chroma=0;check(render(src,p,out)&&out.rgba==src.rgba,"neutral 100 percent / 444 is exact identity");
 p={};p.chroma=0;p.quality=10;check(render(src,p,out),"low quality render");const auto low=mse(src,out);p.quality=80;check(render(src,p,again)&&mse(src,again)<low,"quality reduces compression loss");
 // Analytic DC oracle: an orthonormal constant block has only 8*(value-.5)*255.
 Image constant{8,8,0,0,std::vector<float>(8*8*4,1)};for(std::size_t i=0;i<constant.rgba.size();i+=4)for(int k=0;k<3;++k)constant.rgba[i+k]=.371f;
 p={};p.chroma=0;p.quality=10;check(render(constant,p,out),"DC block render");const double value=double(.371f),q=16*5,expected=std::round((value-.5)*255*8/q)*q/(255*8)+.5;
 for(std::size_t i=0;i<out.rgba.size();i+=4)check(std::abs(out.rgba[i]-expected)<2e-6,"DC normalization/quantization oracle");
 // A single known DCT basis coefficient: k=(u=1,v=0), amplitude=25.
 Image wave=constant;for(int y=0;y<8;++y)for(int x=0;x<8;++x)for(int k=0;k<3;++k)wave.rgba[(y*8+x)*4+k]=float(.5+25/255.*std::sqrt(.25)*std::cos((2*x+1)*std::numbers::pi/16)*std::sqrt(.125));
 p.quality=50;check(render(wave,p,out),"single basis render");for(int y=0;y<8;++y)for(int x=0;x<8;++x){const double target=.5+22/255.*std::sqrt(.25)*std::cos((2*x+1)*std::numbers::pi/16)*std::sqrt(.125);check(std::abs(out.rgba[(y*8+x)*4]-target)<2e-6,"AC quantization / inverse DCT oracle");}
 for(int mode=0;mode<6;++mode){p=preset(2);p.error_mode=mode;p.density=100;p.error_amp=2;check(render(src,p,out)&&render(src,p,again)&&out.rgba==again.rgba,"seed is deterministic in every mode");
  for(std::size_t i=0;i<out.rgba.size();++i){check(std::isfinite(out.rgba[i]),"finite output");if(i%4==3)check(out.rgba[i]==src.rgba[i],"alpha preserved exactly");}
  ++p.variation;check(render(src,p,again)&&out.rgba!=again.rgba,"variation changes damage");
 }
 p=preset(2);p.error_amp=0;check(render(src,p,out),"zero amplitude");p.error_rate=0;check(render(src,p,again)&&out.rgba==again.rgba,"zero amplitude bypasses all corruption");
 p=preset(2);p.density=0;check(render(src,p,out),"zero density");p.error_rate=0;check(render(src,p,again)&&out.rgba==again.rgba,"zero density bypasses corruption");
 p={};p.affect_luma=p.affect_chroma=0;p.all_freq=0;p.x_freq=4;p.y_freq=2;p.low_freq=5;p.mid_freq=3;p.high_freq=6;check(render(src,p,out),"frequency influence off");p.all_freq=p.x_freq=p.y_freq=p.low_freq=p.mid_freq=p.high_freq=1;check(render(src,p,again)&&out.rgba==again.rgba,"zero luma/chroma influence bypasses frequency gains");
 p={};p.saturation=0;check(render(src,p,out),"monochrome adjustment");for(std::size_t i=0;i<out.rgba.size();i+=4)check(out.rgba[i]==out.rgba[i+1]&&out.rgba[i]==out.rgba[i+2],"zero saturation is gray");
 Image black=src,white=src;for(std::size_t i=0;i<black.rgba.size();i+=4){for(int k=0;k<3;++k){black.rgba[i+k]=0;white.rgba[i+k]=1;}black.rgba[i+3]=white.rgba[i+3]=1;}
 p=preset(2);p.mask_blur=0;check(render(src,p,out,&black)&&out.rgba==src.rgba,"black mask bypasses effect");check(render(src,p,out,&white)&&render(src,p,again)&&out.rgba==again.rgba,"white mask equals unmasked output");p.invert_mask=true;check(render(src,p,out,&white)&&out.rgba==src.rgba,"mask inversion");
 p=preset(2);p.mask_mode=1;p.mask_blur=5;check(render(src,p,out,&src),"alpha mask with blur");
 Image hdr=src;for(std::size_t i=0;i<hdr.rgba.size();i+=4){hdr.rgba[i]=3;hdr.rgba[i+1]=-.25f;}p={};check(render(hdr,p,out),"HDR input accepted");p.preserve_hdr=false;check(render(hdr,p,again),"bounded base accepted");for(std::size_t i=0;i<hdr.rgba.size();i+=4){check(std::abs(out.rgba[i]-again.rgba[i]-2)<1e-6,"positive HDR residual");check(std::abs(out.rgba[i+1]-again.rgba[i+1]+.25)<1e-6,"negative HDR residual");}
 for(int chroma=0;chroma<4;++chroma)for(double factor:{1.,3.,64.}){p=preset(3);p.chroma=chroma;p.resolution=factor;p.aspect=.125;check(render(src,p,out)&&out.valid()&&out.left==src.left&&out.top==src.top,"odd dimensions/extreme downsampling/origin");}
 p={};p.quality=0;check(!render(src,p,out),"invalid quality rejected");p={};p.mask_blur=std::nan("");check(!render(src,p,out),"NaN rejected");p={};p.error_mode=99;check(!render(src,p,out),"invalid enum rejected");
 p={};Image sentinel=src;check(!render(src,p,sentinel,nullptr,[]{return true;})&&sentinel.rgba==src.rgba,"pre-cancel is transactional");int calls=0;check(!render(src,p,sentinel,nullptr,[&]{return ++calls>3;})&&sentinel.rgba==src.rgba,"mid-render cancel is transactional");
 Image alias=src;check(render(alias,p,alias)&&render(src,p,out)&&alias.rgba==out.rgba,"in-place alias supported");
 const std::filesystem::path folder=argc>1?argv[1]:"evidence/jpeg-v01/renders";std::filesystem::create_directories(folder);
 Image large=fixture(385,257);for(int y=0;y<large.height;++y)for(int x=0;x<large.width;++x){const auto n=(std::size_t(y)*large.width+x)*4;large.rgba[n]=float(x)/384;large.rgba[n+1]=float(y)/256;large.rgba[n+2]=float(.5+.45*std::sin(x*.11)*std::cos(y*.07));large.rgba[n+3]=1;}
 ppm(folder/"source.ppm",large);std::ofstream raw(folder/"deterministic.f32",std::ios::binary);
 for(int i=0;i<6;++i){p=preset(i);check(render(large,p,out),"preset render");ppm(folder/("preset-"+std::to_string(i)+".ppm"),out);raw.write(reinterpret_cast<char*>(out.rgba.data()),out.rgba.size()*sizeof(float));}
 Image bench=fixture(1024,768);p=preset(2);const auto start=std::chrono::steady_clock::now();check(render(bench,p,out),"benchmark");const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"PASS: "<<checks<<" assertions; analytic DC/AC oracles; identity/quality; six corruption modes; determinism; alpha/HDR; mask/invert/blur; odd edges; guards; transactional cancellation; presets.\nBENCH: 1024x768 damage, milliseconds="<<ms<<'\n';
}
