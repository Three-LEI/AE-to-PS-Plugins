#include "glow_core.hpp"
#include <fstream>
#include <iostream>
#include <chrono>
#include <cstring>
int main(int argc,char** argv){
 if(argc<5)return 2;
 glow::Image source;std::ifstream in(argv[1],std::ios::binary);
 in.read(reinterpret_cast<char*>(&source.width),4);in.read(reinterpret_cast<char*>(&source.height),4);
 if(source.width<=0||source.height<=0||std::size_t(source.width)*source.height>16*1024*1024)return 3;
 source.rgba.resize(std::size_t(source.width)*source.height*4);in.read(reinterpret_cast<char*>(source.rgba.data()),source.rgba.size()*4);if(!in)return 4;
 auto p=glow::defaults(glow::Kind::deep);p.radius=std::stod(argv[3]);p.blend=std::strcmp(argv[4],"add")==0?glow::Blend::add:glow::Blend::screen;
 if(argc>5)p.kind=glow::Kind::classic;
 glow::Image out;const auto start=std::chrono::steady_clock::now();if(!glow::render(source,p,out))return 5;
 std::ofstream file(argv[2],std::ios::binary);file.write(reinterpret_cast<const char*>(&out.width),4);file.write(reinterpret_cast<const char*>(&out.height),4);file.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*4);
 std::cout<<argv[2]<<" ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<'\n';return file?0:6;
}
