#include "caustics_host.hpp"
#include "parameter_schema.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
caustics::Parameters fixture(int i){auto p=caustics::preset(i);p.shift_x=i*7-8;p.shift_y=i*3;p.refraction=i==3?100:25;p.seed=345+i*17;p.output=i==5?2:0;p.mix=i==4?72:100;if(i==1)p.surface=1;if(i==2){p.height=12;p.softness=2;p.surface=2;p.map_key=psui::layer_name_key("鎏金水波");}return p;}
struct Archive {
 std::ostream& out;bool comma=false;
 void key(const char* id,const char* type){if(comma)out<<',';comma=true;out<<"{\"id\":\""<<id<<"\",\"type\":\""<<type<<"\",\"value\":";}
 void number(const char* id,double v){key(id,"number");out<<std::setprecision(17)<<v<<'}';}
 template<class T>void integer(const char* id,T v){key(id,"integer");out<<std::int32_t(v)<<'}';}
 void flag(const char* id,bool v){key(id,"flag");out<<(v?"true":"false")<<'}';}
 void layer(const char* id,std::uint64_t value){key(id,"layer");out<<'"'<<(value?"鎏金水波":"")<<"\"}";}
 void data(const char* id,const std::vector<std::uint8_t>& bytes){key(id,"data");out<<'"';constexpr const char* digits="0123456789abcdef";for(auto b:bytes)out<<digits[b>>4]<<digits[b&15];out<<"\"}";}
};
int main(int argc,char** argv){
 if(argc==2){std::ofstream out(argv[1]);out<<'[';for(int i=0;i<6;i++){if(i)out<<',';out<<'[';Archive a{out};a.integer("ZpSv",1);a.integer("ZpId",caustics::magic);auto p=fixture(i);psfilter::parameters::Schema<caustics::Parameters>::visit(a,p);out<<']';}out<<']';return out.good()?0:3;}
 if(argc==4){caustics::Image src{320,240,0,0,std::vector<float>(320*240*4)},out;std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*sizeof(float));const int i=std::stoi(argv[2]);auto p=fixture(i);if(!file||!caustics::render(src,p,out,p.surface==2?&src:nullptr))return 4;std::ofstream raw(argv[3],std::ios::binary);raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));return raw.good()?0:5;}
 return 2;
}
