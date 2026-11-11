#include "creative_core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
int main(int argc,char** argv){
 if(argc!=2)return 1;std::filesystem::path folder=argv[1];std::ifstream list(folder/"cases.txt");int w,h,count;list>>w>>h>>count;
 for(int n=0;n<count;n++){
  std::string name,source;int kind;list>>name>>source>>kind;auto p=creative::defaults(creative::Kind(kind),w,h);
  list>>p.cx>>p.cy>>p.amount>>p.size>>p.complexity>>p.evolution>>p.cycles>>p.seed>>p.horizontal>>p.vertical>>p.mix>>p.direction>>p.width>>p.intensity>>p.edge_intensity>>p.thickness>>p.rotation;
  list>>p.type>>p.pinning>>p.edge>>p.shape>>p.reception>>p.mirroring>>p.quality>>p.cycle>>p.floating>>p.color[0]>>p.color[1]>>p.color[2];if(!list)return 2;
  creative::Image image{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)},out;std::ifstream in(folder/(source+".f32"),std::ios::binary);in.read(reinterpret_cast<char*>(image.rgba.data()),image.rgba.size()*4);if(!in||!creative::render(image,p,out)){std::cerr<<name<<'\n';return 3;}
  std::ofstream f(folder/(name+".out.f32"),std::ios::binary);f.write(reinterpret_cast<char*>(out.rgba.data()),out.rgba.size()*4);
 }std::cout<<"PASS: rendered "<<count<<" native same-parameter reference cases\n";
}
