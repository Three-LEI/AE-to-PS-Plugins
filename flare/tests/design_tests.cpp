#include "flare_design.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace flare;
int checks=0;void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
Image source(int w,int h){Image i{w,h,0,0,std::vector<float>(std::size_t(w)*h*4,0)};for(std::size_t n=3;n<i.rgba.size();n+=4)i.rgba[n]=1;return i;}
int main(int argc,char** argv){try{
 auto src=source(256,160);Image old,out;Parameters p;Design d;
 for(int i=0;i<8;i++){p=preset(i,256,160);p.blend=1;d=design_from_legacy(p);check(valid_design(d),"converted design valid");check(render(src,p,old)&&render(src,p,out,nullptr,{}, {},&d),"converted design render");for(std::size_t n=0;n<out.rgba.size();n++)check(std::abs(old.rgba[n]-out.rgba[n])<1e-5,"conversion retains legacy look");
  auto bytes=encode_design(d);Design copy;check(decode_design(bytes,copy)&&copy==d,"all element properties round trip");
  auto saved=encode_preset({p,d});FlarePreset preset;check(decode_preset(saved,preset)&&preset.design==d,"preset round trip");auto receiving=p;Design restored;apply_preset(preset,receiving,restored);check(render(src,receiving,out,nullptr,{}, {},&restored),"preset renders");for(std::size_t n=0;n<out.rgba.size();n++)check(std::abs(old.rgba[n]-out.rgba[n])<1e-5,"preset retains global look");
 }
 d.enabled=true;d.elements.clear();p=defaults(256,160);p.blend=2;check(render(src,p,out,nullptr,{}, {},&d),"empty active design");for(auto v:out.rgba)check(v==0,"empty design has no emission");
 for(int type=0;type<element_type_count;type++){d.elements={make_element(type)};d.elements[0].name="鎏金_元素";d.elements[0].custom_name=true;check(render(src,p,out,nullptr,{}, {},&d),"single object renders");double energy=0;for(auto v:out.rgba)energy+=v;check(energy>0,"object visible");d.elements[0].enabled=false;check(render(src,p,out,nullptr,{}, {},&d),"hidden object renders");for(auto v:out.rgba)check(v==0,"hidden object has no light");}
 d=design_from_legacy(defaults(256,160));auto bytes=encode_design(d);Design sentinel=d,copy=d;
 for(const std::string& glyph:{std::string("鎏"),std::string("\xf0\x9f\x8c\x9f")}){auto e=make_element(2);e.custom_name=true;e.name.clear();while(e.name.size()+glyph.size()<=96)e.name+=glyph;auto duplicate=duplicate_element(e);Design named{true,{e,duplicate}};Design restored;check(valid_design(named)&&decode_design(encode_design(named),restored)&&restored==named,"duplicating long Unicode names stays valid and persists");check(static_cast<ElementValues&>(e)==static_cast<ElementValues&>(duplicate),"duplicate preserves independent appearance");}
 auto extreme=defaults(256,160);extreme.ghost_size=.01;extreme.ghost_spacing=10;extreme.ghosts=24;check(valid_design(design_from_legacy(extreme)),"tiny distant legacy ghosts convert safely");
 for(std::size_t n=0;n<bytes.size();n+=17){auto truncated=bytes;truncated.resize(n);check(!decode_design(truncated,copy)&&copy==sentinel,"truncated decode is atomic");}
 auto corrupt=bytes;corrupt[4]=3;check(!decode_design(corrupt,copy)&&copy==sentinel,"future design rejected");corrupt=bytes;corrupt.push_back(0);check(!decode_design(corrupt,copy),"trailing bytes rejected");
 d.elements[0].name="\xc0\xaf";check(!valid_design(d)&&encode_design(d).empty(),"overlong UTF8 rejected");d.elements[0]=make_element(0);d.elements.resize(65);check(!valid_design(d),"element limit");d.elements.resize(64);check(valid_design(d)&&decode_design(encode_design(d),copy),"64 elements supported");
 p=defaults(256,160);p.hx=10;p.hy=-20;p.map_key=42;p.occlusion=2;p.blend=2;p.mix=77;const auto original=p;FlarePreset value{defaults(400,300),design_from_legacy(defaults(400,300))};value.style.brightness=2.5;apply_preset(value,p,d);check(p.hx==original.hx&&p.hy==original.hy&&p.px==original.px&&p.py==original.py&&p.map_key==42&&p.occlusion==2&&p.blend==2&&p.mix==77&&p.brightness==2.5,"loading a look preserves document/output bindings");
 for(int type=0;type<element_type_count;type++){auto e=make_element(type);const auto en=element_name(e,false),zh=element_name(e,true);check(!en.empty()&&!zh.empty()&&en!=zh,"automatic names follow language");auto duplicate=duplicate_element(e,true);check(element_name(duplicate,true).find("副本")!=std::string::npos,"automatic copy follows language");e.name="My 鎏 / custom";e.custom_name=true;check(element_name(e,true)==e.name&&element_name(e,false)==e.name,"custom name never translated or split");}
 for(int i=0;i<builtin_preset_count;i++){auto value=design_preset(i,256,160);FlarePreset restored;check(valid_design(value.design)&&decode_preset(encode_preset(value),restored)&&restored.design==value.design,"all twelve preset designs persist");check(render(src,value.style,out,nullptr,{}, {},&value.design),"all twelve presets render");for(auto v:out.rgba)check(std::isfinite(v),"new elements finite");}
 auto newsrc=source(384,240);p=defaults(384,240);p.blend=2;for(int type=6;type<element_type_count;type++){d={true,{make_element(type)}};Image first,second;check(render(newsrc,p,first,nullptr,{}, {},&d),"new object baseline");d.elements[0].rotation=37;check(render(newsrc,p,second,nullptr,{}, {},&d),"new object parameter edit");if(type!=10)check(first.rgba!=second.rgba,"oriented/scattered new element changes with rotation");}
 std::ofstream raw(argc>1?argv[1]:"design.f32",std::ios::binary);auto medium=source(513,257);
 for(int i=0;i<builtin_preset_count;i++){auto value=design_preset(i,513,257);p=value.style;d=value.design;auto extra=make_element(6+i%6);extra.axis=1.4;extra.x=7;extra.y=-5;extra.color={.2,.9,.5};d.elements.push_back(extra);d.elements[0].name="鎏金";check(render(medium,p,out,nullptr,{}, {},&d),"designer float fixture");raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));}check(raw.good(),"designer fixtures written");
 std::cout<<"PASS: "<<checks<<" design assertions; conversion, independent objects, complete codec, portable look files, Unicode/bounds/corruption and 12 float fixtures\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
