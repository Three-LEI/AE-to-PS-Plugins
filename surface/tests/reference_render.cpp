#include "surface_core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::filesystem::path folder=argv[1];std::ifstream list(folder/"cases.txt");int w,h,count;list>>w>>h>>count;
 for(int n=0;n<count;++n){std::string name,source,map;int kind;surface::Parameters p;list>>name>>source>>map>>kind; p=surface::defaults(surface::Kind(kind),w,h);
  list>>p.property>>p.softness>>p.cut>>p.height>>p.displacement>>p.light_type>>p.intensity>>p.light_color[0]>>p.light_color[1]>>p.light_color[2]>>p.light_height>>p.light_x>>p.light_y>>p.direction>>p.ambient>>p.diffuse>>p.specular>>p.roughness>>p.metal;
  if(!list){std::cerr<<"bad case "<<n;return 2;}
  auto read=[&](const std::string& stem){surface::Image a{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};std::ifstream f(folder/(stem+".f32"),std::ios::binary);f.read(reinterpret_cast<char*>(a.rgba.data()),a.rgba.size()*4);if(!f)throw std::runtime_error(stem);return a;};
  auto input=read(source),bump=read(map);surface::Image out;if(!surface::render(input,p,out,&bump)){std::cerr<<"failed "<<name;return 3;}
  std::ofstream f(folder/(name+".out.f32"),std::ios::binary);f.write(reinterpret_cast<char*>(out.rgba.data()),out.rgba.size()*4);
 }
 std::cout<<"PASS: rendered "<<count<<" same-parameter reference cases\n";
}
