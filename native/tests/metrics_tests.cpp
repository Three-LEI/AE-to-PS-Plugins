#include "psui/ui_metrics.hpp"
#include <cstdlib>
#include <iostream>
void check(bool v,const char* text){if(!v){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
int main(){
 for(float scale:{1.f,1.25f,1.5f,2.f,3.f}){
  check(psui::raster_font_pixels(scale)==std::round(14*scale),"font rasterizes at integer device pixels");
  const auto rail=psui::rail_geometry(10,20,180*scale,22*scale,scale,50,0,100);
  check(rail.radius*2>=rail.thickness*5,"thumb is visibly larger than rail");
  check(rail.radius*2<22*scale,"thumb fits full-height hit target");
  check(std::abs(rail.center-(rail.left+rail.right)*.5f)<.001f,"middle position");
  check(psui::rail_geometry(10,20,180*scale,22*scale,scale,-50,0,100).center==rail.left,"below-range values preserve input but clamp visual thumb");
  check(psui::rail_geometry(10,20,180*scale,22*scale,scale,150,0,100).center==rail.right,"above-range visual thumb clamp");
 }
 std::cout<<"PASS: 100/125/150/200/300% font pixels and rail geometry; thumb/hit target; endpoint clamps.\n";
}
