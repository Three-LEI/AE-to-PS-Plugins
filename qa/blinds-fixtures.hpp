#pragma once
#include "blinds_core.hpp"
inline blinds::Parameters blinds_fixture(int index,int w,int h){
 auto p=blinds::defaults(w,h);
 constexpr double values[][4]={{0,0,20,0},{100,0,20,0},{50,0,20,0},{37,30,27.5,5},{65,90,11,2.5},{50,-45,7.5,12}};
 p.completion=values[index][0];p.direction=values[index][1];p.width=values[index][2];p.feather=values[index][3];
 if(index==3){p.cx-=1.75;p.cy+=1.5;}if(index==4){p.cx-=30;p.cy-=8;}if(index==5){p.cx+=11;p.cy-=1;}return p;
}
