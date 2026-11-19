#include "longshadow_host.hpp"
#include "parameter_schema.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
longshadow::Parameters fixture(int i){
 auto p=longshadow::preset(i%5);
 p.angle=(i-2)*47;p.length=37+i*29.5;p.opacity=44+9*i;p.shadow_only=(i==1||i==4);
 p.crop=(i>=3);p.crop_left=8+i*3;p.crop_right=96-i*6;p.expansion=i*.6;
 p.fade=i*12;p.mask=i%3;p.mask_x=58;p.mask_y=45;p.mask_width=135;p.mask_height=115;
 p.mask_feather=5+i*6;p.invert_mask=(i==5);
 if(i>0){
  p.color_mode=i%2||i==4;p.gap=i*3.5;p.source_brightness=85+i*8;
  p.blur_near=i%3;p.blur_far=i==1?p.blur_near:i*3.25;
 }
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
 if(argc==2){std::ofstream out(argv[1]);out<<'[';for(int i=0;i<6;i++){if(i)out<<',';out<<'[';Archive a{out};a.integer("ZpSv",1);a.integer("ZpId",longshadow::magic);auto p=fixture(i);psfilter::parameters::Schema<longshadow::Parameters>::visit(a,p);out<<']';}out<<']';return out.good()?0:3;}
 if(argc==4){longshadow::Image src{320,240,0,0,std::vector<float>(320*240*4)},out;std::ifstream file(argv[1],std::ios::binary);file.read(reinterpret_cast<char*>(src.rgba.data()),src.rgba.size()*sizeof(float));const int i=std::stoi(argv[2]);auto p=fixture(i);if(!file||!longshadow::render(src,p,out))return 4;std::ofstream raw(argv[3],std::ios::binary);raw.write(reinterpret_cast<const char*>(out.rgba.data()),out.rgba.size()*sizeof(float));return raw.good()?0:5;}
 return 2;
}
