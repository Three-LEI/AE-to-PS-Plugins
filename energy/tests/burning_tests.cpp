#include "energy_core.hpp"
#include "energy_legacy.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
int checks=0;void require(bool value,const char* text){checks++;if(!value)throw std::runtime_error(text);}
double centroid_x(const energy::Image& image,const energy::Image& baseline){double mass=0,moment=0;for(int y=0;y<image.height;y++)for(int x=0;x<image.width;x++){const auto n=(std::size_t(y)*image.width+x)*4;const double e=std::max(0.,double(image.rgba[n]-baseline.rgba[n]));mass+=e;moment+=e*(x+.5);}require(mass>.01,"burning body has measurable energy");return moment/mass;}
}
int main(){try{
 using namespace energy;Image source{224,192,0,0,std::vector<float>(224*192*4)},a,b,c;
 auto p=material_preset(4,calibrated_defaults(224,192));p.start_x=40;p.end_x=184;p.start_y=p.end_y=96;p.composite=2;
 require(render(source,p,{},a)&&render(source,p,{},b)&&a.rgba==b.rgba,"burning is exactly repeatable");for(float value:a.rgba)require(std::isfinite(value)&&value>=0,"finite burning output");
 auto changed=p;changed.burn_seed++;require(render(source,changed,{},b)&&a.rgba!=b.rgba,"independent burning seed changes the body");changed=p;changed.burn_evolution=71;require(render(source,changed,{},b)&&a.rgba!=b.rgba,"burning evolution selects another body");
 auto isolated=p;isolated.halo_intensity=0;require(render(source,isolated,{},a),"body independence baseline");changed=isolated;changed.distortion=12;require(render(source,changed,{},b),"core distortion change");int core_changes=0;
 for(int y=0;y<192;y++)for(int x=0;x<224;x++)for(int k=0;k<3;k++){const auto n=(y*224+x)*4+k;if(y<78||y>114)require(a.rgba[n]==b.rgba[n],"core distortion leaves isolated body and far light identical");else core_changes+=a.rgba[n]!=b.rgba[n];}
 require(core_changes>20,"core actually changes");
 for(int y=40;y<150;y++)for(int x=40;x<184;x++)if(!(x>=92&&x<132&&y>=74&&y<114))source.rgba[(y*224+x)*4+3]=1;
 for(int input=0;input<4;input++){
  auto off=p;off.input=input;off.distortion=0;off.branches=0;off.burn_density=0;auto plain=off;plain.style=0;auto path=line_path(off);
  require(render(source,off,path,a)&&render(source,plain,path,b)&&a.rgba==b.rgba,"zero burning intensity reproduces existing layered model for every input");
 }
 p.input=2;require(render(source,p,{},a)&&render(source,p,{},b)&&a.rgba==b.rgba,"alpha contours and holes burn deterministically");p.input=3;require(render(source,p,{},a),"original alpha strokes burn");
 for(auto& value:source.rgba)value=0;
 for(int y=0;y<192;y++)for(int x=0;x<224;x++)if(std::abs(std::hypot(x+.5-112,y+.5-96)-56)<2)source.rgba[(y*224+x)*4+3]=1;
 auto circle=p;circle.distortion=0;circle.burn_wind=0;auto circle_base=circle;circle_base.burn_density=0;require(render(source,circle,{},a)&&render(source,circle_base,{},b),"wrapped ring");double quadrant[4]={};
 for(int y=0;y<192;y++)for(int x=0;x<224;x++){const auto n=(y*224+x)*4;quadrant[(x>=112)+2*(y>=96)]+=std::max(0.,double(a.rgba[n]-b.rgba[n]));}
 for(auto value:quadrant)require(value>1,"no-wind burning surrounds all four ring quadrants");
 for(auto& value:source.rgba)value=0;
 auto point=material_preset(4,calibrated_defaults(224,192));point.start_x=point.end_x=112;point.start_y=point.end_y=96;point.distortion=0;point.composite=2;auto point_base=point;point_base.burn_density=0;
 require(render(source,point_base,{},c)&&render(source,point,{},a),"point burning baseline");const auto center=centroid_x(a,c);auto east=point;east.burn_wind=24;east.fire_direction=0;require(render(source,east,{},a),"east wind");const auto right=centroid_x(a,c);east.fire_direction=180;require(render(source,east,{},b),"west wind");const auto left=centroid_x(b,c);
 require(right>center+4&&left<center-4,"wind pushes the body while the core stays fixed");
 auto local=point;local.burn_width=2;local.burn_turbulence=3;local.glow_radius=3;local.halo_radius=1;require(render(source,local,{},a),"translation baseline");changed=local;changed.offset_x=7;changed.offset_y=-5;require(render(source,changed,{},b),"translated burning");
 for(int y=25;y<155;y++)for(int x=25;x<185;x++)for(int k=0;k<4;k++)require(std::abs(a.rgba[(y*224+x)*4+k]-b.rgba[((y-5)*224+x+7)*4+k])<2e-5,"flame texture stays anchored when the effect moves");
 auto draft=draft_parameters(point,0,0,4);require(draft.burn_width==point.burn_width/4&&draft.burn_turbulence==point.burn_turbulence/4&&draft.burn_scale==point.burn_scale/4&&draft.burn_wind==point.burn_wind/4&&draft.burn_seed==point.burn_seed&&draft.burn_evolution==point.burn_evolution,"draft preserves noise identity and scales only physical dimensions");
 point.input=3;require(render(source,point,{},a),"empty alpha");for(std::size_t n=0;n<a.rgba.size();n+=4)require(a.rgba[n]==0&&a.rgba[n+1]==0&&a.rgba[n+2]==0,"empty emitter cannot generate fire");
 point.input=0;point.composite=0;point.brightness=0;for(std::size_t n=0;n<source.rgba.size();n+=4){source.rgba[n]=2.5f;source.rgba[n+3]=.3f;}require(render(source,point,{},a)&&a.rgba==source.rgba,"zero brightness preserves HDR source");point.brightness=3;require(render(source,point,{},a)&&*std::max_element(a.rgba.begin(),a.rgba.end())>1,"burning retains HDR energy");auto keep=a;int polls=0;require(!render(source,point,{},a,[&]{return ++polls>15;})&&a.rgba==keep.rgba,"cancel does not publish partial burning");
 for(int i=0;i<5;i++){auto bad=point;if(i==0)bad.burn_detail=0;if(i==1)bad.burn_seed=0xffffffffu;if(i==2)bad.burn_scale=0;if(i==3)bad.burn_wind=std::numeric_limits<double>::infinity();if(i==4)bad.glow_model=0;require(!valid(bad)&&!render(source,bad,{},a),"invalid burning control rejected");}
 legacy::SavedV4 old;old.values.prefix.prefix.style=3;old.values.prefix.glow_model=1;old.values.glow_seed=12345;auto migrated=point;require(legacy::upgrade(&old,sizeof(old),migrated)&&migrated.style==3&&migrated.glow_seed==12345&&migrated.burn_seed==5000,"0.4 block retains electric style and initializes burning fields");
 std::cout<<"PASS "<<checks<<" burning checks: wrapped body, independent seed, wind, identity, anchoring, HDR and 0.4 migration\n";
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
