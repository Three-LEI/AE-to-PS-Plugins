#include "vector_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <thread>
using namespace vectorblur;
void check(bool b,const char* text){if(!b){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
Image image(int w=65,int h=65){return {w,h,0,0,std::vector<float>(std::size_t(w)*h*4,0)};}
void pixel(Image& a,int x,int y,float r,float g,float b,float alpha){const auto i=(std::size_t(y)*a.width+x)*4;a.rgba[i]=r;a.rgba[i+1]=g;a.rgba[i+2]=b;a.rgba[i+3]=alpha;}
Image ramp(){auto m=image();for(int y=0;y<65;++y)for(int x=0;x<65;++x)pixel(m,x,y,x/64.f,y/64.f,(64-x)/64.f,1);return m;}
double centroid(const Image& a,int axis){double sum=0,weight=0;for(int y=0;y<a.height;++y)for(int x=0;x<a.width;++x){const float v=a.rgba[(std::size_t(y)*a.width+x)*4+3];sum+=(axis?y:x)*v;weight+=v;}return sum/weight;}
int main(){
 auto src=image(),map=ramp();pixel(src,32,32,2,.4f,.1f,1);Image out;Parameters p;
 check(valid(p)&&render(src,p,out)&&out.rgba==src.rgba,"zero amount exact identity including HDR");
 p.amount=24;p.type=Type::constant;p.map_softness=0;p.property=Property::red;p.follow=false;p.quality=3;
 check(render(src,p,out,&map),"external map");
 const auto idx=[](int x,int y){return (std::size_t(y)*65+x)*4+3;};
 check(out.rgba[idx(22,32)]>0&&out.rgba[idx(32,22)]==0,"red ramp yields horizontal blur");
 check(std::abs(centroid(out,0)-32)<1.e-5,"centered integration remains centered");
 for(std::size_t i=0;i<out.rgba.size();i+=4)if(out.rgba[i+3]>1.e-5)check(std::abs(out.rgba[i]-2)<1.e-5,"associated interpolation preserves HDR colour on transparent edges");
 auto natural=p;natural.type=Type::natural;
 check(render(src,natural,out,&map)&&out.rgba[idx(42,32)]>0&&out.rgba[idx(22,32)]==0&&centroid(out,0)>40,"AE-calibrated Natural is a one-sided slope trail");
 natural.amount=-24;check(render(src,natural,out,&map)&&out.rgba[idx(22,32)]>0&&out.rgba[idx(42,32)]==0,"signed Natural reverses the trail");
 p.angle=90;check(render(src,p,out,&map)&&out.rgba[idx(32,22)]>0&&out.rgba[idx(22,32)]==0,"angle offset rotates field");p.angle=0;
 p.type=Type::perpendicular;p.ridge=.1;check(render(src,p,out,&map)&&out.rgba[idx(32,24)]>0,"perpendicular follows contours");
 p.property=Property::blue;p.type=Type::direction_fading;p.revolutions=0;p.angle=-90;check(render(src,p,out,&map)&&centroid(out,0)<29,"one-sided fading direction");
 p.amount=-24;check(render(src,p,out,&map)&&centroid(out,0)>35,"negative amount reverses directional blur");
 p.type=Type::direction_center;check(render(src,p,out,&map)&&std::abs(centroid(out,0)-32)<1.e-4,"direction center is bidirectional");
 p.preserve_alpha=true;check(render(src,p,out,&map),"preserve alpha");for(std::size_t i=3;i<src.rgba.size();i+=4)check(out.rgba[i]==src.rgba[i],"source alpha exact");p.preserve_alpha=false;
 p.mix=0;check(render(src,p,out,&map)&&out.rgba==src.rgba,"zero mix exact identity");p.mix=100;
 Field f;p=Parameters{};p.map_softness=0;p.property=Property::green;p.type=Type::constant;
 check(make_field(src,p,f,&map)&&f.at(32,32).y>.99&&std::abs(f.at(32,32).x)<1.e-5,"green property selects vertical slope");
 p.property=Property::blue;check(make_field(src,p,f,&map)&&f.at(32,32).x<-.99,"blue property negative slope");p.invert_map=true;check(make_field(src,p,f,&map)&&f.at(32,32).x>.99,"invert vector map");
 auto unit_ramp=map;for(int y=0;y<65;++y)for(int x=0;x<65;++x)pixel(unit_ramp,x,y,x/255.f,0,0,1);
 p=Parameters{};p.property=Property::red;p.map_softness=0;
 check(make_field(src,p,f,&unit_ramp)&&std::abs(f.at(32,32).x-.89442719f)<1.e-5,"linear-map response agrees with measured AE slope/ridge profile");
 p=Parameters{};p.amount=60;p.map_softness=2;auto hidden=image();for(std::size_t i=0;i<hidden.rgba.size();i+=4)hidden.rgba[i]=100;
 check(render(hidden,p,out)&&std::all_of(out.rgba.begin(),out.rgba.end(),[](float v){return v==0;}),"hidden transparent RGB never emits");
 for(int k=0;k<8;++k){p.property=Property(k);for(int t=0;t<5;++t){p.type=Type(t);check(render(src,p,out,&map),"all property/type combinations");for(float v:out.rgba)check(std::isfinite(v),"finite output");}}
 auto dc=image();for(int y=0;y<65;++y)for(int x=0;x<65;++x)pixel(dc,x,y,.25f,.5f,3,1);
 for(int edge=1;edge<4;++edge){p.edge=Edge(edge);check(render(dc,p,out,&map),"edge mode");for(std::size_t i=0;i<out.rgba.size();i+=4)check(std::abs(out.rgba[i+2]-3)<1.e-5&&std::abs(out.rgba[i+3]-1)<1.e-6,"constant energy and HDR with repeat/reflect/wrap");}
 p.edge=Edge::transparent;p.linear_light=true;check(render(dc,p,out,&map),"linear-light path");for(float v:out.rgba)check(std::isfinite(v),"linear-light finite");
 // Circular hue interpolation must keep the red wrap near zero, rather than turn it cyan.
 auto hue_map=image();for(int y=0;y<65;++y)for(int x=0;x<65;++x)pixel(hue_map,x,y,1,0,x<32?.06f:0.f,1);
 p=Parameters{};p.type=Type::direction_center;p.property=Property::hue;p.map_softness=8;
 check(make_field(src,p,f,&hue_map)&&f.at(32,32).y<-.99,"hue wrap is smoothed circularly");
 auto radial=image();for(int y=0;y<65;++y)for(int x=0;x<65;++x){float t=std::min(1.f,std::hypot(x-32.f,y-32.f)/40);pixel(radial,x,y,t,t,t,1);}
 auto mark=image();pixel(mark,45,32,1,1,1,1);p=Parameters{};p.type=Type::perpendicular;p.map_softness=2;p.amount=50;p.ridge=.1;p.quality=3;
 Image straight;p.follow=true;check(render(mark,p,out,&radial),"curved flow render");p.follow=false;check(render(mark,p,straight,&radial)&&out.rgba!=straight.rgba,"curved flow differs from straight sampling around contours");
 p.property=Property::alpha;check(make_field(mark,p,f)&&std::isfinite(f.at(45,32).x),"alpha field");
 auto extreme=map;for(std::size_t i=0;i<extreme.rgba.size();i+=4)extreme.rgba[i]=(i&4)?std::numeric_limits<float>::max():-std::numeric_limits<float>::max();
 for(int t=0;t<5;++t){p.type=Type(t);p.property=Property::red;check(make_field(src,p,f,&extreme),"HDR vector-map input");for(auto v:f.vectors)check(std::isfinite(v.x)&&std::isfinite(v.y),"bounded HDR map field");}
 p.amount=std::numeric_limits<double>::quiet_NaN();check(!valid(p)&&!render(src,p,out)&&out.rgba.empty(),"invalid params do not publish partial output");
 p=Parameters{};p.amount=200;auto big=image(512,256);int polls=0;check(!render(big,p,out,nullptr,[&]{return ++polls>20;})&&out.rgba.empty(),"cancel before output publish");
 // Cancellation during parallel integration must also poll on the initiating thread.
 p.map_softness=0;p.type=Type::direction_center;p.amount=0;polls=0;Field diagnostic;
 check(render(big,p,out,nullptr,[&]{++polls;return false;},&diagnostic),"parallel test preparation");const int preparation_polls=polls;p.amount=200;polls=0;const auto thread=std::this_thread::get_id();
 check(!render(big,p,out,nullptr,[&]{check(std::this_thread::get_id()==thread,"cancel callback stays on calling thread");return ++polls>preparation_polls;})&&out.rgba.empty(),"parallel cancellation");
 std::cout<<"PASS: identity; five modes / eight properties; field direction; signed amount; alpha / hidden RGB; HDR / edges; mix; cancel; thread-safe host polling.\n";
}
