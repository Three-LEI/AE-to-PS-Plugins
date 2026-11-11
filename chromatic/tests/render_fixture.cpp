#include "chromatic_core.hpp"
#include <fstream>
#include <chrono>
#include <iostream>
int main(int argc,char** argv){
 if(argc<4)return 1;chromatic::Image src;std::ifstream input(argv[1],std::ios::binary);input.read(reinterpret_cast<char*>(&src.width),4);input.read(reinterpret_cast<char*>(&src.height),4);
 if(src.width<=0||src.height<=0||std::size_t(src.width)*src.height>16*1024*1024)return 2;
 src.rgba.resize(std::size_t(src.width)*src.height*4);input.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*4);if(!input)return 3;
 auto p=chromatic::preset(std::stoi(argv[3]));if(argc>4)p.amount=std::stod(argv[4]);if(argc>5)p.angle=std::stod(argv[5]);if(argc>6)p.samples=std::stoi(argv[6]);
 chromatic::Image out;const auto start=std::chrono::steady_clock::now();if(!chromatic::render(src,p,out))return 4;
 std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<const char*>(&out.width),4);output.write(reinterpret_cast<const char*>(&out.height),4);output.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*4);
 std::cout<<argv[2]<<" samples="<<p.samples<<" ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<'\n';return output?0:5;
}
