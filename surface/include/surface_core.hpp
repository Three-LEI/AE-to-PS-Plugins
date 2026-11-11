#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace surface {
enum class Kind:int{blob,glass};
struct Parameters {
 Kind kind=Kind::glass;std::uint64_t map_key=0;int property=5;
 double softness=20,cut=25,height=25,displacement=100;
 int light_type=0;double intensity=100;std::array<double,3> light_color{1,1,1};
 double light_height=65,light_x=0,light_y=0,direction=-45;
 double ambient=50,diffuse=50,specular=50,roughness=.025,metal=100;
 bool operator==(const Parameters&)const=default;
};
struct Image {int width=0,height=0,left=0,top=0;std::vector<float> rgba;bool valid()const{return width>0&&height>0&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}};
using Cancel=std::function<bool()>;
Parameters defaults(Kind,int width,int height,int left=0,int top=0);
bool valid(const Parameters&);
bool render(const Image&,const Parameters&,Image&,const Image* map=nullptr,const Cancel& cancel={});
}
