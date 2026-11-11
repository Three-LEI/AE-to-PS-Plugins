#include "roughen_core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char** argv){if(argc!=2)return 2;std::ifstream jobs(argv[1]);std::string in,out;int w,h;while(jobs>>in>>out>>w>>h){roughen::Parameters p;int cycle;jobs>>p.type>>p.border>>p.sharpness>>p.influence>>p.scale>>p.stretch>>p.offset_x>>p.offset_y>>p.complexity>>p.evolution>>cycle>>p.cycles>>p.seed>>p.color[0]>>p.color[1]>>p.color[2];p.cycle=cycle!=0;roughen::Image src{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)},result;std::ifstream f(in,std::ios::binary);if(!f.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*4)||!roughen::render(src,p,result))return 3;std::ofstream g(out,std::ios::binary);g.write(reinterpret_cast<const char*>(result.rgba.data()),result.rgba.size()*4);if(!g)return 4;}std::cout<<"PASS: AE reference fixture renders\n";}
