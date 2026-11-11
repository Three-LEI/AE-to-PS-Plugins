#include "displacement_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
using namespace displacement;
void check(bool q,const char* s){if(!q){std::cerr<<"FAIL: "<<s<<'\n';std::exit(1);}}
Image solid(int w,int h,float value){Image im;im.width=w;im.height=h;im.rgba.resize(w*h*4,value);for(int i=0;i<w*h;++i)im.rgba[i*4+3]=1;return im;}
int main(){
 auto source=solid(9,7,0);source.left=11;source.top=-3;
 for(int y=0;y<7;++y)for(int x=0;x<9;++x)for(int k=0;k<3;++k)source.rgba[(y*9+x)*4+k]=float(x+10*y+k);
 auto map=solid(9,7,.5f);Parameters p;p.amount_x=p.amount_y=8;
 std::vector<float> out(9*7*4);Region region{11,-3,1,9,7};
 check(render(source,map,p,region,out.data())&&out==source.rgba,"neutral map exactly preserves HDR source");
 std::fill(map.rgba.begin(),map.rgba.end(),1);p.amount_x=2;p.amount_y=0;
 check(render(source,map,p,region,out.data()),"positive displacement");
 check(out[(3*9+4)*4]==source.rgba[(3*9+2)*4],"positive map shifts content right by specified pixels");
 check(out[3]==0,"nonwrapping edge becomes transparent");
 p.wrap=true;check(render(source,map,p,region,out.data())&&out[0]==source.rgba[7*4],"wrapped left edge reads opposite side");
 p.amount_x=-2;check(render(source,map,p,region,out.data())&&out[0]==source.rgba[2*4],"negative amount reverses shift");
 map=solid(1,1,1);p.amount_x=2;p.wrap=false;p.placement=Placement::center;
 check(render(source,map,p,region,out.data())&&out[0]==source.rgba[0],"centered small map leaves exterior unchanged");
 p.placement=Placement::tile;check(render(source,map,p,region,out.data())&&out[3]==0,"tiled small map covers full source");
 map=solid(9,7,128.f/255);map.midpoint=128./255;p.amount_x=99;p.amount_y=99;
 check(render(source,map,p,region,out.data()),"8-bit neutral render");
 check(std::abs(out[(3*9+4)*4]-source.rgba[(3*9+4)*4])<1e-4,"128 is 8-bit neutral");
 p.horizontal=Channel::none;p.vertical=Channel::none;
 check(render(source,map,p,region,out.data())&&out==source.rgba,"disabled channels preserve source");
 p.horizontal=Channel::red;p.vertical=Channel::green;p.amount_x=1.37;p.amount_y=-2.4;p.wrap=true;
 map=solid(3,4,1);p.placement=Placement::stretch;
 check(render(source,map,p,region,out.data()),"stretch render");
 for(int y=0;y<7;++y)for(int x=0;x<9;++x){float pixel[4];check(render(source,map,p,{11.+x,-3.+y,1,1,1},pixel),"one-pixel tile");for(int k=0;k<4;++k)check(pixel[k]==out[(y*9+x)*4+k],"arbitrary tiles share global coordinates");}
 source=solid(2,1,0);source.rgba={1,0,0,1,0,0,1,0};map=solid(2,1,1);p.amount_x=-.5;p.amount_y=0;p.wrap=false;
 float edge[4];check(render(source,map,p,{0,0,1,1,1},edge)&&edge[0]==1&&edge[2]==0&&edge[3]==.5f,"premultiplied interpolation prevents transparent RGB fringes");
 p.amount_x=std::nan("");check(!valid(p),"reject nonfinite amounts");
 std::cout<<"PASS: neutral/8-bit midpoint; exact HDR identity; signed shifts; transparent/wrapped edges; center/stretch/tile; global tiled equivalence; premultiplied alpha.\n";
}
