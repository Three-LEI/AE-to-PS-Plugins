#include "energy_internal.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace energy::detail {
namespace {
constexpr double pi=std::numbers::pi;
double clamp(double v){return std::clamp(v,0.,1.);}
std::uint32_t hash(std::uint32_t v){v^=v>>16;v*=0x7feb352du;v^=v>>15;v*=0x846ca68bu;return v^(v>>16);}
double random(std::uint32_t seed,std::uint32_t a,std::uint32_t b=0){return double(hash(seed^hash(a+0x9e3779b9u)^hash(b+0x85ebca6bu)))/4294967295.;}
double fade(double x){return x*x*(3-2*x);}
double noise(double x,std::uint32_t seed,int period=0){
 const auto cell=std::int64_t(std::floor(x));const double t=fade(x-double(cell));auto index=[&](std::int64_t i){if(period){i%=period;if(i<0)i+=period;}return std::uint32_t(i);};return std::lerp(random(seed,index(cell)),random(seed,index(cell+1)),t)*2-1;
}
double noise2(double x,double y,std::uint32_t seed){
 const auto xx=std::int64_t(std::floor(x)),yy=std::int64_t(std::floor(y));const double tx=fade(x-double(xx)),ty=fade(y-double(yy));
 return std::lerp(std::lerp(random(seed,std::uint32_t(xx),std::uint32_t(yy)),random(seed,std::uint32_t(xx+1),std::uint32_t(yy)),tx),std::lerp(random(seed,std::uint32_t(xx),std::uint32_t(yy+1)),random(seed,std::uint32_t(xx+1),std::uint32_t(yy+1)),tx),ty)*2-1;
}
double fbm(double x,double y,const Parameters& p,std::uint32_t salt){double sum=0,weight=1,norm=0;for(int i=0;i<p.detail;i++){sum+=noise2(x,y,p.seed+salt+std::uint32_t(i)*0x9e3779b9u)*weight;norm+=weight;x*=2.03;y*=2.03;weight*=.5;}return sum/norm;}
float sample(const std::vector<float>& data,int w,int h,double x,double y){
 if(x<=-1||y<=-1||x>=w||y>=h)return 0;const int xx=int(std::floor(x)),yy=int(std::floor(y));const double tx=x-xx,ty=y-yy;auto value=[&](int a,int b){return a>=0&&a<w&&b>=0&&b<h?data[std::size_t(b)*w+a]:0.f;};return float(std::lerp(std::lerp(double(value(xx,yy)),double(value(xx+1,yy)),tx),std::lerp(double(value(xx,yy+1)),double(value(xx+1,yy+1)),tx),ty));
}
Point interpolate(Point a,Point b,double t){return {std::lerp(a.x,b.x,t),std::lerp(a.y,b.y,t)};}
double length(Point a,Point b){return std::hypot(a.x-b.x,a.y-b.y);}
struct ElectricCurve {bool closed=false;double length=0;std::vector<Point> points;};
bool electric(const Path& path,const Parameters& p,std::vector<Segment>& out,std::vector<Segment>& branches,const Cancel& cancel){
 std::vector<ElectricCurve> curves;std::size_t total_points=0;double total_length=0;
 for(std::size_t index=0;index<path.curves.size();index++){
  if(cancel&&cancel())return false;const auto& original=path.curves[index];std::vector<Segment> flat;if(!flatten(Path{{original}},.2,flat,cancel))return false;
  double size=0;std::vector<double> ends;for(const auto& s:flat){size+=length(s.a,s.b);ends.push_back(size);}if(size<=1e-10){out.insert(out.end(),flat.begin(),flat.end());continue;}
  const double desired=std::ceil(size/std::max(.25,std::min(5.,p.wavelength/12)));if(desired>262144)return false;const int count=std::max(2,int(desired));if(total_points+count+1>262144)return false;total_points+=count+1;
  std::vector<Point> base;std::size_t at=0;for(int j=0;j<=count;j++){const double d=size*j/count;while(at+1<flat.size()&&ends[at]<d)at++;const double begin=at?ends[at-1]:0,span=ends[at]-begin;base.push_back(interpolate(flat[at].a,flat[at].b,span?(d-begin)/span:0));}
  ElectricCurve curve;curve.closed=original.closed;curve.length=size;curve.points=base;
  for(int j=0;j<=count;j++){
   const auto before=base[j?j-1:(original.closed?count-1:0)],after=base[j<count?j+1:(original.closed?1:count)];const double dx=after.x-before.x,dy=after.y-before.y,n=std::hypot(dx,dy),u=double(j)/count;double displacement=0,weight=1,norm=0;
   for(int octave=0;octave<p.detail;octave++){const int period=original.closed?std::max(1,int(std::lround(size/p.wavelength)))*(1<<octave):0;const double position=period?u*period:u*size/p.wavelength*(1<<octave);displacement+=noise(position+p.evolution/90,p.seed+std::uint32_t(index)*0x85ebca6bu+std::uint32_t(octave)*0x9e3779b9u,period)*weight;norm+=weight;weight*=.5;}
   const double envelope=original.closed?1:(j==0||j==count?0:std::pow(std::sin(pi*u),.75));displacement=displacement/norm*p.distortion*envelope;if(n>0){curve.points[j].x-=dy/n*displacement;curve.points[j].y+=dx/n*displacement;}
  }
  if(original.closed)curve.points.back()=curve.points.front();for(int j=0;j<count;j++)out.push_back({curve.points[j],curve.points[j+1],double(j)/count,double(j+1)/count});total_length+=size;curves.push_back(std::move(curve));
 }
 for(int b=0;b<p.branches&&total_length>0&&p.branch_length>0;b++){
  if(cancel&&cancel())return false;double pick=random(p.seed+1337,std::uint32_t(b))*total_length;const ElectricCurve* selected=&curves.back();for(const auto& c:curves){if(pick<=c.length){selected=&c;break;}pick-=c.length;}
  const double u=clamp(pick/selected->length);if(u<p.trim_start*.01||u>p.trim_end*.01)continue;const double f=u*(selected->points.size()-1);const auto at=std::min(selected->points.size()-2,std::size_t(f));const auto root=interpolate(selected->points[at],selected->points[at+1],f-at);auto a=selected->points[at],z=selected->points[at+1];double angle=std::atan2(z.y-a.y,z.x-a.x)+(random(p.seed+233,std::uint32_t(b))>.5?1:-1)*(pi*.25+random(p.seed+811,std::uint32_t(b))*pi*.35);
  const double size=p.branch_length*(.5+random(p.seed+993,std::uint32_t(b))*.5);const int steps=std::max(2,int(std::ceil(size/2)));Point previous=root;
  for(int j=1;j<=steps;j++){const double t=double(j)/steps,shift=noise(t*4+p.evolution/90,p.seed+std::uint32_t(b)*97)*p.distortion*.3*std::sin(pi*t);Point next{root.x+std::cos(angle)*size*t-std::sin(angle)*shift,root.y+std::sin(angle)*size*t+std::cos(angle)*shift};branches.push_back({previous,next,double(j-1)/steps,t});previous=next;}
 }
 return true;
}
bool fire(const Image& src,const Parameters& p,const std::vector<float>& core,std::vector<float>& flame,std::vector<float>& heat,const Cancel& cancel){
 const int w=src.width,h=src.height;flame.assign(core.size(),0);heat.assign(core.size(),0);if(p.fire_height<=0)return true;
 const double angle=std::remainder(p.fire_direction,360.)*pi/180,dx=std::cos(angle),dy=std::sin(angle),nx=-dy,ny=dx;
 int cx0=w,cy0=h,cx1=-1,cy1=-1;for(int y=0;y<h;y++){if(cancel&&cancel())return false;for(int x=0;x<w;x++)if(core[std::size_t(y)*w+x]>0){cx0=std::min(cx0,x);cy0=std::min(cy0,y);cx1=std::max(cx1,x);cy1=std::max(cy1,y);}}
 if(cx1<cx0)return true;
 double support_u0=1e30,support_u1=-1e30,support_v0=1e30,support_v1=-1e30;for(Point a:std::initializer_list<Point>{{cx0+.5,cy0+.5},{cx1+.5,cy0+.5},{cx0+.5,cy1+.5},{cx1+.5,cy1+.5}}){const double u=a.x*nx+a.y*ny,v=a.x*dx+a.y*dy;support_u0=std::min(support_u0,u);support_u1=std::max(support_u1,u);support_v0=std::min(support_v0,v);support_v1=std::max(support_v1,v);}
 double ulo=1e30,uhi=-1e30,vlo=1e30,vhi=-1e30;for(Point a:std::initializer_list<Point>{{0,0},{double(w),0},{0,double(h)},{double(w),double(h)}}){double u=a.x*nx+a.y*ny,v=a.x*dx+a.y*dy;ulo=std::min(ulo,u);uhi=std::max(uhi,u);vlo=std::min(vlo,v);vhi=std::max(vhi,v);}
 ulo=std::floor(ulo)-2;vlo=std::floor(vlo)-2;const int gw=int(std::ceil(uhi-ulo))+3,gh=int(std::ceil(vhi-vlo))+3;if(std::uint64_t(gw)*gh>std::max<std::uint64_t>(16777216,std::uint64_t(w)*h*3))return false;std::vector<float> field(std::size_t(gw)*gh);
 // Independent columns carry heat away from the emitter. This avoids a sparse
 // ray marcher missing thin strokes at large flame heights.
 if(!pscpu::parallel_ranges(gw,field.size(),cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(auto column=begin;column<end;column++){if(stop())return false;const double u=ulo+column+.5,variation=.72+.28*noise(u/p.wavelength*5+p.evolution/150,p.seed+77),decay=1/std::max(.25,p.fire_height*variation);double value=0;for(int row=0;row<gh;row++){const double v=vlo+row+.5,x=u*nx+v*dx-.5,y=u*ny+v*dy-.5;value=std::max(double(sample(core,w,h,x,y)),std::max(0.,value-decay));field[std::size_t(row)*gw+column]=float(value);}}return true;}))return false;
 return pscpu::parallel_ranges(h,core.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){if(stop())return false;for(int x=0;x<w;x++){const double xx=x+.5,yy=y+.5,u=xx*nx+yy*ny,v=xx*dx+yy*dy;if(u<support_u0-p.fire_spread*1.65-3||u>support_u1+p.fire_spread*1.65+3||v<support_v0-3||v>support_v1+p.fire_height+3)continue;const double base=sample(field,gw,gh,u-ulo-.5,v-vlo-.5),t=1-base,phase=p.evolution/90,curl=fbm(u/p.wavelength*2,v/p.wavelength-phase,p,319)*p.fire_spread*(t+.1)*1.5;const double value=sample(field,gw,gh,u+curl-ulo-.5,v-vlo-.5);if(value<=0)continue;const double grain=fbm((u+curl)/p.wavelength*5,v/p.wavelength*2-phase,p,613)*.5+.5;
    const double strength=clamp((value-(.08+.62*grain*grain))*3.6)*std::pow(value,1.35);const auto i=y*w+x;flame[i]=float(strength);heat[i]=float(clamp(value));
   }}return true;});
}
}
bool extended(const Image& src,const Parameters& p,const Path& input,Image& out,const Cancel& cancel){
 if(p.style>=3)return textured_electric(src,p,input,out,cancel);
 Path path;std::vector<float> core(std::size_t(src.width)*src.height);std::vector<Segment> segments,branches;
 if(p.input==3){
  std::vector<float> alpha(core.size());for(std::size_t i=0;i<alpha.size();i++){double a=src.rgba[i*4+3];alpha[i]=float(std::isfinite(a)?clamp(a):0);}
  if(!pscpu::parallel_ranges(src.height,alpha.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){if(stop())return false;for(int x=0;x<src.width;x++){double xx=x-p.offset_x,yy=double(y)-p.offset_y;if(p.style==1&&p.distortion>0){const double phase=p.evolution/90;xx+=fbm(x/p.wavelength+phase,double(y)/p.wavelength,p,971)*p.distortion;yy+=fbm(x/p.wavelength,double(y)/p.wavelength-phase,p,1297)*p.distortion;}core[y*src.width+x]=sample(alpha,src.width,src.height,xx,yy);}}return true;}))return false;
 }else{
  if(p.input==2){if(!trace_alpha(src,p.alpha_threshold,path,cancel))return false;}else path=p.input?input:line_path(p);
  if(p.style==1){if(!electric(path,p,segments,branches,cancel))return false;}else if(!flatten(path,.2,segments,cancel))return false;
  if(!rasterize(src,p,segments,core,cancel))return false;
  if(!branches.empty()){auto q=p;q.width=std::max(.1,p.width*.55);q.trim_start=0;q.trim_end=100;q.taper_start=0;q.taper_end=100;if(!rasterize(src,q,branches,core,cancel))return false;}
 }
 if(p.style==2&&p.fire_height>0){std::vector<float> flame,heat;if(!fire(src,p,core,flame,heat,cancel))return false;return composite(src,p,core,out,cancel,&flame,&heat);}
 return composite(src,p,core,out,cancel);
}
}
