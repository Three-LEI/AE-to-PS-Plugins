#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <vector>
namespace creative {
enum class Kind:int{turbulent,sweep,kaleida};
struct Parameters {
 Kind kind=Kind::turbulent;
 int type=0,pinning=0,edge=0,shape=2,reception=0,mirroring=5,quality=1;
 bool cycle=false,floating=false;
 double cx=0,cy=0,amount=50,size=100,complexity=1,evolution=0,cycles=1,seed=0;
 double horizontal=100,vertical=100,mix=100;
 double direction=-30,width=50,intensity=25,edge_intensity=50,thickness=4;
 std::array<double,3> color{1,250./255,240./255};double rotation=0;
 bool operator==(const Parameters&)const=default;
};
struct Image {int width=0,height=0,left=0,top=0;std::vector<float> rgba;bool valid()const{return width>0&&height>0&&std::size_t(width)*height<=16777216&&rgba.size()==std::size_t(width)*height*4;}};
using Cancel=std::function<bool()>;
Parameters defaults(Kind,int width,int height,int left=0,int top=0);
bool valid(const Parameters&);
bool render(const Image&,const Parameters&,Image&,const Cancel& cancel={});
// Pixel-centre inverse mapping, exposed for analytic geometry tests.
std::array<double,2> warp(double x,double y,const Parameters&,const Image&);
}
