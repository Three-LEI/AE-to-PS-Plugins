#include "flare_host.hpp"
#include "parameter_schema.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
flare::Parameters fixture(int i){auto p=flare::preset(i,320,240);p.hx=85+i*10;p.hy=65+i*7;p.px=180;p.py=125;p.blend=i%3;p.background=i==2?35:100;p.preserve_alpha=i==3;p.occlusion=i==5?2:0;return p;}
flare::Design fixture_design(int i,const flare::Parameters& p){if(i<3)return {};auto d=flare::design_from_legacy(p);auto e=flare::make_element(6+(i-3)*2);e.axis=1.2;e.x=8;e.y=-3;e.color={.25,.9,.65};e.name="鎏金_自定义元素";e.custom_name=true;d.elements.push_back(e);if(i==5){auto multi=flare::make_element(7);multi.count=4;d.elements.push_back(multi);d.elements.push_back(flare::make_element(9));d.elements.push_back(flare::make_element(11));}d.elements[0].size*=1.3;return d;}
struct Archive {
 std::ostream& out;bool comma=false;
 void key(const char* id,const char* type){if(comma)out<<',';comma=true;out<<"{\"id\":\""<<id<<"\",\"type\":\""<<type<<"\",\"value\":";}
 void number(const char* id,double v){key(id,"number");out<<std::setprecision(17)<<v<<'}';}
 template<class T>void integer(const char* id,T v){key(id,"integer");out<<std::int32_t(v)<<'}';}
 void flag(const char* id,bool v){key(id,"flag");out<<(v?"true":"false")<<'}';}
 void layer(const char* id,std::uint64_t){key(id,"layer");out<<"\"\"}";}
 void data(const char* id,const std::vector<std::uint8_t>& bytes){key(id,"data");out<<'"';constexpr const char* digits="0123456789abcdef";for(auto b:bytes)out<<digits[b>>4]<<digits[b&15];out<<"\"}";}
};
int main(int argc,char** argv){
 if(argc==2){std::ofstream out(argv[1]);out<<'[';for(int i=0;i<6;i++){if(i)out<<',';out<<'[';Archive a{out};a.integer("ZpSv",1);a.integer("ZpId",flare::magic);auto p=fixture(i);psfilter::parameters::Schema<flare::Parameters>::visit(a,p);a.data("ZfEd",flare::encode_design(fixture_design(i,p)));out<<']';}out<<']';return out.good()?0:3;}
 if(argc==4){flare::Image src{320,240,0,0,std::vector<float>(320*240*4)},out;std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*sizeof(float));const int i=std::stoi(argv[2]);auto p=fixture(i);auto d=fixture_design(i,p);if(!file||!flare::render(src,p,out,nullptr,{}, {},&d))return 4;std::ofstream raw(argv[3],std::ios::binary);raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));return raw.good()?0:5;}
 return 2;
}
