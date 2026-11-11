#include "surface_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
using namespace surface;
int checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
Image fixture(int w,int h){Image a{w,h,-11,17,std::vector<float>(std::size_t(w)*h*4)};for(int y=0;y<h;++y)for(int x=0;x<w;++x){const auto i=(std::size_t(y)*w+x)*4;a.rgba[i]=float(x)/std::max(1,w-1);a.rgba[i+1]=float(y)/std::max(1,h-1);a.rgba[i+2]=float(.5+.4*std::sin(x*.17)*std::cos(y*.12));a.rgba[i+3]=float((x+y)%5)/4;}return a;}
int main(int argc,char** argv){
 Image src=fixture(137,91),out,again;
 auto p=defaults(Kind::glass,137,91,-11,17);check(p.light_x==-11+137*.5&&p.light_y==17+91*.5&&p.ambient==50,"glass document defaults");auto blob=defaults(Kind::blob,137,91,-11,17);check(blob.light_x==-11+137*.25&&blob.ambient==75&&blob.specular==100,"blob document defaults");
 p.height=0;p.ambient=100;p.diffuse=p.specular=0;check(render(src,p,out)&&out.rgba==src.rgba,"neutral Glass exact HDR/hidden-RGB identity");
 Image flat=src;for(std::size_t i=0;i<flat.rgba.size();i+=4){flat.rgba[i]=flat.rgba[i+1]=flat.rgba[i+2]=.3f;flat.rgba[i+3]=1;}
 p=defaults(Kind::glass,137,91,-11,17);p.softness=0;p.height=0;p.displacement=0;p.ambient=0;p.diffuse=100;p.specular=0;p.light_height=50;
 check(render(flat,p,out),"flat distant light");check(std::abs(out.rgba[0]-.3/std::sqrt(2))<1e-6,"distant height normalization analytic oracle");
 p.light_height=-50;check(render(flat,p,out)&&out.rgba[0]==0,"negative light behind surface");
 p.light_height=100;p.diffuse=0;p.specular=100;p.metal=0;check(render(flat,p,out)&&std::abs(out.rgba[0]-1)<1e-6,"plastic reflects light color");p.metal=100;check(render(flat,p,out)&&out.rgba[0]==.3f,"metal tints specular by source");
 p.metal=0;p.light_color={0,1,0};check(render(flat,p,out)&&out.rgba[0]==0&&std::abs(out.rgba[1]-1)<1e-6&&out.rgba[2]==0,"RGB effect light");
 p.light_color={1,1,1};p.light_height=65;p.roughness=.1;check(render(flat,p,out),"broad specular");const double z=65/std::sqrt(65.*65+35.*35);check(std::abs(out.rgba[0]-std::pow(z,10))<1e-6,"roughness Phong exponent oracle");
 p=defaults(Kind::glass,137,91,-11,17);p.softness=0;p.displacement=0;p.ambient=0;p.diffuse=100;p.specular=0;p.direction=90;
 Image ramp=flat;for(int y=0;y<91;++y)for(int x=0;x<137;++x)for(int k=0;k<3;++k)ramp.rgba[(std::size_t(y)*137+x)*4+k]=float(.25+.5*x/136.);
 check(render(flat,p,out,&ramp),"gradient normal");const int x=68,y=45;const double gx=(ramp.rgba[(y*137+x)*4]-ramp.rgba[(y*137+x-1)*4])*25;const double expected=.3*(z-gx*35/std::sqrt(65.*65+35.*35))/std::sqrt(1+gx*gx);check(std::abs(out.rgba[(y*137+x)*4]-expected)<2e-6,"backward-difference normal oracle");
 p=defaults(Kind::glass,137,91,-11,17);p.ambient=100;p.diffuse=p.specular=0;p.softness=0;check(render(src,p,out,&ramp),"positive height warp");p.height=-25;check(render(src,p,again,&ramp)&&out.rgba==again.rgba,"height sign changes normals, retains squared displacement");
 Image red=flat;for(std::size_t i=0;i<red.rgba.size();i+=4){red.rgba[i]=1;red.rgba[i+1]=red.rgba[i+2]=0;}
 p=blob;p.softness=0;p.ambient=100;p.diffuse=p.specular=0;
 for(int channel=0;channel<6;++channel){p.property=channel;check(render(flat,p,out,&red),"blob property render");const bool empty=channel==1||channel==2;check((out.rgba[3]==0)==empty,"six blob map properties");}
 p.cut=100;check(render(src,p,out)&&std::all_of(out.rgba.begin(),out.rgba.end(),[](float v){return v==0;}),"Cut Away 100 empties frame");
 for(auto kind:{Kind::blob,Kind::glass})for(int channel=0;channel<6;++channel)for(int type=0;type<2;++type){p=defaults(kind,137,91,-11,17);p.property=channel;p.light_type=type;p.light_color={1,.4,.1};check(render(src,p,out)&&render(src,p,again)&&out.rgba==again.rgba,"both models deterministic across properties/lights");for(std::size_t i=0;i<out.rgba.size();++i){check(std::isfinite(out.rgba[i]),"finite output");if(i%4==3)check(out.rgba[i]>=0&&out.rgba[i]<=1,"valid alpha");}}
 p=defaults(Kind::glass,137,91,-11,17);check(render(src,p,out),"distant position ignored baseline");p.light_x=900;p.light_y=-900;check(render(src,p,again)&&out.rgba==again.rgba,"distant light ignores position");p.light_type=1;check(render(src,p,out),"point light baseline");p.direction=222;check(render(src,p,again)&&out.rgba==again.rgba,"point light ignores direction");
 Image hdr=src;for(std::size_t i=0;i<hdr.rgba.size();i+=4){hdr.rgba[i]=3;hdr.rgba[i+1]=-.25f;}p=defaults(Kind::glass,137,91,-11,17);p.height=0;p.ambient=150;p.diffuse=p.specular=0;check(render(hdr,p,out)&&out.rgba[4]>1&&out.rgba[5]<0,"native HDR is not clipped");
 for(auto size:{1,2,3}){auto tiny=fixture(size,size);p=defaults(Kind::glass,size,size);p.softness=500;p.height=100;p.displacement=-500;check(render(tiny,p,out)&&out.valid(),"tiny/extreme softness and warp");}
 p=defaults(Kind::glass,137,91);p.roughness=0;check(!render(src,p,out),"zero roughness rejected");p=defaults(Kind::glass,137,91);p.light_type=2;check(!render(src,p,out),"unsupported light type rejected");p=defaults(Kind::glass,137,91);p.softness=std::nan("");check(!render(src,p,out),"NaN parameter rejected");
 p=defaults(Kind::glass,137,91);Image sentinel=src;check(!render(src,p,sentinel,nullptr,[]{return true;})&&sentinel.rgba==src.rgba,"early cancel transactional");int calls=0;check(!render(src,p,sentinel,nullptr,[&]{return ++calls>4;})&&sentinel.rgba==src.rgba,"mid-render cancel transactional");
 auto alias=src;check(render(alias,p,alias)&&render(src,p,out)&&alias.rgba==out.rgba,"input/output alias");
 const std::filesystem::path folder=argc>1?argv[1]:"evidence/surface-v01/core";std::filesystem::create_directories(folder);std::ofstream raw(folder/"deterministic.f32",std::ios::binary);auto large=fixture(385,257);
 for(auto kind:{Kind::blob,Kind::glass})for(int type=0;type<2;++type)for(int channel=0;channel<6;++channel){p=defaults(kind,385,257,-11,17);p.property=channel;p.light_type=type;check(render(large,p,out),"parallel reference case");raw.write(reinterpret_cast<char*>(out.rgba.data()),out.rgba.size()*4);}
 auto bench=fixture(1024,768);p=defaults(Kind::glass,1024,768);const auto start=std::chrono::steady_clock::now();check(render(bench,p,out),"benchmark");std::cout<<"PASS: "<<checks<<" assertions; defaults; exact neutral; analytic light/normal/specular; Blob cut/properties; both effect lights; deterministic HDR/alpha; extreme sizes; guards; cancel; alias.\nBENCH: 1024x768 Glass ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<'\n';
}
