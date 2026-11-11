#include "roughen_core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <thread>
using namespace roughen;
std::size_t checks=0;void check(bool v,const char* why){++checks;if(!v){std::cerr<<"FAIL: "<<why<<'\n';std::exit(1);}}
Image fixture(int w,int h){Image a{w,h,-13,7,std::vector<float>(std::size_t(w)*h*4)};for(int y=0;y<h;y++)for(int x=0;x<w;x++){const auto i=(std::size_t(y)*w+x)*4;a.rgba[i]=float(x)/std::max(1,w-1)*2-.25f;a.rgba[i+1]=float(y)/std::max(1,h-1);a.rgba[i+2]=.2f;a.rgba[i+3]=(x>10&&y>10&&x<w-11&&y<h-11)?float(.3+.7*std::abs(std::sin(x*.08))):0;}return a;}
int main(int argc,char** argv){
 auto src=fixture(192,128);auto p=defaults();Image out,second;Prepared c;check(valid(p)&&p.offset_x==0&&p.offset_y==0,"AE defaults; offset is not document center");
 p.mix=0;check(render(src,p,out,&c)&&out.rgba==src.rgba,"zero mix exact HDR/hidden RGB identity");p.mix=100;
 check(!render(src,p,out,&c,[]{return true;})&&out.rgba==src.rgba,"cancel preserves previous output");p.scale=0;check(!render(src,p,out),"invalid parameter rejected");p=defaults();p.seed=std::numeric_limits<double>::infinity();check(!valid(p),"nonfinite rejected");p=defaults();
 std::set<std::vector<float>> modes;
 for(int mode=0;mode<8;mode++){p.type=mode;check(render(src,p,out,&c),"eight modes");modes.insert(out.rgba);for(float v:out.rgba)check(std::isfinite(v),"finite HDR");for(std::size_t i=3;i<out.rgba.size();i+=4)check(out.rgba[i]>=0&&out.rgba[i]<=1,"alpha bounded");if(mode==7)for(std::size_t i=3;i<src.rgba.size();i+=4)check(out.rgba[i]==src.rgba[i],"photocopy color retains original alpha");}
 check(modes.size()==8,"eight distinct modes");p=defaults();p.influence=0;p.type=4;check(render(src,p,out,&c),"smooth rust");for(std::size_t i=0;i<src.rgba.size();i++)if(src.rgba[(i/4)*4+3]>0)check(out.rgba[i]==src.rgba[i],"zero-influence rust preserves visible source");
 p=defaults();check(render(src,p,out),"alpha-only transfer");for(std::size_t i=0;i<src.rgba.size();i+=4)if(src.rgba[i+3]>0)for(int k=0;k<3;k++)check(std::abs(out.rgba[i+k]-src.rgba[i+k])<1e-6,"alpha-only modes preserve straight RGB even at erased pixels, including locked-alpha hosts");
 p=defaults();p.cycle=true;p.cycles=2;check(render(src,p,out,&c),"cycle begin");p.evolution=720;check(render(src,p,second,&c)&&out.rgba==second.rgba,"cycle period exact");p.evolution=180;check(render(src,p,second,&c)&&out.rgba!=second.rgba,"evolution changes pattern");p.evolution=0;p.seed=7;check(render(src,p,second,&c)&&out.rgba!=second.rgba,"seed changes pattern");check(render(src,p,out,&c)&&out.rgba==second.rgba,"deterministic seed");
 auto alias=src;check(render(alias,p,alias)&&alias.rgba==out.rgba,"in-place render");
 auto plain=src;for(std::size_t i=0;i<plain.rgba.size();i+=4){plain.rgba[i]=1;plain.rgba[i+1]=plain.rgba[i+2]=0;plain.rgba[i+3]=128.f/255;}
 p=defaults();p.influence=0;check(render(plain,p,out),"half-alpha transfer");check(std::abs(out.rgba[(64*192+96)*4+3]*255-67)<1,"measured AE half-alpha rough transfer");p.type=6;check(render(plain,p,out)&&std::abs(out.rgba[(64*192+96)*4+3]*255-86)<1,"measured AE photocopy transfer");
 p=defaults();int calls=0;const auto owner=std::this_thread::get_id();check(!render(src,p,out,nullptr,[&]{check(owner==std::this_thread::get_id(),"cancel on invoking thread");return ++calls>3;}),"mid-prepare cancellation");
 auto bad=src;bad.rgba[1]=std::numeric_limits<float>::quiet_NaN();check(!render(bad,p,out),"nonfinite input rejected");Image empty{1,1,0,0,{2,3,4,0}};check(render(empty,p,out)&&out.rgba==empty.rgba,"empty transparent hidden RGB safe");
 const std::filesystem::path folder=argc>1?argv[1]:"evidence/roughen-v01/core";std::filesystem::create_directories(folder);std::ofstream raw(folder/"deterministic.f32",std::ios::binary);
 for(int mode=0;mode<8;mode++)for(int n=0;n<3;n++){p=defaults();p.type=mode;p.seed=n;p.border=n*12.5;p.sharpness=n*.5;p.complexity=1+n*.5;p.stretch=(n-1)*85;check(render(src,p,out),"24 deterministic cases");raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*4);}raw.close();
 auto large=fixture(1024,768);p=defaults();auto start=std::chrono::steady_clock::now();check(render(large,p,out,&c),"large cold render");double cold=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();start=std::chrono::steady_clock::now();p.seed=1;check(render(large,p,out,&c),"cached parameter render");double warm=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"PASS: "<<checks<<" assertions; eight modes; measured alpha/photocopy; HDR/alpha; neutral/cycle/seed; cache; in-place; guards/cancel; 24 serial/parallel cases.\nBENCH 1024x768 cold="<<cold<<" ms; cached="<<warm<<" ms\n";
}
