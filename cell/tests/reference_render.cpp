// Minimal float-RGBA exchange for comparison against saved AE reference pixels.
#include "cell_core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char** argv){
 if(argc!=7){std::cerr<<"mode contrast width height source.f32 output.f32\n";return 2;}
 const int width=std::stoi(argv[3]),height=std::stoi(argv[4]);
 auto p=cell::defaults(width,height);p.mode=cell::Mode(std::stoi(argv[1]));p.contrast=std::stod(argv[2]);
 cell::Image source{width,height,0,0,std::vector<float>(std::size_t(width)*height*4)},out;
 std::ifstream input(std::filesystem::path(argv[5]),std::ios::binary);
 input.read(reinterpret_cast<char*>(source.rgba.data()),std::streamsize(source.rgba.size()*sizeof(float)));
 if(!input||!cell::render(p,{},width,height,out,{&source})){std::cerr<<"Reference render failed\n";return 1;}
 std::ofstream output(std::filesystem::path(argv[6]),std::ios::binary);
 output.write(reinterpret_cast<const char*>(out.rgba.data()),std::streamsize(out.rgba.size()*sizeof(float)));
 return output?0:1;
}
