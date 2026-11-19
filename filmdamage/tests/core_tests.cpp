#include "filmdamage_core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace filmdamage;
int checks=0;void check(bool x,const char* why){++checks;if(!x)throw std::runtime_error(why);}
Image solid(int w,int h,float value=.5f){Image im{w,h,0,0,std::vector<float>(std::size_t(w)*h*4)};for(std::size_t i=0;i<im.rgba.size();i+=4){im.rgba[i]=im.rgba[i+1]=im.rgba[i+2]=value;im.rgba[i+3]=1;}return im;}
Parameters neutral(){Parameters p;p.grain=p.fade=p.vignette=0;p.dust=p.hairs=p.stains=p.scratches=0;p.saturation=p.contrast=100;p.tint={1,1,1};return p;}
bool equal(const Image& a,const Image& b){return a.rgba==b.rgba;}
double mean(const Image& a){double v=0;for(std::size_t i=0;i<a.rgba.size();i+=4)v+=a.rgba[i];return v/(a.width*a.height);}
float red(const Image& a,int x,int y){return a.rgba[(std::size_t(y)*a.width+x)*4];}
double grain_edges(const Image& a){
 double energy=0;
 for(int y=1;y<a.height;y++)for(int x=1;x<a.width;x++){
  const double dx=red(a,x,y)-red(a,x-1,y),dy=red(a,x,y)-red(a,x,y-1);
  energy+=dx*dx+dy*dy;
 }
 return energy;
}
void write_fixture(std::ofstream& file,const Image& image){
 if(file.is_open())file.write(reinterpret_cast<const char*>(image.rgba.data()),image.rgba.size()*sizeof(float));
}
int main(int argc,char** argv){try{
 Parameters p;check(valid(p),"default parameters valid");auto src=solid(193,127),source=src;src.left=-48;src.top=-24;
 for(int y=0;y<src.height;y++)for(int x=0;x<src.width;x++){auto n=(y*src.width+x)*4;src.rgba[n]=float(x)/80;src.rgba[n+1]=float(y)/127;src.rgba[n+2]=.2f;src.rgba[n+3]=x<8?0:x<16?.6f:1;}
 Image a,b;std::ofstream fixture,new_fixture;
 if(argc>=2){fixture.open(argv[1],std::ios::binary);check(bool(fixture),"legacy fixture opens");}
 if(argc>=3){new_fixture.open(argv[2],std::ios::binary);check(bool(new_fixture),"new fixture opens");}
 for(int i=0;i<8;i++){p=preset(i);check(valid(p)&&render(src,p,a),"all eight presets render");check(render(src,p,b)&&equal(a,b),"same seed repeats exactly");
  for(std::size_t n=0;n<a.rgba.size();n+=4){check(a.rgba[n+3]==src.rgba[n+3],"source Alpha retained");for(int k=0;k<3;k++)check(std::isfinite(a.rgba[n+k])&&a.rgba[n+k]>=0,"finite HDR output");}
  write_fixture(fixture,a);
 }
 p.mix=0;check(render(src,p,a)&&equal(src,a),"mix zero is exact identity including hidden RGB");p=neutral();check(render(source,p,a)&&equal(source,a),"disabled components preserve source");
 p=preset(4);check(render(source,p,a),"damage sample");p.variation=1;check(render(source,p,b)&&!equal(a,b),"variation changes damage");p.variation=0;p.seed++;check(render(source,p,b)&&!equal(a,b),"seed changes damage");
 for(int component=0;component<5;component++){p=neutral();if(component==0)p.grain=40;if(component==1){p.dust=120;p.dust_size=8;}if(component==2){p.stains=5;p.stain_size=90;}if(component==3){p.hairs=12;p.hair_width=2;}if(component==4){p.scratches=18;p.scratch_width=2;p.scratch_gaps=0;}check(render(source,p,a)&&!equal(source,a),"each independent damage master changes output");}
 p=neutral();p.grain=30;p.grain_mono=true;check(render(source,p,a),"monochrome grain renders");for(std::size_t i=0;i<a.rgba.size();i+=4)check(a.rgba[i]==a.rgba[i+1]&&a.rgba[i]==a.rgba[i+2],"mono channels match");
 auto hidden=src,hidden2=src;for(std::size_t i=0;i<hidden.rgba.size();i+=4)if(hidden.rgba[i+3]==0)for(int k=0;k<3;k++){hidden.rgba[i+k]=99;hidden2.rgba[i+k]=-24;}
 p=neutral();p.defocus=4;p.shake=2;p.tilt=.5;check(render(hidden,p,a)&&render(hidden2,p,b)&&equal(a,b),"weighted blur/transform does not leak hidden RGB");
 p=neutral();p.exposure=2;auto hdr=solid(24,17,2);check(render(hdr,p,a)&&mean(a)>7.9,"HDR headroom retained");
 p=neutral();p.dust=80;p.dust_size=18;auto full=solid(480,320),proxy=solid(120,80);check(render(full,p,a)&&render(proxy,p,b,{}, {0,0,480,320,4}),"native/draft geometry renders");check(std::abs(mean(a)-mean(b))<.025,"proxy retains native damage distribution");
 auto highlight=solid(128,96,.12f);
 for(int y=0;y<highlight.height;y++)for(int x=0;x<64;x++){
  const auto i=(y*highlight.width+x)*4;highlight.rgba[i]=highlight.rgba[i+1]=highlight.rgba[i+2]=.92f;
 }
 p=neutral();p.halation=65;p.halation_radius=8;
 check(render(highlight,p,a),"halation around a bright edge");
 check(red(a,66,46)>.22&&red(a,66,46)>a.rgba[(46*128+66)*4+1]+.07,"warm on dark side of edge");
 check(red(a,10,46)==red(highlight,10,46)&&red(a,114,46)==red(highlight,114,46),"no tint in bright interiors or remote shadows");
 write_fixture(new_fixture,a);
 auto unlit=solid(128,96,.6f);check(render(unlit,p,b)&&equal(unlit,b),"no halo below threshold");
 p.halation_threshold=96;check(render(highlight,p,b)&&equal(highlight,b),"higher threshold suppresses halo");
 p.halation_threshold=70;p.halation_radius=2;
 check(render(highlight,p,b)&&red(b,75,46)<red(a,75,46),"radius changes spread");
 p.halation_radius=8;
 auto draft=solid(32,24,.12f);for(int y=0;y<24;y++)for(int x=0;x<16;x++){const auto i=(y*32+x)*4;draft.rgba[i]=draft.rgba[i+1]=draft.rgba[i+2]=.92f;}
 check(render(draft,p,b,{}, {0,0,128,96,4})&&std::abs(red(a,70,46)-red(b,17,11))<.045,"draft halo retains approximate native radius and strength");
 auto transparent=highlight;
 for(int y=0;y<96;y++)for(int x=0;x<64;x++)transparent.rgba[(y*128+x)*4+3]=0;
 check(render(transparent,p,a)&&red(a,66,46)==red(transparent,66,46)&&a.rgba[(46*128+22)*4+3]==0,"hidden highlights do not emit or add opacity");
 p=neutral();p.grain=30;p.grain_size=2.7;check(render(src,p,a),"grain softness reference");
 p.grain_softness=100;check(render(src,p,b)&&grain_edges(b)<grain_edges(a),"grain softness reduces fine noise");
 write_fixture(new_fixture,b);
 p.grain_size=1;p.grain_softness=0;check(render(source,p,a),"one-pixel grain reference");
 p.grain_softness=100;check(render(source,p,b)&&grain_edges(b)<.85*grain_edges(a),"grain softness works at the default one-pixel size");
 write_fixture(new_fixture,b);
 p.grain_mono=true;check(render(source,p,a),"soft monochrome grain");
 for(std::size_t i=0;i<a.rgba.size();i+=4)check(a.rgba[i]==a.rgba[i+1]&&a.rgba[i]==a.rgba[i+2],"soft monochrome channels match");
 p.grain=0;check(render(source,p,b)&&equal(source,b),"grain softness cannot soften image details");
 p=preset(7);p.grain_softness=65;p.halation=40;p.leak=30;
 p.dust=p.stains=p.hairs=p.scratches=0;check(render(src,p,a),"reroll isolation reference");
 p.damage_variation=5;check(render(src,p,b)&&equal(a,b),"reroll leaves other effects unchanged");
 for(int component=0;component<4;component++){
  p=preset(7);p.dust=p.stains=p.hairs=p.scratches=0;
  if(component==0)p.dust=180;if(component==1)p.stains=12;if(component==2)p.hairs=15;if(component==3)p.scratches=32;
  check(render(source,p,a),"individual mark reference");p.damage_variation=5;
  check(render(source,p,b)&&!equal(a,b),"reroll moves each kind of mark");
  write_fixture(new_fixture,b);check(render(source,p,a)&&equal(a,b),"rerolled positions are repeatable");
 }
 p=preset(7);p.grain_softness=60;p.halation=35;p.damage_variation=19;
 check(render(src,p,a),"new controls render with gate weave, rotation, HDR and transparency");
 write_fixture(new_fixture,a);
 p.mix=0;check(render(src,p,a)&&equal(src,a),"new effects respect exact bypass");
 p=neutral();p.stains=4;p.stain_size=140;p.stain_opacity=65;
 auto stock=solid(256,192,.66f);
 check(render(stock,p,a),"flat stain reference");
 auto flat_stains=a;
 p.stain_texture=100;check(render(stock,p,b),"textured stains");
 int darker=0,lighter=0,clear=0;
 for(std::size_t i=0;i<a.rgba.size();i+=4){
  if(a.rgba[i]==stock.rgba[i]){check(b.rgba[i]==stock.rgba[i],"texture stays within stain footprints");clear++;}
  else if(b.rgba[i]<a.rgba[i]-.025f)darker++;
  else if(b.rgba[i]>a.rgba[i]+.025f)lighter++;
  check(b.rgba[i+3]==stock.rgba[i+3],"texture does not change alpha");
 }
 check(darker>400&&lighter>400&&clear>0,"texture adds dark deposits and lighter patches");
 check(render(stock,p,a)&&equal(a,b),"textured mark pattern is deterministic");
 write_fixture(new_fixture,b);
 auto small_stock=solid(64,48,.66f);
 check(render(small_stock,p,a,{}, {0,0,256,192,4})&&std::abs(mean(a)-mean(b))<.02,"draft texture has similar density");
 p.stain_texture=0;check(render(stock,p,a)&&equal(a,flat_stains),"zero restores the original stains");
 p.stain_texture=100;p.stain_opacity=0;
 check(render(stock,p,a)&&equal(a,stock),"texture respects stain opacity");
 p.stain_opacity=100;p.stain_size=1000;b=stock;int texture_calls=0;
 check(!render(stock,p,b,[&]{return ++texture_calls>10;})&&equal(b,stock),"cancel during textured stain raster leaves output untouched");
 p.stain_texture=101;check(!valid(p),"out-of-range stain texture rejected");
 p=neutral();p.stains=4;p.stain_size=140;p.stain_opacity=65;p.stain_edge_roughness=100;
 check(render(stock,p,a),"rough contour with texture disabled");
 int grown=0,eroded=0;
 for(std::size_t i=0;i<a.rgba.size();i+=4){
  if(flat_stains.rgba[i]==stock.rgba[i]&&a.rgba[i]<stock.rgba[i]-.01f)grown++;
  if(a.rgba[i]==stock.rgba[i]&&flat_stains.rgba[i]<stock.rgba[i]-.01f)eroded++;
  check(a.rgba[i+3]==stock.rgba[i+3],"roughness preserves alpha");
 }
 check(grown>100&&eroded>100,"roughness grows and erodes the actual contour");
 check(render(stock,p,b)&&equal(a,b),"rough contour is deterministic");
 check(render(small_stock,p,b,{}, {0,0,256,192,4})&&std::abs(mean(a)-mean(b))<.02,"draft rough contour has similar coverage");
 write_fixture(new_fixture,a);
 p.stain_texture=100;check(render(stock,p,b)&&!equal(a,b),"rough contour and texture combine");
 write_fixture(new_fixture,b);
 p.stain_opacity=0;check(render(stock,p,b)&&equal(stock,b),"roughness respects zero stain opacity");
 p.stain_edge_roughness=101;check(!valid(p),"invalid roughness rejected");
 p=neutral();p.stains=4;p.stain_size=140;p.stain_opacity=65;p.stain_edge_roughness=100;
 Image coarse_edge,fine_edge;
 check(render(stock,p,coarse_edge),"original edge detail at 100%");
 p.stain_edge_size=28;
 check(render(stock,p,fine_edge)&&!equal(coarse_edge,fine_edge),"finer chips at the same roughness");
 auto edge_changes=[&](const Image& im){
  auto at=[&](int x,int y){const auto i=(std::size_t(y)*im.width+x)*4;return std::abs(im.rgba[i]-flat_stains.rgba[i])>.02f;};
  int area=0,perimeter=0;
  for(int y=0;y<im.height;y++)for(int x=0;x<im.width;x++){
   const bool changed=at(x,y);area+=changed;
   if(x)perimeter+=changed!=at(x-1,y);
   if(y)perimeter+=changed!=at(x,y-1);
  }
  return std::array<int,2>{area,perimeter};
 };
 const auto [coarse_area,coarse_perimeter]=edge_changes(coarse_edge);
 const auto [fine_area,fine_perimeter]=edge_changes(fine_edge);
 check(fine_area>100&&fine_area<coarse_area*.75,"fine chips affect a narrower edge band");
 check(double(fine_perimeter)/fine_area>1.25*coarse_perimeter/coarse_area,"fine chips have more small contour changes per area");
 check(render(small_stock,p,a,{}, {0,0,256,192,4})&&std::abs(mean(a)-mean(fine_edge))<.02,"fine detail is filtered at draft resolution");
 write_fixture(new_fixture,fine_edge);
 p.stain_texture=65;
 check(render(stock,p,b)&&!equal(fine_edge,b),"fine chips work with stain texture");
 write_fixture(new_fixture,b);
 p.stain_edge_roughness=0;
 check(render(stock,p,a),"fine size without roughness");
 p.stain_edge_size=250;
 check(render(stock,p,b)&&equal(a,b),"size alone does not change stain texture or base shape");
 p.stain_edge_size=0;check(!valid(p),"invalid edge size is rejected");
 p=preset(7);b=source;int calls=0;check(!render(src,p,b,[&]{return ++calls>3;})&&equal(b,source),"cancel leaves output untouched");
 p.grain=std::numeric_limits<double>::quiet_NaN();check(!valid(p)&&!render(src,p,a),"invalid values rejected");
 auto large=solid(768,768);auto start=std::chrono::steady_clock::now();check(render(large,preset(0),a),"large image");double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 auto textured=preset(0);textured.halation=35;textured.grain_softness=55;
 start=std::chrono::steady_clock::now();check(render(large,textured,a),"large image with new features");double textured_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"PASS: "<<checks<<" checks; eight presets; halation/softness/reroll/stain texture/rough edge size; exact bypass; Alpha/HDR; hidden RGB; native/draft distribution; cancellation; 768px "<<ms<<"ms default, "<<textured_ms<<"ms feature sample\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
