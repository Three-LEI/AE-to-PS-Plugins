#include "pattern_core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace patterns;
void save(const std::filesystem::path& folder,const std::string& name,Kind kind,Parameters p,int w=128,int h=96){
 std::vector<float> source(std::size_t(w)*h*4);for(std::size_t i=0;i<source.size();i+=4){source[i]=.2f;source[i+1]=.4f;source[i+2]=.6f;source[i+3]=1;}
 Image out;if(!render(kind,p,{0,0,1},w,h,out,&source))throw std::runtime_error("render failed");
 std::ofstream file(folder/(name+".rgba"),std::ios::binary);file.write(reinterpret_cast<const char*>(&w),4);file.write(reinterpret_cast<const char*>(&h),4);file.write(reinterpret_cast<const char*>(out.rgba.data()),std::streamsize(out.rgba.size()*4));
}
int main(int argc,char** argv){const std::filesystem::path folder=argc>1?argv[1]:"evidence/patterns";std::filesystem::create_directories(folder);
 for(Kind kind:{Kind::grid,Kind::checker}){
  const std::string n=kind==Kind::grid?"grid":"checker";auto p=defaults(kind,256,192);save(folder,n+"-default",kind,p,256,192);
  save(folder,"host-"+n+"-expected",kind,defaults(kind,128,96));
  p=defaults(kind,128,96);p.size_from=SizeFrom::width_height;p.width=32;p.height=24;p.corner_x=96;p.corner_y=72;
  for(int i=0;i<4;++i){const int fx[]={0,2,6,20},fy[]={0,2,4,20};p.feather_x=fx[i];p.feather_y=fy[i];save(folder,n+"-feather"+std::to_string(i),kind,p);}
  p.feather_x=p.feather_y=0;
  if(kind==Kind::grid){for(int border=0;border<=6;++border){p.border=border;save(folder,n+"-border"+std::to_string(border),kind,p);}p.border=5;}
  p.anchor_x=64.25;p.anchor_y=48.75;save(folder,n+"-fractional",kind,p);p.anchor_x=64;p.anchor_y=48;
  p.size_from=SizeFrom::corner;save(folder,n+"-corner",kind,p);p.size_from=SizeFrom::width;save(folder,n+"-square",kind,p);p.size_from=SizeFrom::width_height;
  p.opacity=40;save(folder,n+"-opacity40",kind,p);p.opacity=100;p.color[0]=.8;p.color[1]=.15;p.color[2]=.6;
  const std::pair<Blend,int> modes[]={{Blend::none,1},{Blend::normal,2},{Blend::stencil_alpha,3},{Blend::add,5},{Blend::multiply,6},{Blend::screen,7},{Blend::overlay,8},{Blend::soft_light,9},{Blend::hard_light,10},{Blend::color_dodge,12},{Blend::color_burn,13},{Blend::darken,15},{Blend::lighten,16},{Blend::difference,17},{Blend::exclusion,18},{Blend::hue,20},{Blend::saturation,21},{Blend::color,22},{Blend::luminosity,23}};
  for(const auto [mode,index]:modes){p.blend=mode;save(folder,n+"-blend"+std::to_string(index),kind,p);}
 }
 std::cout<<"PASS: rendered both generators, size modes, feather, border, fractional anchor, opacity and 19 blend references.\n";
}
