#include "filmdamage_host.hpp"
#include "parameter_schema.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
filmdamage::Parameters fixture(int i){
 auto p=filmdamage::preset(i);p.seed=345+i*17;p.variation=i*3;p.grain=7+i*3;p.dust=12+i*10;p.stains=1+i;p.hairs=i%3;p.scratches=2+i;
 if(i==4)p.mix=72;
 if(i){p.halation=25+i*12;p.halation_radius=2+i*5;p.halation_threshold=58+i*4;p.grain_softness=18+i*14;p.damage_variation=326+i*71;}
 if(i){p.stain_texture=20+i*16;p.stain_edge_roughness=25+i*14;p.stain_edge_size=10+i*15;}
 return p;
}
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
 if(argc==2){std::ofstream out(argv[1]);out<<'[';for(int i=0;i<6;i++){if(i)out<<',';out<<'[';Archive a{out};a.integer("ZpSv",1);a.integer("ZpId",filmdamage::magic);auto p=fixture(i);psfilter::parameters::Schema<filmdamage::Parameters>::visit(a,p);out<<']';}out<<']';return out.good()?0:3;}
 if(argc==4){filmdamage::Image src{320,240,0,0,std::vector<float>(320*240*4)},out;std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*sizeof(float));const int i=std::stoi(argv[2]);auto p=fixture(i);if(!file||!filmdamage::render(src,p,out))return 4;std::ofstream raw(argv[3],std::ios::binary);raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));return raw.good()?0:5;}
 return 2;
}
