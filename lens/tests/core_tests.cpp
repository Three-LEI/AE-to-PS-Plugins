#include "lens_core.hpp"
#include <iostream>
#include <cmath>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <cstdlib>
void check(bool value,const char* why){if(!value){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
int main(int argc,char** argv){
 if(argc==12){int w=std::atoi(argv[3]),h=std::atoi(argv[4]);lens::Image src{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)},out;std::ifstream input(std::filesystem::path(argv[1]),std::ios::binary);input.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*4);check(bool(input),"reference input");bool cc=std::atoi(argv[5]);auto p=lens::defaults(cc?lens::Kind::cc:lens::Kind::optics,w,h);if(cc){p.size=std::atof(argv[6]);p.convergence=std::atof(argv[7]);}else{p.fov=std::atof(argv[6]);p.reverse=std::atoi(argv[7]);}p.axis=std::atoi(argv[8]);p.optimal=std::atoi(argv[9]);p.cx=std::atof(argv[10]);p.cy=std::atof(argv[11]);check(lens::render(src,p,out),"reference render");std::ofstream output(std::filesystem::path(argv[2]),std::ios::binary);output.write(reinterpret_cast<char*>(out.rgba.data()),out.rgba.size()*4);return output?0:2;}
 lens::Image src{321,257,-13,7,std::vector<float>(321*257*4)},out;
 for(std::size_t i=0;i<src.rgba.size();i+=4){src.rgba[i]=float(i%71)/20;src.rgba[i+1]=.3f;src.rgba[i+2]=-.1f;src.rgba[i+3]=(i%5)?1:0;}
 auto p=lens::defaults(lens::Kind::optics,321,257,-13,7);check(lens::render(src,p,out)&&out.rgba==src.rgba,"FOV0 native HDR identity");
 for(auto kind:{lens::Kind::optics,lens::Kind::cc})for(int variant=0;variant<12;++variant){p=lens::defaults(kind,321,257,-13,7);p.fov=variant*15;p.size=variant*30;p.convergence=-200+variant*25;p.axis=variant%3;p.reverse=variant&1;p.optimal=variant&2;check(lens::render(src,p,out),"all controls");for(float v:out.rgba)check(std::isfinite(v),"finite output");lens::Image repeat;check(lens::render(src,p,repeat)&&repeat.rgba==out.rgba,"deterministic parallel output");}
 p=lens::defaults(lens::Kind::cc,321,257,-13,7);p.size=0;check(lens::render(src,p,out),"zero lens radius");for(float v:out.rgba)check(v==0,"zero lens is empty");
 check(!lens::render(src,p,out,[]{return true;}),"cancel");p.fov=180;check(!lens::render(src,p,out),"FOV guard");
 std::cout<<"PASS: identity/HDR/alpha, both models/controls, deterministic parallel output, zero size, cancellation and guards.\n";
}
