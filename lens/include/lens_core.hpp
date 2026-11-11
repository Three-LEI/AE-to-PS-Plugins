#pragma once
#include <vector>
#include <functional>
namespace lens {
enum class Kind:int{optics,cc};
struct Parameters{Kind kind=Kind::optics;double fov=0,cx=0,cy=0,size=50,convergence=100;int axis=0;bool reverse=false,optimal=false;bool operator==(const Parameters&)const=default;};
struct Image{int width=0,height=0,left=0,top=0;std::vector<float> rgba;bool valid()const{return width>0&&height>0&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}};
using Cancel=std::function<bool()>;
Parameters defaults(Kind,int width,int height,int left=0,int top=0);
bool valid(const Parameters&);
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={});
}
