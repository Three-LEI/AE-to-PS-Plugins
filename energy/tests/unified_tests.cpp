#include "energy_core.hpp"
#include "energy_legacy.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
int checks=0;void require(bool value,const char* text){checks++;if(!value)throw std::runtime_error(text);}
// Independent list of editable common controls. No engine/style switch is
// copied: a preset must be reproducible by editing this public surface alone.
void public_controls(const energy::Parameters& a,energy::Parameters& b){
 b.width=a.width;b.brightness=a.brightness;b.core_white=a.core_white;b.color=a.color;b.tip_color=a.tip_color;b.color_mix=a.color_mix;
 b.glow_radius=a.glow_radius;b.glow_intensity=a.glow_intensity;b.halo_radius=a.halo_radius;b.halo_intensity=a.halo_intensity;b.glow_gamma=a.glow_gamma;b.glow_bias=a.glow_bias;b.glow_sizes=a.glow_sizes;b.glow_gains=a.glow_gains;
 b.core_noise=a.core_noise;b.distortion=a.distortion;b.wavelength=a.wavelength;b.detail=a.detail;b.seed=a.seed;b.evolution=a.evolution;b.branches=a.branches;b.branch_length=a.branch_length;
 b.glow_noise=a.glow_noise;b.glow_distortion=a.glow_distortion;b.glow_noise_scale=a.glow_noise_scale;b.glow_detail=a.glow_detail;b.glow_seed=a.glow_seed;b.glow_evolution=a.glow_evolution;b.glow_width=a.glow_width;b.glow_texture=a.glow_texture;b.filament_strength=a.filament_strength;b.filament_count=a.filament_count;b.burn_wind=a.burn_wind;b.fire_direction=a.fire_direction;
}
}
int main(){try{
 using namespace energy;Image source{224,192,0,0,std::vector<float>(224*192*4)},a,b,c;
 auto p=unified_defaults(224,192);p.composite=2;p.start_x=40;p.end_x=184;p.start_y=p.end_y=96;
 require(p.engine==1&&p.style==0&&valid(p),"new default uses one common pipeline");require(render(source,p,{},a)&&render(source,p,{},b)&&a.rgba==b.rgba,"common default is repeatable");
 auto old=p;old.engine=0;require(render(source,old,{},c)&&a.rgba==c.rgba,"default light retains the calibrated basic appearance");
 for(int preset=0;preset<unified_preset_count;preset++){
  auto target=unified_preset(preset,p);require(target.engine==1&&target.style==0&&target.glow_model==1,"every preset has identical engine flags");auto manual=p;public_controls(target,manual);
  require(render(source,target,{},a)&&render(source,manual,{},b)&&a.rgba==b.rgba,"public parameter edits alone reproduce each preset exactly");
  auto irrelevant=target;irrelevant.fire_height=310;irrelevant.fire_spread=90;irrelevant.burn_width=49;irrelevant.burn_turbulence=73;irrelevant.burn_density=8;irrelevant.burn_seed=123;irrelevant.burn_evolution=239;
  require(render(source,irrelevant,{},b)&&a.rgba==b.rgba,"old hidden material controls do not affect the common renderer");
 }
 require(render(source,p,{},a),"plain light baseline");auto cloud=p;cloud.glow_texture=1;require(render(source,cloud,{},b)&&a.rgba!=b.rgba&&cloud.style==0,"cloud control works directly from default without selecting a material");auto fine=p;fine.filament_strength=1;require(render(source,fine,{},b)&&a.rgba!=b.rgba,"filaments work with cloud intensity zero");
 auto wind=p;wind.burn_wind=20;wind.fire_direction=0;require(render(source,wind,{},b)&&a.rgba!=b.rgba,"wind acts on default outer light without a fire mode");auto gradient=p;gradient.color_mix=.7;require(render(source,gradient,{},b)&&a.rgba!=b.rgba,"color transition is a common control");
 auto core=p;core.distortion=8;core.halo_intensity=0;require(render(source,core,{},a),"core noise baseline");for(int type:{0,2}){auto different=core;different.core_noise=type;require(render(source,different,{},b)&&a.rgba!=b.rgba,"core noise type changes the same pipeline");}
 auto outer=cloud;outer.glow_distortion=12;outer.filament_strength=.7;outer.halo_intensity=0;require(render(source,outer,{},a),"outer field baseline");for(int type:{0,2}){auto different=outer;different.glow_noise=type;require(render(source,different,{},b)&&a.rgba!=b.rgba,"outer noise type changes the same pipeline");}
 auto different=outer;different.distortion=12;require(render(source,different,{},b),"independent core change");for(int y=0;y<192;y++)for(int x=0;x<224;x++)if(y<78||y>114)for(int k=0;k<3;k++)require(a.rgba[(y*224+x)*4+k]==b.rgba[(y*224+x)*4+k],"core deformation does not change isolated outer field");
 for(int y=40;y<150;y++)for(int x=40;x<184;x++)if(!(x>=92&&x<132&&y>=74&&y<114))source.rgba[(y*224+x)*4+3]=1;
 for(int input=0;input<4;input++){auto q=unified_preset(3,p);q.input=input;auto path=line_path(q);require(render(source,q,path,a)&&render(source,q,path,b)&&a.rgba==b.rgba,"common mixed settings support all inputs and alpha holes");for(auto value:a.rgba)require(std::isfinite(value)&&value>=0,"finite common HDR output");}
 for(auto& value:source.rgba)value=0;
 auto local=p;local.start_x=local.end_x=112;local.glow_radius=3;local.halo_radius=1;local.glow_width=2;local.glow_texture=.8;local.filament_strength=.5;local.glow_distortion=3;require(render(source,local,{},a),"translation baseline");different=local;different.offset_x=7;different.offset_y=-5;require(render(source,different,{},b),"common effect offset");
 for(int y=25;y<155;y++)for(int x=25;x<185;x++)for(int k=0;k<4;k++)require(std::abs(a.rgba[(y*224+x)*4+k]-b.rgba[((y-5)*224+x+7)*4+k])<2e-5,"common texture stays anchored when moved");
 for(int style=0;style<5;style++){auto prior=material_preset(style,p);prior.engine=0;const auto before=prior;auto converted=to_unified(prior);require(prior==before&&converted.engine==1&&converted.style==0&&valid(converted)&&converted.start_x==prior.start_x&&converted.end_y==prior.end_y&&converted.input==prior.input,"explicit conversion preserves geometry and never mutates legacy settings");}
 auto blank=unified_preset(3,p);blank.input=3;require(render(source,blank,{},a),"empty alpha common renderer");for(std::size_t n=0;n<a.rgba.size();n+=4)require(a.rgba[n]==0&&a.rgba[n+1]==0&&a.rgba[n+2]==0,"empty alpha emits nothing");
 p.brightness=0;p.composite=0;for(std::size_t n=0;n<source.rgba.size();n+=4){source.rgba[n]=2.5f;source.rgba[n+3]=.3f;}require(render(source,p,{},a)&&a.rgba==source.rgba,"zero common brightness preserves exact source");p.brightness=3;require(render(source,p,{},a)&&*std::max_element(a.rgba.begin(),a.rgba.end())>1,"common pipeline retains HDR");auto keep=a;int polls=0;require(!render(source,p,{},a,[&]{return ++polls>12;})&&a.rgba==keep.rgba,"cancel preserves last output");
 for(int i=0;i<4;i++){auto bad=p;if(i==0)bad.engine=9;if(i==1)bad.core_noise=3;if(i==2)bad.glow_noise=-1;if(i==3)bad.color_mix=std::numeric_limits<double>::infinity();require(!valid(bad)&&!render(source,bad,{},a),"invalid common control rejected");}
 legacy::SavedV5 saved;saved.values.prefix.prefix.prefix.style=4;saved.values.prefix.prefix.glow_model=1;saved.values.burn_seed=9876;auto migrated=p;require(legacy::upgrade(&saved,sizeof(saved),migrated)&&migrated.engine==0&&migrated.style==4&&migrated.burn_seed==9876,"0.5 block always stays on legacy renderer");
 std::cout<<"PASS "<<checks<<" common controls checks: parameter-only presets, live stages, mixed inputs, conversion and legacy isolation\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
