#include "vector_core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
using namespace vectorblur;
void ppm(const Image& image,const std::string& path){std::ofstream file(path,std::ios::binary);file<<"P6\n"<<image.width<<' '<<image.height<<"\n255\n";
 for(std::size_t i=0;i<image.rgba.size();i+=4)for(int k=0;k<3;++k){const float v=image.rgba[i+k]*image.rgba[i+3];file.put(char(std::lround(std::clamp(v,0.f,1.f)*255)));}}
void fixture(int w,int h,Image& source,Image& map){source={w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};map=source;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){const auto i=(std::size_t(y)*w+x)*4;const float nx=float(x)/w,ny=float(y)/h;
  const float radius=std::hypot(nx-.5f,ny-.5f);const bool ring=radius>.15f&&radius<.3f&&int(radius*w/5)%3==0;
  const bool mark=(std::abs(nx-.24f)<.007f&&ny>.25f&&ny<.75f)||(std::abs(ny-.5f)<.008f&&nx>.12f&&nx<.36f);
  const bool dots=std::hypot(nx-.72f,ny-.32f)<.024f||std::hypot(nx-.66f,ny-.7f)<.018f;
  source.rgba[i]=ring?.3f:mark?1.f:dots?.15f:.018f;source.rgba[i+1]=ring?.7f:mark?.35f:dots?.6f:.028f;source.rgba[i+2]=ring?1.f:mark?.12f:dots?1.f:.046f;source.rgba[i+3]=1;
  const float field=.5f+.2f*std::sin(nx*10)+.15f*std::cos(ny*9)+.1f*std::sin((nx+ny)*16);
  map.rgba[i]=map.rgba[i+1]=map.rgba[i+2]=field;map.rgba[i+3]=1;
 }}
int main(int argc,char** argv){const std::string folder=argc>1?argv[1]:".";Image source,map,out;fixture(480,320,source,map);ppm(source,folder+"/vector-source.ppm");ppm(map,folder+"/vector-map.ppm");
 for(int t=0;t<5;++t){Parameters p;p.type=Type(t);p.amount=90;p.map_softness=5;p.ridge=.5;p.revolutions=2;p.quality=3;p.edge=Edge::repeat;render(source,p,out,&map);ppm(out,folder+"/vector-mode-"+std::to_string(t)+".ppm");}
 Parameters p=preset(2);p.map_softness=5;p.edge=Edge::repeat;p.quality=2;p.follow=true;render(source,p,out,&map);ppm(out,folder+"/vector-curved.ppm");p.follow=false;render(source,p,out,&map);ppm(out,folder+"/vector-straight.ppm");
 fixture(1920,1080,source,map);p=Parameters{};p.type=Type::direction_center;p.amount=80;p.map_softness=15;p.quality=1;p.follow=true;
 const auto start=std::chrono::steady_clock::now();if(!render(source,p,out,&map))return 1;const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"BENCH: 1920x1080 RGBA float, standard, curved, amount80, including map build, ms="<<ms<<'\n';
 std::ofstream metadata(folder+"/vector-benchmark.json");metadata<<"{\"width\":1920,\"height\":1080,\"quality\":\"standard\",\"amount\":80,\"curved\":true,\"threads_max\":8,\"single_run_ms\":"<<ms<<"}\n";
}
