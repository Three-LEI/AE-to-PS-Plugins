#include "blinds_core.hpp"
#include "psui/adaptive_preview.hpp"
#include "../blinds/src/preview_renderer.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <stdexcept>
void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(int argc,char** argv){try{
 psui::DraftResolution budget;check(budget.side()==512,"balanced start");budget.observe(80,512);check(budget.side()==384,"slow frame lowers work");budget.observe(1,512);check(budget.side()==384,"stale timing ignored");for(int i=0;i<4;i++)budget.observe(2,384);check(budget.side()==512,"sustained fast work increases detail");for(int i=0;i<4;i++)budget.observe(2,512);check(budget.side()==768,"clearer upper bound");for(int i=0;i<12;i++)budget.observe(80,budget.side());check(budget.side()==256,"bounded fallback");
 blinds::Image pair{2,1,0,0,{1,0,0,0,0,0,2,1}};auto sample=psui::preview_pixel(pair,.5,0,{0,0,1});check(sample[0]==0&&sample[2]==2&&sample[3]==.5,"premultiplied interpolation preserves HDR without transparency fringes");
 pair.rgba={1,0,0,1,0,0,1,1};sample=psui::preview_pixel(pair,3.5,0,{0,0,8});check(sample[0]==1&&sample[2]==0,"reduced area centres align");sample=psui::preview_pixel(pair,7.5,0,{0,0,8});check(sample[0]==.5&&sample[2]==.5,"smooth interpolation between reduced cells");
 blinds::Image source{2048,512,-17,31,std::vector<float>(2048*512*4)};
 for(int y=0;y<512;y++)for(int x=0;x<2048;x++){const auto i=(std::size_t(y)*2048+x)*4;source.rgba[i]=(x%2)?1.f:.04f;source.rgba[i+1]=(y%3)?0.15f:.9f;source.rgba[i+2]=float((x+y)%17)/16;source.rgba[i+3]=(x%7==0||y%31<2)?0.f:1.f;}
 double total_old=0,total_new=0,maximum_ms=0;std::ofstream report;if(argc>1)report.open(argv[1]);if(report)report<<"case,old_premultiplied_mae,new_premultiplied_mae,draft_ms\n";
 for(int index=0;index<6;index++){
  auto p=blinds::defaults(2048,512,-17,31);p.completion=37;p.direction=index%3==0?0:index%3==1?30:90;p.width=index<3?20:7.5;p.feather=index%3==0?0:index%3==1?5:2;
  blinds::Image fine;check(blinds::render(source,p,fine),"native reference");
  psui::DraftImageCache<blinds::Image> cache;auto old_space=psui::preview_space(source,psui::PreviewPass{true,256});const auto& small=cache.get(source,old_space,[]{return false;});auto q=p;q.cx=old_space.x(p.cx);q.cy=old_space.y(p.cy);q.width=p.width/old_space.step;q.feather=p.feather/old_space.step;blinds::Image old;check(blinds::render(small,q,old),"old draft baseline");
  const auto start=std::chrono::steady_clock::now();auto improved=blinds::preview_mask(source,p,{true,512},{});const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();maximum_ms=std::max(maximum_ms,ms);check(improved.image.valid(),"new draft mask");
  double eold=0,enew=0;int count=0;
  for(int y=0;y<512;y+=2)for(int x=0;x<2048;x+=3){
   const auto i=(std::size_t(y)*2048+x)*4;const double xx=source.left+x,yy=source.top+y;const int ox=std::clamp(int(old_space.x(xx)),0,old.width-1),oy=std::clamp(int(old_space.y(yy)),0,old.height-1);const auto oi=(std::size_t(oy)*old.width+ox)*4;
   const auto mask=psui::preview_pixel(improved.image,xx,yy,improved.space);const double alpha=source.rgba[i+3]*mask[3];
   for(int k=0;k<3;k++){const double expected=fine.rgba[i+k]*fine.rgba[i+3];eold+=std::abs(expected-old.rgba[oi+k]*old.rgba[oi+3]);enew+=std::abs(expected-source.rgba[i+k]*alpha);++count;}
  }
  eold/=count;enew/=count;total_old+=eold;total_new+=enew;if(report)report<<index<<','<<eold<<','<<enew<<','<<ms<<'\n';
  auto full=blinds::preview_mask(source,p,{false,512},{});for(int y=0;y<512;y+=31)for(int x=0;x<2048;x+=29){const auto i=(std::size_t(y)*2048+x)*4;check(std::abs(full.image.rgba[i+3]*source.rgba[i+3]-fine.rgba[i+3])<1e-7,"full preview equals native opacity");}
 }
 check(total_new<total_old*.65,"representative fine detail/transparent strokes move closer to native output");
 std::cout<<"PASS adaptive bounds/hysteresis, alpha-aware sampling, native-aligned mask preview; mean error reduction="<<(1-total_new/total_old)*100<<"%; maximum draft_ms="<<maximum_ms<<'\n';
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
