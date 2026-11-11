#include "pattern_core.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace patterns;
void check(bool v,const char* s){if(!v){std::cerr<<"FAIL: "<<s<<'\n';std::exit(1);}}
int main(){
 auto p=defaults(Kind::grid,128,96);p.size_from=SizeFrom::width_height;p.width=32;p.height=24;
 check(p.anchor_x==64&&p.anchor_y==48&&valid(p),"document-relative defaults");
 check(coverage(Kind::grid,p,64,60)==1&&coverage(Kind::grid,p,62,60)==.5&&coverage(Kind::grid,p,61,60)==0,"AE grid border / pixel centre convention");
 p.feather_x=6;p.feather_y=4;check(std::abs(coverage(Kind::grid,p,64,48)-170./255)<.005,"AE feather intersection height");
 p.feather_x=p.feather_y=0;p.border=0;check(coverage(Kind::grid,p,64,48)==0,"zero border disappears");p.invert=true;check(coverage(Kind::grid,p,64,48)==1,"invert empty grid");
 p=defaults(Kind::checker,128,96);p.size_from=SizeFrom::width_height;p.width=32;p.height=24;
 check(coverage(Kind::checker,p,65,49)==1&&coverage(Kind::checker,p,63,49)==0&&coverage(Kind::checker,p,64,49)==.5,"checker transparent half and AA edge");
 p.feather_x=6;p.feather_y=4;check(std::abs(coverage(Kind::checker,p,65,49)-180./255)<.005,"checker separable signed distances / feather");
 for(Kind kind:{Kind::grid,Kind::checker}){
  for(auto& field:{&p.anchor_x,&p.anchor_y})*field+=.375;p.rotation=23;
  Image full,tile;check(render(kind,p,{-13,-7,1},130,90,full),"full pattern");
  for(int y=0;y<90;y+=17)for(int x=0;x<130;x+=29){const int w=std::min(29,130-x),h=std::min(17,90-y);check(render(kind,p,{-13.+x,-7.+y,1},w,h,tile),"tile pattern");for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx)for(int k=0;k<4;++k)check(tile.rgba[(std::size_t(yy)*w+xx)*4+k]==full.rgba[(std::size_t(y+yy)*130+x+xx)*4+k],"rotation/fractional anchor tile invariance");}
 }
 p=defaults(Kind::grid,128,96);p.opacity=0;p.blend=Blend::normal;const Pixel source{3,.4f,.1f,.3f};check(pixel(Kind::grid,p,64,48,source)==source,"zero opacity blend is exact HDR identity");
 p.blend=Blend::none;check(pixel(Kind::grid,p,64,48,source)==Pixel{},"none ignores original and zero opacity is transparent");
 p.opacity=50;p.blend=Blend::normal;auto out=pixel(Kind::grid,p,64,48,source);check(std::abs(out[3]-.65)<1.e-6,"associated source-over alpha");
 p.blend=Blend::stencil_alpha;out=pixel(Kind::grid,p,64,48,source);check(std::abs(out[3]-.15)<1.e-6&&out[0]==3,"stencil retains source colour and masks alpha");
 p.blend=Blend::add;out=pixel(Kind::grid,p,64,48,{3,.4f,.1f,1});check(out[0]>3,"add retains float HDR");
 p.opacity=40;p.blend=Blend::none;out=pixel(Kind::grid,p,64,48,{.2f,.4f,.6f,.3f},1,true);check(std::abs(out[0]-.4)<1.e-6&&out[3]==.3f,"locked alpha keeps opacity and antialiasing in RGB");
 p.blend=Blend::normal;out=pixel(Kind::grid,p,64,48,{.2f,.4f,.6f,.3f},1,true);check(std::abs(out[0]-.52)<1.e-6&&out[3]==.3f,"locked source-over preserves source alpha without darkening source");
 for(int blend=0;blend<int(Blend::count);++blend){p.blend=Blend(blend);for(float v:pixel(Kind::checker,p,64,48,source))check(std::isfinite(v),"finite every blend");}
 Image image;int count=0;check(!render(Kind::grid,p,{0,0,1},64,64,image,nullptr,[&]{return ++count==3;})&&!image.valid(),"cancel never publishes a partial tile");
 p.width=std::numeric_limits<double>::quiet_NaN();check(!valid(p)&&!render(Kind::grid,p,{0,0,1},2,2,image),"invalid values rejected");
 std::cout<<"PASS: AE border/feather/phase; transparent cells; source blend/HDR/alpha; stencil; rotation/crop/tile invariance; cancellation.\n";
}
