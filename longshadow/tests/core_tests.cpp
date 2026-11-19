#include "longshadow_core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
using namespace longshadow;
namespace {
int checks;
void check(bool pass,const char* what){++checks;if(!pass){std::cerr<<"FAIL: "<<what<<'\n';std::exit(1);}}
void near(double a,double b,const char* what){check(std::abs(a-b)<2e-6,what);}
Image canvas(int w,int h){return {w,h,-5,18,std::vector<float>(std::size_t(w)*h*4)};}
float& at(Image& p,int x,int y,int k=3){return p.rgba[(std::size_t(y)*p.width+x)*4+k];}
float get(const Image& p,int x,int y){return x<0||y<0||x>=p.width||y>=p.height?0:p.rgba[(std::size_t(y)*p.width+x)*4+3];}
}
int main(int argc,char** argv){
 Parameters p;p.angle=0;p.length=5.5;p.opacity=80;p.color={.2,.4,.6};p.shadow_only=true;
 auto src=canvas(24,24);at(src,3,4)=.625;at(src,3,4,0)=3.5;
 Image out;
 check(render(src,p,out)&&out.left==-5&&out.top==18&&out.width==24&&out.height==24,"native coordinates");
 for(int y=0;y<24;y++)for(int x=0;x<24;x++)near(get(out,x,y),(y==4&&x>=3&&x<=8)?.5:y==4&&x==9?.25:0,"finite continuous horizontal shadow and subpixel endpoint");
 near(at(out,8,4,2),.6,"straight tint independent of alpha");
 p.angle=90;check(render(src,p,out),"downward shadow");
 near(get(out,3,10),.25,"vertical subpixel tail");near(get(out,4,8),0,"no width changes");
 p.angle=180;check(render(src,p,out),"leftward shadow");near(get(out,0,4),.5,"left boundary");near(get(out,4,4),0,"no reverse leak");
 p.angle=-90;check(render(src,p,out),"upward shadow");near(get(out,3,0),.5,"upper boundary");near(get(out,3,5),0,"no down shadow");
 p.angle=45;p.length=5*std::sqrt(2.);check(render(src,p,out),"diagonal shadow");
 for(int j=0;j<=5;j++){near(get(out,3+j,4+j),.5,"diagonal continuous");near(get(out,3+j,5+j),0,"no stray adjacent diagonal");}
 p.angle=0;p.length=4;p.fade=100;at(src,5,4)=.25;
 check(render(src,p,out),"distance-weighted shadow");
 near(get(out,4,4),.5*std::exp2(-1.5),"gradual fade");near(get(out,5,4),.2,"stronger recent caster");near(get(out,9,4),.2/64,"finite faded tail");
 check(render(src,p,src),"in-place rendering");check(src.rgba==out.rgba,"alias safe");
 src=canvas(24,24);at(src,3,7)=.625;at(src,3,7,0)=3.5;at(src,9,7)=.25;at(src,9,7,0)=.75;
 p.fade=0;p.length=8;p.shadow_only=false;
 check(render(src,p,out),"alpha compositing");
 near(get(out,9,7),.625,"source over projection alpha");
 near(at(out,9,7,0),(.25*.75+.5*.75*.2)/.625,"straight-alpha source-over");
 near(at(out,3,7,0),(3.5*.625+.5*.375*.2)/(.625+.5*.375),"HDR remains unclipped");
 p.crop=true;p.crop_left=45;
 check(render(src,p,out),"source crop");
 near(get(out,3,7),.625,"crop preserves source");near(get(out,7,7),0,"discarded caster no longer projects");near(get(out,13,7),.2,"retained caster projects");
 p.crop=false;p.shadow_only=true;p.expansion=1.5;
 check(render(src,p,out),"expanded shadow");
 near(get(out,6,8),.5,"expansion adds width");near(get(out,6,9),.25,"subpixel expansion");near(get(out,6,10),0,"bounded cross-section");
 p.expansion=0;p.angle=0;p.length=16;p.mask=2;p.mask_x=50;p.mask_y=31.25;p.mask_width=30;p.mask_height=30;p.mask_feather=25;
 check(render(src,p,out),"ellipse mask");
 near(get(out,6,7),0,"ellipse rejects shadow outside");near(get(out,10,7),.5,"ellipse retains interior");
 p.invert_mask=true;check(render(src,p,out),"inverted ellipse");
 near(get(out,6,7),.5,"inversion retains outside");near(get(out,10,7),0,"inversion discards inside");
 p.shadow_only=false;p.length=0;check(render(src,p,out)&&out.rgba==src.rgba,"length zero exact bypass");
 p.shadow_only=true;check(render(src,p,out)&&get(out,3,7)==0,"zero length shadow only blank");
 p=preset(0);
 auto stripe=canvas(120,16),proxy=canvas(30,4);
 for(int y=0;y<16;y++)for(int x=8;x<16;x++)at(stripe,x,y)=1;
 for(int y=0;y<4;y++)for(int x=2;x<4;x++)at(proxy,x,y)=1;
 p.length=36;p.angle=0;p.shadow_only=true;Image draft;
 check(render(stripe,p,out)&&render(proxy,p,draft,{},4),"4x draft");
 for(int x=0;x<30;x++)near(get(out,x*4+2,6),get(draft,x,1),"draft keeps native shadow length");
 p.opacity=std::numeric_limits<double>::quiet_NaN();check(!render(src,p,out),"nonfinite parameters rejected");
 p=preset(3);
 auto dense=canvas(768,512);
 for(int y=0;y<512;y++)for(int x=0;x<768;x++)if((x%47)<8&&(y%79)<48)at(dense,x,y)=.35f+(x%13)*.05f;
 std::ofstream fixture;
 if(argc>1)fixture.open(argv[1],std::ios::binary);
 const auto start=std::chrono::steady_clock::now();
 for(double angle:{-157.,-52.,13.,45.,77.,126.}){
  p.angle=angle;p.expansion=angle<15?0:3.5;p.crop=angle>100;p.crop_left=10;p.crop_right=90;p.fade=angle<30?75:0;
  check(render(dense,p,out),"oblique complete render");check(out.valid()&&out.rgba!=dense.rgba,"oblique effect nonneutral");
  if(argc>1)fixture.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));
 }
 if(argc>1)check(fixture.good(),"wrote determinism fixture");
 const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 const auto prior=out.rgba;int polls=0;check(!render(dense,p,out,[&]{return ++polls>20;})&&out.rgba==prior,"cancel leaves output untouched");
 auto painted=canvas(112,64);
 for(int y=16;y<48;y++)for(int x=5;x<17;x++){
  at(painted,x,y)=1;at(painted,x,y,y<32?0:2)=y<32?2.f:.75f;
 }
 // Hidden RGB must not leak through oblique interpolation or soft edges.
 at(painted,18,20,1)=40;
 p={};p.angle=0;p.length=72;p.opacity=100;p.shadow_only=true;p.color_mode=1;
 check(render(painted,p,out),"stretch colored silhouette");
 near(at(out,60,22,0),2,"red remains HDR down the ray");
 near(at(out,60,38,2),.75,"blue follows its own ray");
 near(at(out,60,20,1),0,"hidden RGB never casts");
 p.source_brightness=50;p.gap=20;
 check(render(painted,p,out),"separated colored shadow");
 near(get(out,24,22),0,"gap has no shadow before the start");
 near(get(out,25,22),1,"gap shifts start");
 near(at(out,75,22,0),1,"source brightness control preserves hue");
 near(get(out,108,22),1,"gap does not shorten the projection");
 p.gap=20.5;check(render(painted,p,out),"fractional gap");
 near(get(out,25,22),.5,"gap moves continuously between pixels");
 near(at(out,25,22,0),1,"fractional gap preserves source color");
 p.gap=0;p.source_brightness=100;p.angle=32;p.expansion=1.5;
 Image solid;
 check(render(painted,p,out),"source color along oblique expanded rays");
 p.color_mode=0;check(render(painted,p,solid),"solid geometry control");
 for(std::size_t i=0;i<out.rgba.size();i+=4){
  near(out.rgba[i+3],solid.rgba[i+3],"color does not change the shadow alpha");
  near(out.rgba[i+1],0,"hidden RGB excluded from expansion");
 }

 p.angle=0;p.expansion=0;p.blur_near=p.blur_far=8;
 check(render(painted,p,out),"uniform soft shadow");
 const auto uniform=get(out,27,14);
 check(uniform>.1f&&std::abs(uniform-get(out,70,14))<.001,"uniform blur on either portion");
 p.blur_near=0;p.blur_far=16;
 check(render(painted,p,out),"contact-hardening soft shadow");
 check(get(out,20,14)<.075f&&get(out,69,14)>.18f,"near is crisp and far has a broad penumbra");
 check(get(out,70,10)>get(out,35,10)+.05f,"blur reaches progressively farther from the edge");
 p.color_mode=1;
 check(render(painted,p,out),"progressively blurred source color");
 near(at(out,70,12,0),2,"soft red edge stays saturated");
 near(at(out,70,12,1),0,"soft edges do not blend with hidden RGB");
 near(at(out,70,50,2),.75,"blue survives the opposite soft edge");
 p.mask=1;p.mask_width=50;p.mask_height=50;
 check(render(painted,p,out),"mask after blur");
 near(get(out,70,12),0,"no blur outside final mask");
 check(get(out,61,22)>0,"masked shadow still has an interior");
 if(argc>1)fixture.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));
 p=preset(4);p.color_mode=1;
 const auto soft_start=std::chrono::steady_clock::now();
 check(render(dense,p,out),"large progressive color shadow");
 const auto soft_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-soft_start).count();
 if(argc>1){fixture.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));check(fixture.good(),"wrote new determinism fixtures");}
 // A circle with a 0-to-160px blur revealed a contour in the middle of the
 // old projected ramp. Measure abrupt slope changes along several rays.
 auto circle=canvas(1145,1061);
 for(int y=0;y<circle.height;y++)for(int x=0;x<circle.width;x++)
  at(circle,x,y)=float(std::clamp(233.5-std::hypot(x-337.,y-366.),0.,1.));
 p={};p.angle=45;p.length=365;p.opacity=82;p.blur_far=160;p.shadow_only=true;
 check(render(circle,p,out),"circle at maximum progressive radius");
 double bend=0;
 for(int side:{0,30,55}){
  auto ray=[&](int t){return double(get(out,337+t+side,366+t-side));};
  for(int t=211;t<370;t++)bend=std::max(bend,std::abs(ray(t+1)+ray(t-1)-2*ray(t)));
 }
 check(bend<.0004,"no abrupt brightness contour inside the long shadow");
 check(get(out,547,576)>.80f&&get(out,767,796)>.23f&&get(out,767,796)<.42f,"contact stays hard and tail spreads");
 if(argc>1){fixture.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));check(fixture.good(),"wrote circular regression fixture");}
 std::cout<<"PASS: "<<checks<<" checks; directional projection, source color/HDR, expansion, subpixel gap, uniform/progressive blur, final mask, draft and cancellation; six oblique renders "<<ms<<" ms; 768px color progressive render "<<soft_ms<<" ms; circular slope change "<<bend<<"\n";
}
