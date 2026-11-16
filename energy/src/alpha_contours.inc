#include "energy_core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
namespace energy {
namespace {
constexpr std::size_t limit=262144;
struct Edge {Point a,b;std::uint64_t ka,kb;};
double chord(Point p,Point a,Point b){const double dx=b.x-a.x,dy=b.y-a.y,ss=dx*dx+dy*dy,t=ss?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/ss,0.,1.):0;return std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy);}
void simplify_span(const std::vector<Point>& points,std::size_t first,std::size_t last,std::vector<bool>& keep){
 std::vector<std::pair<std::size_t,std::size_t>> stack{{first,last}};keep[first]=keep[last]=true;
 while(!stack.empty()){const auto [a,b]=stack.back();stack.pop_back();double maximum=.2;std::size_t split=0;for(auto i=a+1;i<b;i++){const auto d=chord(points[i],points[a],points[b]);if(d>maximum){maximum=d;split=i;}}if(split){keep[split]=true;stack.emplace_back(a,split);stack.emplace_back(split,b);}}
}
}
bool trace_alpha(const Image& image,double threshold_percent,Path& out,const Cancel& cancel){
 if(!image.valid()||!std::isfinite(threshold_percent)||threshold_percent<.1||threshold_percent>99.9)return false;
 const int w=image.width,h=image.height;const double iso=threshold_percent*.01;std::vector<Edge> edges;
 auto alpha=[&](int x,int y){if(x<0||x>=w||y<0||y>=h)return 0.;double a=image.rgba[(std::size_t(y)*w+x)*4+3];return std::isfinite(a)?std::clamp(a,0.,1.):0.;};
 auto key=[&](int x,int y,int vertical){return (std::uint64_t(y+1)*std::uint64_t(w+2)+std::uint64_t(x+1))*2+vertical;};
 for(int y=-1;y<h;y++){
  if(cancel&&cancel())return false;
  for(int x=-1;x<w;x++){
   const double v[]={alpha(x,y),alpha(x+1,y),alpha(x+1,y+1),alpha(x,y+1)};int code=0;for(int k=0;k<4;k++)if(v[k]>=iso)code|=1<<k;if(code==0||code==15)continue;
   const Point corners[]={{x+.5,y+.5},{x+1.5,y+.5},{x+1.5,y+1.5},{x+.5,y+1.5}};
   const std::uint64_t keys[]={key(x,y,0),key(x+1,y,1),key(x,y+1,0),key(x,y,1)};
   auto point=[&](int side){int a=side,b=(side+1)%4;const double t=std::clamp((iso-v[a])/(v[b]-v[a]),0.,1.);return Point{image.left+corners[a].x+(corners[b].x-corners[a].x)*t,image.top+corners[a].y+(corners[b].y-corners[a].y)*t};};
   auto add=[&](int a,int b){if(edges.size()>=limit)return false;edges.push_back({point(a),point(b),keys[a],keys[b]});return true;};
   bool okay=true;switch(code){
    case 1:case 14:okay=add(3,0);break;case 2:case 13:okay=add(0,1);break;case 3:case 12:okay=add(3,1);break;case 4:case 11:okay=add(1,2);break;case 6:case 9:okay=add(0,2);break;case 7:case 8:okay=add(3,2);break;
    case 5:case 10:{const double q=(v[0]-iso)*(v[2]-iso)-(v[1]-iso)*(v[3]-iso);if(q>0||(q==0&&code==10))okay=add(0,1)&&add(2,3);else okay=add(3,0)&&add(1,2);break;}
   }if(!okay)return false;
  }
 }
 struct Vertex {std::size_t edges[2]{};unsigned count=0;};std::unordered_map<std::uint64_t,Vertex> vertices;vertices.reserve(edges.size()*2);
 for(std::size_t i=0;i<edges.size();i++)for(auto k:{edges[i].ka,edges[i].kb}){auto& vertex=vertices[k];if(vertex.count>=2)return false;vertex.edges[vertex.count++]=i;}
 std::vector<bool> used(edges.size());Path next;std::size_t knots=0;
 for(std::size_t i=0;i<edges.size();i++)if(!used[i]){
  if(cancel&&cancel())return false;std::vector<Point> points;std::size_t at=i;const auto first=edges[i].ka;auto cursor=first;
  for(std::size_t steps=0;steps<=edges.size();steps++){
   if(used[at])return false;used[at]=true;const auto& e=edges[at];const bool forward=e.ka==cursor;const auto a=forward?e.a:e.b,b=forward?e.b:e.a;const auto end=forward?e.kb:e.ka;
   if(points.empty()||std::hypot(points.back().x-a.x,points.back().y-a.y)>1e-10)points.push_back(a);
   if(end==first)break;const auto found=vertices.find(end);if(found==vertices.end()||found->second.count!=2)return false;const auto& vertex=found->second;at=vertex.edges[0]==at?vertex.edges[1]:vertex.edges[0];cursor=end;
   if(steps==edges.size())return false;(void)b;
  }
  if(points.size()<3)continue;
  // Split the closed ring at a distant vertex; simplify both arcs without
  // dropping inner rings, isolated pixels, or connecting separate components.
  std::size_t far=1;double far_distance=0;for(std::size_t j=1;j<points.size();j++){auto d=std::hypot(points[j].x-points[0].x,points[j].y-points[0].y);if(d>far_distance){far_distance=d;far=j;}}
  points.push_back(points.front());std::vector<bool> keep(points.size());simplify_span(points,0,far,keep);simplify_span(points,far,points.size()-1,keep);std::vector<Point> reduced;
  for(std::size_t j=0;j+1<points.size();j++)if(keep[j])reduced.push_back(points[j]);if(reduced.size()<3)reduced.assign(points.begin(),points.end()-1);
  if(next.curves.size()>=4096||knots+reduced.size()>16384)return false;Curve curve;curve.closed=true;for(auto point:reduced)curve.knots.push_back({point,point,point});knots+=reduced.size();next.curves.push_back(std::move(curve));
 }
 if(cancel&&cancel())return false;if(!valid_path(next))return false;out=std::move(next);return true;
}
}
