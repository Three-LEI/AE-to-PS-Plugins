#include "energy_core.hpp"
#include "energy_legacy.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
int checks=0;
void require(bool value,const char* text){checks++;if(!value)throw std::runtime_error(text);}
double energy_sum(const energy::Image& image){double sum=0;for(std::size_t i=0;i<image.rgba.size();i+=4)sum+=image.rgba[i+2]*image.rgba[i+3];return sum;}
}
int main(){try{
 using namespace energy;
 Image source{192,144,0,0,std::vector<float>(192*144*4)},a,b,c;
 auto p=calibrated_defaults(192,144);p.composite=1;p.start_x=50;p.end_x=142;p.start_y=p.end_y=72;p.glow_radius=20;p.halo_radius=4;
 require(render(source,p,{},a)&&render(source,p,{},b)&&a.rgba==b.rgba,"layered light is deterministic");
 for(float value:a.rgba)require(std::isfinite(value)&&value>=0,"finite layered light");
 auto core=p;core.glow_intensity=core.halo_intensity=0;require(render(source,core,{},b),"core without any glow");
 require(b.rgba[(82*192+96)*4+3]==0&&a.rgba[(82*192+96)*4+3]>0,"independent light reaches beyond core");
 auto halo=core;halo.halo_intensity=1;require(render(source,halo,{},c)&&energy_sum(c)>energy_sum(b),"halo works with far-glow intensity zero");
 halo.halo_intensity=0;halo.glow_intensity=1;halo.glow_gains.fill(0);require(render(source,halo,{},c)&&c.rgba==b.rgba,"all bands disabled leaves only the core");
 for(int band=0;band<4;band++){
  auto one=halo;one.glow_gains[band]=1;require(render(source,one,{},c)&&energy_sum(c)>energy_sum(b),"each glow band emits independently");
  const double base=energy_sum(c)-energy_sum(b);one.glow_gains[band]=2;require(render(source,one,{},c),"band intensity increase");
  require(std::abs((energy_sum(c)-energy_sum(b))/base-2)<1e-5,"band energy doubles without HDR clipping");
 }
 auto fractional=p;fractional.glow_radius=20.001;require(render(source,fractional,{},b),"fractional radius");
 double maximum=0;for(std::size_t i=0;i<a.rgba.size();i+=4)maximum=std::max(maximum,std::abs(double(a.rgba[i+2]*a.rgba[i+3]-b.rgba[i+2]*b.rgba[i+3])));
 require(maximum<.001,"small radius changes do not jump between integer kernels");
 auto moved=p;moved.offset_x=7;moved.offset_y=-5;p.glow_radius=moved.glow_radius=3;p.halo_radius=moved.halo_radius=1;
 require(render(source,p,{},a)&&render(source,moved,{},b),"translated layered light");
 for(int y=20;y<120;y++)for(int x=20;x<160;x++)for(int k=0;k<4;k++)require(std::abs(a.rgba[(y*192+x)*4+k]-b.rgba[((y-5)*192+x+7)*4+k])<2e-5,"translation preserves light away from canvas clipping");
 auto draft=draft_parameters(p,0,0,4);require(draft.halo_radius==p.halo_radius/4&&draft.glow_sizes==p.glow_sizes&&draft.glow_gains==p.glow_gains&&draft.glow_gamma==p.glow_gamma,"draft scales physical radius but not light shape controls");
 p.input=3;require(render(source,p,{},a)&&energy_sum(a)==0,"empty alpha emits no light");
 for(std::size_t i=0;i<source.rgba.size();i+=4){source.rgba[i]=2.5f;source.rgba[i+1]=.2f;source.rgba[i+2]=.7f;source.rgba[i+3]=.3f;}
 p.input=0;p.composite=0;p.brightness=0;require(render(source,p,{},a)&&a.rgba==source.rgba,"zero brightness preserves exact HDR source");
 p.brightness=5;require(render(source,p,{},a)&&*std::max_element(a.rgba.begin(),a.rgba.end())>1,"layered compositor retains HDR");
 auto tiny=p;tiny.glow_sizes[0]=std::numeric_limits<double>::denorm_min();tiny.glow_bias=2;require(render(source,tiny,{},b)&&std::all_of(b.rgba.begin(),b.rgba.end(),[](float v){return std::isfinite(v);}),"tiny finite band size cannot overflow a biased gain");
 auto unchanged=a;int polls=0;require(!render(source,p,{},a,[&]{return ++polls>10;})&&a.rgba==unchanged.rgba,"cancel does not publish partial output");
 for(int field=0;field<6;field++){auto bad=p;if(field==0)bad.glow_model=9;if(field==1)bad.glow_sizes[3]=-1;if(field==2)bad.glow_gains[0]=std::numeric_limits<double>::infinity();if(field==3)bad.glow_gamma=0;if(field==4)bad.halo_radius=-1;if(field==5)bad.glow_bias=3;require(!valid(bad)&&!render(source,bad,{},a),"invalid new control rejected");}
 legacy::SavedV2 old;old.values.style=2;old.values.input=3;old.values.seed=987;old.values.fire_height=67;
 auto upgraded=calibrated_defaults(20,20);require(legacy::upgrade(&old,sizeof(old),upgraded)&&upgraded.glow_model==0&&upgraded.style==2&&upgraded.input==3&&upgraded.seed==987&&upgraded.fire_height==67,"0.2 POD preserves old material and uses classic lighting");
 old.magic=0;auto keep=upgraded;require(!legacy::upgrade(&old,sizeof(old),upgraded)&&upgraded==keep,"invalid 0.2 POD migration transactional");
 std::cout<<"PASS "<<checks<<" layered-light checks; deterministic HDR, four bands, halo, fractional radii and 0.2 migration\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
