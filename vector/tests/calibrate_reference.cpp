#include "vector_core.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace vectorblur;
Image read(const std::string& path){std::ifstream f(path,std::ios::binary);std::string header;int w,h,range;f>>header>>w>>h>>range;f.get();if(header!="P6"||range!=255)return {};Image image{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<image.rgba.size();i+=4){for(int k=0;k<3;++k)image.rgba[i+k]=float(std::uint8_t(f.get()))/255;image.rgba[i+3]=1;}return image;}
void write(const Image& image,const std::string& path){std::ofstream f(path,std::ios::binary);f<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";for(std::size_t i=0;i<image.rgba.size();i+=4)for(int k=0;k<3;++k)f.put(char(std::lround(std::clamp(image.rgba[i+k]*image.rgba[i+3],0.f,1.f)*255)));}
int main(int argc,char** argv){if(argc<3)return 1;const std::string folder=argv[2];const auto source=read(argv[1]);if(!source.valid())return 2;std::filesystem::create_directories(folder);
 if(argc>3){Parameters p;p.amount=std::atof(argv[3]);if(argc>4)p.type=Type(std::atoi(argv[4]));if(argc>5){p.ridge=std::atof(argv[5]);if(int(p.type)>=3)p.revolutions=std::atof(argv[5]);}if(argc>6)p.map_softness=std::atof(argv[6]);if(argc>7)p.follow=std::atoi(argv[7]);if(argc>8)p.angle=std::atof(argv[8]);p.quality=3;Image out,map;if(argc>9)map=read(argv[9]);if(!render(source,p,out,map.valid()?&map:nullptr))return 3;write(out,folder+"/result.ppm");return 0;}
 for(bool follow:{false,true})for(double softness:{0.,2.,4.,8.,15.})for(double ridge:{.05,.1,.25,.5,1.}){
  Parameters p;p.amount=50;p.map_softness=softness;p.ridge=ridge;p.follow=follow;p.quality=2;Image out;if(!render(source,p,out))return 3;
  const std::string key="f"+std::to_string(int(follow))+"_s"+std::to_string(int(softness))+"_r"+std::to_string(int(ridge*100));write(out,folder+'/'+key+".ppm");
 }
 std::cout<<"Rendered 50 calibration candidates\n";
}
