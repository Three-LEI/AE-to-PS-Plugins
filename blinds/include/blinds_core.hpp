#pragma once
#include <vector>
#include <functional>
#include <cstddef>
namespace blinds {
struct Parameters {
 double completion=0,direction=0,width=20,feather=0,cx=0,cy=0;
 bool operator==(const Parameters&)const=default;
};
struct Image {
 int width=0,height=0,left=0,top=0;std::vector<float> rgba;
 bool valid()const{return width>0&&height>0&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}
};
using Cancel=std::function<bool()>;
Parameters defaults(int width,int height,int left=0,int top=0);
bool valid(const Parameters&);
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={});
// Unfeathered area coverage of one native pixel; for geometry checks.
double coverage(double x,double y,const Parameters&);
}
