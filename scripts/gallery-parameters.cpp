// Build complete, reproducible native Action Manager parameters for the gallery.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "../native/include/tint_core.hpp"
#include "../fractal/include/fractal_core.hpp"
#include "../displacement/include/displacement_core.hpp"
#include "../glow/include/glow_core.hpp"
#include "../chromatic/include/chromatic_core.hpp"
#include "../vector/include/vector_core.hpp"
#include "../patterns/include/pattern_core.hpp"
#include "../cell/include/cell_core.hpp"
#include "../lens/include/lens_core.hpp"
#include "../jpeg/include/jpeg_core.hpp"
#include "../lightfx/include/lightfx_core.hpp"
#include "../energy/include/energy_core.hpp"
#include "../blinds/include/blinds_core.hpp"
#include "../native/build/parameter_schema.hpp"
#include "../fractal/build/parameter_schema.hpp"
#include "../displacement/build/parameter_schema.hpp"
#include "../glow/build/parameter_schema.hpp"
#include "../chromatic/build/parameter_schema.hpp"
#include "../vector/build/parameter_schema.hpp"
#include "../patterns/build/parameter_schema.hpp"
#include "../cell/build/parameter_schema.hpp"
#include "../lens/build/parameter_schema.hpp"
#include "../jpeg/build/parameter_schema.hpp"
#include "../lightfx/build/parameter_schema.hpp"
#include "../energy/build/parameter_schema.hpp"
#include "../blinds/build/parameter_schema.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
struct Archive {
 std::ostream& out;bool comma=false;
 void head(const char* id,const char* kind){if(comma)out<<',';comma=true;out<<"{\"id\":\""<<id<<"\",\"kind\":\""<<kind<<"\",\"value\":";}
 void number(const char* id,double value){head(id,"number");out<<std::setprecision(17)<<value<<'}';}
 template<class T>void integer(const char* id,T value){head(id,"integer");out<<static_cast<long long>(value)<<'}';}
 void flag(const char* id,bool value){head(id,"flag");out<<(value?"true":"false")<<'}';}
 void layer(const char* id,std::uint64_t value,std::uint32_t* =nullptr){head(id,"layer");out<<'"'<<(value?"DemoMap":"")<<"\"}";}
};
bool comma=false;
template<class P>void emit(std::ostream& out,const char* slug,const char* event,unsigned magic,P p,const char* input="photo",int version=1,const energy::Path* path=nullptr){
 if(comma)out<<',';comma=true;out<<"{\"slug\":\""<<slug<<"\",\"event\":\""<<event<<"\",\"input\":\""<<input<<"\",\"fields\":[";
 Archive a{out};a.integer("ZpSv",version);a.integer("ZpId",magic);psfilter::parameters::Schema<P>::visit(a,p);out<<']';
 if(path){out<<",\"geometry\":\"";const char* hex="0123456789abcdef";for(auto v:energy::encode_path(*path))out<<hex[v>>4]<<hex[v&15];out<<'"';}out<<'}';
}
int main(int argc,char** argv){if(argc!=2)return 2;std::ofstream out(argv[1]);out<<'[';constexpr int w=768,h=768;
 tint::Parameters t;t.black[0]=.035;t.black[1]=.075;t.black[2]=.16;t.white[0]=1;t.white[1]=.85;t.white[2]=.65;emit(out,"tint","ZpTi",0x54494E31,t);
 fractal::Parameters f;f.contrast=165;f.scale=145;f.complexity=5;emit(out,"fractal-noise","ZpFn",0x46524E32,f);
 displacement::Parameters d;d.layer_name_key=1;d.amount_x=38;d.amount_y=17;emit(out,"displacement-map","ZpDm",0x5A445031,d,"map");
 auto g=glow::defaults(glow::Kind::classic);g.threshold=56;g.radius=32;g.intensity=.9;emit(out,"glow","ZpGw",0x5A474C31,g);
 g=glow::defaults(glow::Kind::deep);g.radius=100;g.intensity=.8;g.threshold=25;g.smoothness=25;emit(out,"deep-glow","ZpDg",0x5A444731,g);
 chromatic::Parameters c;c.mode=chromatic::Mode::warp;c.warp[0].scale=103;c.warp[1].scale=97;c.warp[0].rotation=.7;c.warp[1].rotation=-.7;c.edge=chromatic::Edge::repeat;emit(out,"chromatic-dispersion","ZpCd",0x5A434431,c);
 auto v=vectorblur::preset(1);v.amount=18;v.map_softness=8;v.mix=75;emit(out,"vector-blur","ZpVb",0x5A564231,v);
 auto p=patterns::defaults(patterns::Kind::grid,w,h);p.size_from=patterns::SizeFrom::width_height;p.width=64;p.height=64;p.border=2;p.color[0]=.2;p.color[1]=.8;p.color[2]=1;p.opacity=80;p.blend=patterns::Blend::normal;emit(out,"grid","ZpGr",0x5A475231,p);
 p=patterns::defaults(patterns::Kind::checker,w,h);p.size_from=patterns::SizeFrom::width_height;p.width=96;p.height=96;p.color[0]=.2;p.color[1]=.8;p.color[2]=1;p.opacity=55;p.blend=patterns::Blend::normal;emit(out,"checkerboard","ZpCh",0x5A434B31,p);
 auto ce=cell::defaults(w,h);ce.mode=cell::Mode::bubbles;ce.size=35;emit(out,"cell-pattern","ZpCe",0x5A434531,ce);
 auto l=lens::defaults(lens::Kind::optics,w,h);l.fov=78;l.reverse=true;emit(out,"optics-compensation","ZpOc",0x5A4C4F31,l);
 l=lens::defaults(lens::Kind::cc,w,h);l.size=66;l.convergence=55;emit(out,"lens","ZpLn",0x5A4C4331,l);
 jpegfx::Parameters j;j.quality=10;j.error_rate=.08;j.density=20;j.color_error=10;j.error_amp=.45;j.mix=90;j.seed=327;emit(out,"jpeg-damage","ZpJd",0x5A4A4431,j);
 auto star=lightfx::preset(lightfx::defaults(lightfx::Kind::starglow,w,h),0);star.threshold=.97;star.softness=.01;star.length=180;star.boost=1.8;emit(out,"starglow","ZpSg",0x5A535431,star,"glints");
 auto shine=lightfx::defaults(lightfx::Kind::shine,w,h);shine.cx=280;shine.cy=150;shine.threshold=.48;shine.length=1.6;shine.boost=1.5;shine.blend=1;shine.effect_opacity=100;shine.mix=80;emit(out,"shine","ZpSh",0x5A534831,shine);
 auto e=energy::unified_preset(1,energy::unified_defaults(w,h));e.input=1;e.composite=0;e.width=2.4;e.glow_intensity=.65;
 const double cx=384,cy=384,rx=285,ry=332,k=.5522847498307936;
 energy::Path path{{{true,{{{cx-rx*k,cy-ry},{cx,cy-ry},{cx+rx*k,cy-ry}},{{cx+rx,cy-ry*k},{cx+rx,cy},{cx+rx,cy+ry*k}},{{cx+rx*k,cy+ry},{cx,cy+ry},{cx-rx*k,cy+ry}},{{cx-rx,cy+ry*k},{cx-rx,cy},{cx-rx,cy-ry*k}}}}}};
 emit(out,"saber","ZpEb",0x5A454231,e,"photo",6,&path);
 auto b=blinds::defaults(w,h);b.completion=45;b.direction=28;b.width=48;b.feather=1.5;emit(out,"venetian-blinds","ZpVl",0x5A574231,b);
 for(const auto& item:{std::pair{"blobbylize",0},std::pair{"glass",1}}){out<<",{\"slug\":\""<<item.first<<"\",\"event\":\""<<(item.second?"ZyGl":"ZyBl")<<"\",\"input\":\"photo\",\"legacy\":\"surface\",\"kind\":"<<item.second<<",\"fields\":[]}";}
 for(const auto& item:{std::pair{"turbulent-displace",0},std::pair{"light-sweep",1},std::pair{"kaleida",2}}){out<<",{\"slug\":\""<<item.first<<"\",\"event\":\""<<(item.second==0?"ZyTD":item.second==1?"ZyLS":"ZyKa")<<"\",\"input\":\"photo\",\"legacy\":\"creative\",\"kind\":"<<item.second<<",\"fields\":[]}";}
 out<<",{\"slug\":\"roughen-edges\",\"event\":\"ZyRE\",\"input\":\"alpha\",\"legacy\":\"roughen\",\"fields\":[]}]\n";
 std::cout<<"Wrote 23 complete gallery parameter profiles\n";return out?0:3;
}
