#include "flare_design.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
namespace flare {namespace {
bool range(double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;}
bool utf8(const std::string& s){
 if(s.size()>96)return false;
 for(std::size_t i=0;i<s.size();){const auto a=static_cast<unsigned char>(s[i++]);if(a<32||a==127)return false;if(a<128)continue;
  int n=a>=0xc2&&a<=0xdf?1:a>=0xe0&&a<=0xef?2:a>=0xf0&&a<=0xf4?3:0;if(!n||i+n>s.size())return false;std::uint32_t c=a&((1u<<(6-n))-1);
  for(int j=0;j<n;j++){const auto b=static_cast<unsigned char>(s[i++]);if((b&0xc0)!=0x80)return false;c=(c<<6)|(b&63);}
  if(c<(n==1?128u:n==2?2048u:65536u)||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return false;
 }return true;
}
struct Writer {
 std::vector<std::uint8_t> b;
 void integer(std::uint32_t n){for(int i=0;i<4;i++)b.push_back(std::uint8_t(n>>(i*8)));}
 void real(double n){const auto bits=std::bit_cast<std::uint64_t>(n);for(int i=0;i<8;i++)b.push_back(std::uint8_t(bits>>(i*8)));}
 void text(const std::string& s){integer(std::uint32_t(s.size()));b.insert(b.end(),s.begin(),s.end());}
};
struct Reader {
 const std::vector<std::uint8_t>& b;std::size_t at=0;bool ok=true;
 std::uint32_t integer(){if(at+4>b.size()){ok=false;return 0;}std::uint32_t v=0;for(int i=0;i<4;i++)v|=std::uint32_t(b[at++])<<(i*8);return v;}
 double real(){if(at+8>b.size()){ok=false;return 0;}std::uint64_t v=0;for(int i=0;i<8;i++)v|=std::uint64_t(b[at++])<<(i*8);return std::bit_cast<double>(v);}
 std::string text(){auto n=integer();if(n>96||n>b.size()-at){ok=false;return {};}std::string s(b.begin()+at,b.begin()+at+n);at+=n;return s;}
};
template<class F>void global_fields(Parameters& p,F f){for(double* v:{&p.brightness,&p.scale,&p.aspect,&p.gamma,&p.saturation,&p.hue,&p.color[0],&p.color[1],&p.color[2],&p.atmosphere,&p.noise_scale,&p.edge_width,&p.edge_bright,&p.center_radius,&p.center_bright})f(*v);}
}
bool valid_design(const Design& d){
 if(d.elements.size()>max_elements)return false;
 for(const auto& e:d.elements){
  if(!utf8(e.name)||e.type<0||e.type>=element_type_count||e.count<0||e.count>64||e.blades<3||e.blades>16||e.name_index<0||e.name_index>1000000||e.copies<0||e.copies>1000000)return false;
  if(!range(e.brightness,0,20)||!range(e.size,.0001,20)||!range(e.aspect,.05,20)||!range(e.axis,-64,64)||!range(e.x,-1000,1000)||!range(e.y,-1000,1000)||!range(e.rotation,-1e9,1e9)||!range(e.softness,0,10)||!range(e.spread,.01,10)||!range(e.chromatic,0,100))return false;
  for(auto c:e.color)if(!range(c,0,1))return false;
 }return true;
}
Element make_element(int type){Element e;e.type=std::clamp(type,0,element_type_count-1);if(type==2||type==3)e.softness=1;if(type==5){e.axis=1.3;e.size=.5;e.brightness=.18;}if(type==6){e.spread=.65;e.softness=.4;e.chromatic=40;}if(type==7){e.count=7;e.axis=1.4;e.size=.5;e.brightness=.25;e.spread=2.5;e.softness=.15;e.chromatic=60;}if(type==8){e.count=4;e.softness=.7;}if(type==9){e.count=24;e.spread=2;e.softness=.6;e.brightness=.7;}if(type==10){e.count=3;e.softness=.5;e.spread=1.1;}if(type==11){e.softness=.5;e.spread=1.8;e.aspect=.4;e.chromatic=100;e.brightness=.7;}return e;}
std::string element_name(const Element& e,bool chinese){if(e.custom_name)return e.name;constexpr const char* zh[]={"热点","柔光","射线","拉丝","光环","光圈","光弧","多重光圈","星耀","闪烁光点","线环","光谱色带"};constexpr const char* en[]={"Hotspot","Soft Glow","Rays","Streak","Spectral Ring","Aperture","Arc","Multi Aperture","Glint","Sparkle","Hoop","Spectrum Band"};const int type=std::clamp(e.type,0,element_type_count-1);std::string result=type==5&&e.name_index>0?(chinese?"鬼影 ":"Ghost ")+std::to_string(e.name_index):(chinese?zh[type]:en[type]);if(e.copies>0)result+=(chinese?" 副本 ":" Copy ")+std::to_string(e.copies);return result;}
Element duplicate_element(const Element& original,bool chinese){auto copy=original;if(!copy.custom_name){copy.copies=std::min(1000000,copy.copies+1);return copy;}const std::string suffix=chinese?" 副本":" copy";auto length=std::min<std::size_t>(96-suffix.size(),original.name.size());while(length<original.name.size()&&(static_cast<unsigned char>(original.name[length])&0xc0)==0x80)--length;copy.name=original.name.substr(0,length)+suffix;return copy;}
std::vector<std::uint8_t> encode_design(const Design& d){
 if(!valid_design(d))return {};Writer w;w.integer(0x31444641);w.integer(2);w.integer(d.enabled?1:0);w.integer(std::uint32_t(d.elements.size()));
 for(const auto& e:d.elements){w.text(e.name);w.integer(e.custom_name?1:0);w.integer(e.name_index);w.integer(e.copies);w.integer(e.type);w.integer(e.enabled?1:0);w.integer(e.count);w.integer(e.blades);for(auto v:{e.brightness,e.size,e.aspect,e.axis,e.x,e.y,e.rotation,e.softness,e.spread,e.chromatic,e.color[0],e.color[1],e.color[2]})w.real(v);}
 return std::move(w.b);
}
bool decode_design(const std::vector<std::uint8_t>& bytes,Design& out){
 if(bytes.size()<16||bytes.size()>max_design_bytes)return false;Reader r{bytes};if(r.integer()!=0x31444641||r.integer()!=2)return false;const auto enabled=r.integer(),count=r.integer();if(enabled>1||count>max_elements)return false;Design next;next.enabled=enabled!=0;
 for(std::uint32_t i=0;i<count&&r.ok;i++){Element e;e.name=r.text();auto custom=r.integer();if(custom>1)return false;e.custom_name=custom!=0;e.name_index=int(r.integer());e.copies=int(r.integer());e.type=int(r.integer());auto visible=r.integer();if(visible>1)return false;e.enabled=visible!=0;e.count=int(r.integer());e.blades=int(r.integer());for(double* v:{&e.brightness,&e.size,&e.aspect,&e.axis,&e.x,&e.y,&e.rotation,&e.softness,&e.spread,&e.chromatic,&e.color[0],&e.color[1],&e.color[2]})*v=r.real();next.elements.push_back(std::move(e));}
 if(!r.ok||r.at!=bytes.size()||!valid_design(next))return false;out=std::move(next);return true;
}
std::vector<std::uint8_t> encode_preset(const FlarePreset& input){
 if(!valid(input.style)||!valid_design(input.design)||!input.design.enabled)return {};auto design=encode_design(input.design);Writer w;w.integer(0x31504641);w.integer(1);w.integer(std::uint32_t(design.size()));w.b.insert(w.b.end(),design.begin(),design.end());auto p=input.style;global_fields(p,[&](double& v){w.real(v);});w.integer(p.seed);return std::move(w.b);
}
bool decode_preset(const std::vector<std::uint8_t>& bytes,FlarePreset& out){
 if(bytes.size()<12||bytes.size()>max_design_bytes+256)return false;Reader r{bytes};if(r.integer()!=0x31504641||r.integer()!=1)return false;const auto size=r.integer();if(size>bytes.size()-r.at)return false;
 FlarePreset next;std::vector<std::uint8_t> design(bytes.begin()+r.at,bytes.begin()+r.at+size);r.at+=size;if(!decode_design(design,next.design)||!next.design.enabled)return false;
 global_fields(next.style,[&](double& v){v=r.real();});next.style.seed=r.integer();if(!r.ok||r.at!=bytes.size()||!valid(next.style))return false;out=std::move(next);return true;
}
void apply_preset(const FlarePreset& preset,Parameters& p,Design& d){auto copy=preset.style;
 p.brightness=copy.brightness;p.scale=copy.scale;p.aspect=copy.aspect;p.gamma=copy.gamma;p.saturation=copy.saturation;p.hue=copy.hue;p.color=copy.color;p.atmosphere=copy.atmosphere;p.noise_scale=copy.noise_scale;p.seed=copy.seed;p.edge_width=copy.edge_width;p.edge_bright=copy.edge_bright;p.center_radius=copy.center_radius;p.center_bright=copy.center_bright;d=preset.design;
}
FlarePreset design_preset(int index,int width,int height,int left,int top){
 const int id=std::clamp(index,0,builtin_preset_count-1);auto p=preset(id<8?id:0,width,height,left,top);if(id<8)return {p,design_from_legacy(p)};
 Design d;d.enabled=true;auto add=[&](int type,double bright,double size,Color color){auto e=make_element(type);e.brightness=bright;e.size=size;e.color=color;d.elements.push_back(e);return &d.elements.back();};
 add(0,1,.8,{.8,.93,1});add(1,.55,1,{.4,.65,1});
 if(id==8){d.elements.clear();add(0,.9,.8,{1,.85,.5});add(1,.5,1.2,{1,.5,.15});auto* e=add(6,.7,1.4,{1,.7,.3});e->rotation=-35;e->spread=.72;e=add(10,.2,.65,{1,.75,.45});e->count=3;e->axis=2;e->chromatic=35;}
 if(id==9){auto* e=add(7,.3,1,{.55,.8,1});e->count=9;e->spread=2.2;e->blades=8;e=add(3,1.1,1,{.3,.6,1});e->softness=.5;e->spread=2.8;}
 if(id==10){auto* e=add(9,1.3,2.2,{.55,.8,1});e->count=32;e->spread=2.6;add(8,.65,1.2,{.8,.92,1});add(4,.15,1.8,{.5,.65,1});}
 if(id==11){auto* e=add(11,1,2,{1,1,1});e->rotation=-18;e->axis=1.5;e=add(6,.55,1.7,{.7,.9,1});e->rotation=140;e->spread=.55;e->chromatic=90;}
 return {p,std::move(d)};
}
}
