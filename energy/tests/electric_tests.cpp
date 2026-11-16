#include "energy_core.hpp"
#include "energy_legacy.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {int checks=0;void require(bool v,const char* message){checks++;if(!v)throw std::runtime_error(message);}}
int main(){try{
 using namespace energy;Image source{224,192,0,0,std::vector<float>(224*192*4)},a,b,c;
 auto p=material_preset(3,calibrated_defaults(224,192));p.start_x=40;p.end_x=184;p.start_y=p.end_y=96;p.composite=2;
 require(render(source,p,{},a)&&render(source,p,{},b)&&a.rgba==b.rgba,"new electric output is exactly repeatable");
 for(float v:a.rgba)require(std::isfinite(v)&&v>=0,"finite nonnegative electric energy");
 auto nohalo=p;nohalo.halo_intensity=0;require(render(source,nohalo,{},a),"outer light baseline");auto changed=nohalo;changed.distortion=12;require(render(source,changed,{},b),"change only core distortion");
 int core_changes=0;for(int y=0;y<192;y++)for(int x=0;x<224;x++)for(int k=0;k<3;k++){
  const auto n=(y*224+x)*4+k;if(y<80||y>112)require(a.rgba[n]==b.rgba[n],"core distortion leaves the isolated outer glow identical");else core_changes+=a.rgba[n]!=b.rgba[n];
 }
 require(core_changes>20,"core distortion actually changes the wire");
 auto onlycore=p;onlycore.glow_intensity=onlycore.halo_intensity=0;require(render(source,onlycore,{},a),"isolated new core");changed=onlycore;changed.glow_seed++;changed.glow_distortion=70;require(render(source,changed,{},b)&&a.rgba==b.rgba,"outer seed and warp never alter isolated core");
 require(a.rgba[(96*224+40)*4+2]>.1&&a.rgba[(96*224+183)*4+2]>.1,"open core endpoints stay pinned");
 changed=p;changed.glow_seed++;require(render(source,p,{},a)&&render(source,changed,{},b)&&a.rgba!=b.rgba,"independent glow seed changes outer structure");changed=p;changed.glow_evolution=73;require(render(source,changed,{},b)&&a.rgba!=b.rgba,"glow-only evolution changes outer structure");
 for(int y=40;y<150;y++)for(int x=40;x<184;x++)if(!(x>=92&&x<132&&y>=74&&y<114))source.rgba[(y*224+x)*4+3]=1;
 for(int input=0;input<4;input++){
  auto off=p;off.input=input;off.distortion=off.glow_distortion=off.glow_texture=0;off.branches=0;auto path=line_path(off);auto plain=off;plain.style=0;
  require(render(source,off,path,a)&&render(source,plain,path,b)&&a.rgba==b.rgba,"all new distortions off matches existing layered model for every input");
 }
 auto small=p;small.distortion=0;small.glow_radius=3;small.glow_width=2;small.halo_radius=1;small.glow_distortion=4;small.glow_noise_scale=20;require(render(source,small,{},a),"translation baseline");changed=small;changed.offset_x=7;changed.offset_y=-5;require(render(source,changed,{},b),"move effect with anchored noise");
 for(int y=35;y<150;y++)for(int x=15;x<200;x++)for(int k=0;k<4;k++)require(std::abs(a.rgba[(y*224+x)*4+k]-b.rgba[((y-5)*224+x+7)*4+k])<2e-5,"translation preserves anchored electric material away from clipping");
 auto draft=draft_parameters(p,0,0,4);require(draft.glow_distortion==p.glow_distortion/4&&draft.glow_width==p.glow_width/4&&draft.glow_noise_scale==p.glow_noise_scale/4&&draft.glow_seed==p.glow_seed&&draft.glow_detail==p.glow_detail&&draft.glow_evolution==p.glow_evolution,"draft scales only new spatial controls");
 p.input=2;require(render(source,p,{},a)&&render(source,p,{},b)&&a.rgba==b.rgba,"nonempty alpha contours with a hole render deterministically");p.input=3;require(render(source,p,{},a),"nonempty original alpha strokes");
 for(auto& value:source.rgba)value=0;
 require(render(source,p,{},a),"empty alpha electric");for(std::size_t n=0;n<a.rgba.size();n+=4)require(a.rgba[n]==0&&a.rgba[n+1]==0&&a.rgba[n+2]==0,"no noise outside an empty emitter");
 p.input=0;p.brightness=0;p.composite=0;for(std::size_t n=0;n<source.rgba.size();n+=4){source.rgba[n]=2.5f;source.rgba[n+3]=.3f;}require(render(source,p,{},a)&&a.rgba==source.rgba,"zero brightness preserves exact HDR source");
 p.brightness=3;require(render(source,p,{},a),"HDR electric");auto keep=a;int polls=0;require(!render(source,p,{},a,[&]{return ++polls>12;})&&a.rgba==keep.rgba,"cancelled electric does not publish partial output");
 for(int field=0;field<5;field++){auto bad=p;if(field==0)bad.glow_detail=0;if(field==1)bad.glow_seed=0xffffffffu;if(field==2)bad.glow_distortion=std::numeric_limits<double>::infinity();if(field==3)bad.glow_noise_scale=0;if(field==4)bad.glow_model=0;require(!valid(bad)&&!render(source,bad,{},a),"invalid new electric controls rejected");}
 legacy::SavedV3 old;old.values.glow_model=1;old.values.prefix.style=1;old.values.halo_radius=7;auto upgraded=p;require(legacy::upgrade(&old,sizeof(old),upgraded)&&upgraded.style==1&&upgraded.glow_model==1&&upgraded.halo_radius==7&&upgraded.glow_seed==5000,"0.3 POD keeps old style and resets appended fields");
 std::cout<<"PASS "<<checks<<" electric checks: independent fields, pinned endpoints, noise locking, identity, HDR and 0.3 migration\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
