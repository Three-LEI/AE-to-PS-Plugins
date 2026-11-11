#include "cell_core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace cell {
namespace {
double smooth(double x){x=std::clamp(x,0.,1.);return x*x*x*(x*(x*6-15)+10);}
std::uint32_t mix(std::uint32_t h){h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;return h^(h>>16);}
double random(std::uint32_t h){return mix(h)/4294967296.;}
int wrap(int x,int n){const int v=x%n;return v<0?v+n:v;}
struct Feature {double x,y,brightness,static_brightness;std::uint32_t key;};
struct Field {
 const Parameters& p;double evolution,fraction;int frame;bool hq;int last_x=INT32_MAX,last_y=INT32_MAX;
 std::array<Feature,49> points{};
 // Bounded memoization for one render invocation. The keys include the
 // unwrapped lattice coordinates; original values and traversal order stay
 // unchanged. Parameter changes/other threads never share these caches.
 struct FeatureEntry {int x=INT32_MAX,y=INT32_MAX;Feature value{};};
 struct PairEntry {int x=INT32_MAX,y=INT32_MAX;std::array<Feature,2> value{};};
 mutable std::vector<FeatureEntry> feature_cache=std::vector<FeatureEntry>(1024);
 mutable std::vector<PairEntry> pair_cache=std::vector<PairEntry>(512);
 static std::uint32_t cache_key(int x,int y){return mix(std::uint32_t(x)*0x9e3779b9u^std::uint32_t(y)*0x85ebca6bu);}

 struct Segment {double ax,ay,bx,by;};
 std::array<Segment,98> tubes{};int tube_count=0;
 explicit Field(const Parameters& v):p(v),hq(high_quality(v.mode)){
  evolution=v.evolution/90.;if(v.cycle_evolution){evolution=std::fmod(evolution,v.cycle*4.);if(evolution<0)evolution+=v.cycle*4.;}
  frame=int(std::floor(evolution));fraction=smooth(evolution-frame);
 }
 double animated(std::uint32_t key,std::uint32_t salt)const{
  const auto index=[&](int f){return p.cycle_evolution?wrap(f,p.cycle*4):f;};
  const auto a=random(key^salt^mix(std::uint32_t(index(frame))*0x9e3779b9u));
  if(fraction==0)return a;
  const auto b=random(key^salt^mix(std::uint32_t(index(frame+1))*0x9e3779b9u));return a+(b-a)*fraction;
 }
 Feature feature(int ix,int iy)const{
  auto& cache=feature_cache[cache_key(ix,iy)&1023];
  if(cache.x==ix&&cache.y==iy)return cache.value;
  const int hash_x=p.tiling?wrap(ix,p.cells_x):ix,hash_y=p.tiling?wrap(iy,p.cells_y):iy;
  const auto key=mix(std::uint32_t(hash_x)*0x9e3779b9u^std::uint32_t(hash_y)*0x85ebca6bu^std::uint32_t(p.seed)*0xc2b2ae35u);
  cache.x=ix;cache.y=iy;return cache.value={ix+.5+(animated(key,0x1234abcd)-.5)*p.disperse,iy+.5+(animated(key,0x63d83595)-.5)*p.disperse,
          animated(key,0xa8f1d43b),random(key^0x72a4d38e),key};
 }
 std::array<Feature,2> neighbors(int ix,int iy)const{
  auto& cache=pair_cache[cache_key(ix,iy)&511];
  if(cache.x==ix&&cache.y==iy)return cache.value;
  const auto a=feature(ix,iy);std::array<Feature,2> result{};
  double first=std::numeric_limits<double>::max(),second=first;
  for(int dy=-3;dy<=3;++dy)for(int dx=-3;dx<=3;++dx)if(dx||dy){
   const auto q=feature(ix+dx,iy+dy);const double sx=q.x-a.x,sy=q.y-a.y,d=sx*sx+sy*sy;
   if(d<first){second=first;result[1]=result[0];first=d;result[0]=q;}
   else if(d<second){second=d;result[1]=q;}
  }
  cache.x=ix;cache.y=iy;return cache.value=result;
 }
 void neighborhood(int ix,int iy){
  if(ix==last_x&&iy==last_y)return;last_x=ix;last_y=iy;
  int n=0;for(int y=-3;y<=3;++y)for(int x=-3;x<=3;++x)points[n++]=feature(ix+x,iy+y);
  if(p.mode==Mode::tubular){
   // Stable mutual-neighbor graph. Choosing the nearest pair per output pixel
   // changes segments abruptly at order-2 Voronoi boundaries and tears tubes.
   tube_count=0;
   for(int y=-3;y<=3;++y)for(int x=-3;x<=3;++x){const auto a=feature(ix+x,iy+y);
    for(const auto& b:neighbors(ix+x,iy+y)){
     // A feature can move outside its lattice cell; recover its cell from the
     // bounded neighborhood instead of inferring it by floor(position).
     int bx=0,by=0;bool found=false;
     for(int dy=-3;dy<=3&&!found;++dy)for(int dx=-3;dx<=3;++dx){const auto q=feature(ix+x+dx,iy+y+dy);
      if(q.x==b.x&&q.y==b.y){bx=ix+x+dx;by=iy+y+dy;found=true;break;}}
     if(!found)continue;
     const auto back=neighbors(bx,by);
     if((back[0].x==a.x&&back[0].y==a.y)||(back[1].x==a.x&&back[1].y==a.y))
      tubes[tube_count++]={a.x,a.y,b.x,b.y};
    }
   }
  }
 }
};
Pixel sample(Source s,double x,double y){
 if(!s.image||!s.image->valid())return {};
 const auto& im=*s.image;
 x=std::clamp((x-s.x)/s.step_x,0.,double(im.width-1));y=std::clamp((y-s.y)/s.step_y,0.,double(im.height-1));
 const int ix=int(x),iy=int(y);const double fx=x-ix,fy=y-iy;Pixel result{};
 for(int j=0;j<2;++j)for(int i=0;i<2;++i){const double weight=(i?fx:1-fx)*(j?fy:1-fy);const auto n=(std::size_t(std::min(im.height-1,iy+j))*im.width+std::min(im.width-1,ix+i))*4;
  const float a=im.rgba[n+3];for(int k=0;k<3;++k)result[k]+=float(weight*a*im.rgba[n+k]);result[3]+=float(weight*a);
 }
 if(result[3]>1.e-8f)for(int k=0;k<3;++k)result[k]/=result[3];else result={};return result;
}
Pixel cell_colour(const Field& field,const Feature& point,Source source){
 const double x=field.p.offset_x+point.x*field.p.size,y=field.p.offset_y+point.y*field.p.size;
 return sample(source,x,y);
}
double overflow(double v,Overflow mode){
 if(mode==Overflow::soft_clamp)return .5+std::atan((v-.5)*3.141592653589793)/3.141592653589793;
 if(mode==Overflow::wrap_back){v=std::fmod(v,2.);if(v<0)v+=2;return v<=1?v:2-v;}
 return std::clamp(v,0.,1.);
}
Pixel shade(Field& field,double x,double y,Source source){
 const auto& p=field.p;const double u=(x-p.offset_x)/p.size,v=(y-p.offset_y)/p.size;
 field.neighborhood(int(std::floor(u)),int(std::floor(v)));
 int first=0;double d1=std::numeric_limits<double>::max();
 for(int i=0;i<49;++i){const auto& q=field.points[i];const double dx=q.x-u,dy=q.y-v,d=dx*dx+dy*dy;
  if(d<d1){d1=d;first=i;}}
 const auto& a=field.points[first];
 if(sharpness_mode(p.mode)){
  // Exponential distance weights give continuous, adjustable cell boundaries.
  // Subtract the nearest distance before exponentiation to avoid underflow.
  double sum=0;std::array<double,4> accum{};const double sharp=p.contrast*.32;
  for(const auto& q:field.points){const double dx=q.x-u,dy=q.y-v;
   const double exponent=-(dx*dx+dy*dy-d1)*sharp;if(exponent<-18)continue;
   const double weight=std::exp(exponent);Pixel c;
   if(samples_source(p.mode))c=cell_colour(field,q,source);
   else {const float value=float(p.mode==Mode::static_plates||p.mode==Mode::static_plates_hq?q.static_brightness:q.brightness);c={value,value,value,1};}
   for(int k=0;k<3;++k)accum[k]+=weight*c[k]*c[3];accum[3]+=weight*c[3];sum+=weight;
  }
  Pixel colour{};if(accum[3]>1.e-8)for(int k=0;k<3;++k)colour[k]=float(accum[k]/accum[3]);
  colour[3]=sum>0?float(std::clamp(accum[3]/sum,0.,1.)):0;
  if(p.invert)for(int k=0;k<3;++k)colour[k]=1-colour[k];return colour;
 }
 double edge=10,pyramid=10;
 for(int i=0;i<49;++i)if(i!=first){const auto& q=field.points[i];const double sx=q.x-a.x,sy=q.y-a.y,separation=sx*sx+sy*sy;
  if(separation<1.e-16)continue;const double dx=q.x-u,dy=q.y-v,delta=std::max(0.,dx*dx+dy*dy-d1);
  edge=std::min(edge,delta/(2*std::sqrt(separation)));pyramid=std::min(pyramid,delta/separation);
 }
 double raw=0;
 switch(p.mode){
  case Mode::bubbles:raw=1-d1*.75;break;
  case Mode::crystals:case Mode::crystals_hq:raw=pyramid*(p.mode==Mode::crystals_hq?.75:.70);break;
  case Mode::pillow:raw=1-std::exp(-edge*24);break;
  case Mode::mixed_crystals:raw=.5+(a.static_brightness-.5)*std::clamp(pyramid,0.,1.)*.45;break;
  case Mode::tubular:{
   double distance2=10;
   for(int i=0;i<field.tube_count;++i){const auto& s=field.tubes[i];
    const double dx=s.bx-s.ax,dy=s.by-s.ay,len=dx*dx+dy*dy;
    const double t=len>1.e-16?std::clamp(((u-s.ax)*dx+(v-s.ay)*dy)/len,0.,1.):0;
    const double xx=u-(s.ax+t*dx),yy=v-(s.ay+t*dy);distance2=std::min(distance2,xx*xx+yy*yy);
   // Width calibrated against the saved AE 100/200 contrast cases. This
   // matches the overall profile, not Adobe's private random layout.
   }raw=1-distance2*1.06;break;}
  default:break;
 }
 // AE's white-centred patterns darken outwards as contrast increases.
 double value;
 if(p.mode==Mode::bubbles||p.mode==Mode::tubular||p.mode==Mode::pillow)value=1-(1-raw)*p.contrast*.01;
 else if(p.mode==Mode::mixed_crystals)value=.5+(raw-.5)*p.contrast*.01;
 else value=raw*p.contrast*.01;
 value=overflow(value,p.overflow);if(p.invert)value=1-value;
 return {float(value),float(value),float(value),1};
}
}
Parameters defaults(double width,double height,double left,double top){Parameters p;p.offset_x=left+width*.5;p.offset_y=top+height*.5;return p;}
bool samples_source(Mode m){return m==Mode::crystallize||m==Mode::crystallize_hq;}
bool sharpness_mode(Mode m){return samples_source(m)||m==Mode::plates||m==Mode::plates_hq||m==Mode::static_plates||m==Mode::static_plates_hq;}
bool high_quality(Mode m){return m>=Mode::crystals_hq&&m<=Mode::mixed_crystals;}
bool valid(const Parameters& p){
 const auto value=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 return int(p.mode)>=0&&p.mode<Mode::count&&int(p.overflow)>=0&&int(p.overflow)<3&&value(p.contrast,0,10000)&&value(p.disperse,0,1.5)&&value(p.size,2,2000)&&value(p.offset_x,-1.e7,1.e7)&&value(p.offset_y,-1.e7,1.e7)&&value(p.evolution,-36000000,36000000)&&p.cells_x>=1&&p.cells_x<=1000&&p.cells_y>=1&&p.cells_y<=1000&&p.cycle>=1&&p.cycle<=88&&p.seed>=0&&p.seed<=100000;
}
bool render(const Parameters& p,Region r,int width,int height,Image& out,Source source,const Cancel& cancel,bool locked_alpha){
 out={};const auto n=std::uint64_t(std::max(0,width))*std::max(0,height);
 if(!valid(p)||width<=0||height<=0||n>16777216||!std::isfinite(r.x)||!std::isfinite(r.y)||!std::isfinite(r.step)||r.step<=0||std::abs(r.x)+width*r.step>1.e8||std::abs(r.y)+height*r.step>1.e8)return false;
 if(source.image&&(!source.image->valid()||!std::isfinite(source.x)||!std::isfinite(source.y)||!std::isfinite(source.step_x)||!std::isfinite(source.step_y)||source.step_x<=0||source.step_y<=0))return false;
 if((samples_source(p.mode)||locked_alpha)&&!source.image)return false;
 Field field(p);Image result{width,height,int(r.x),int(r.y),std::vector<float>(std::size_t(n)*4)};
 // HQ uses a bounded 2x2 subpixel footprint; zoom does not alter saved parameters.
 const int aa=high_quality(p.mode)||r.step>1?2:1;
 for(int y=0;y<height;++y){if(cancel&&cancel())return false;for(int x=0;x<width;++x){
  const double xx=r.x+x*r.step+(r.step-1)*.5,yy=r.y+y*r.step+(r.step-1)*.5;Pixel pixel{};
  for(int j=0;j<aa;++j)for(int i=0;i<aa;++i){const auto q=shade(field,xx+((i+.5)/aa-.5)*r.step,yy+((j+.5)/aa-.5)*r.step,source);
   for(int k=0;k<3;++k)pixel[k]+=q[k]*q[3]/float(aa*aa);pixel[3]+=q[3]/float(aa*aa);}
  if(pixel[3]>1.e-8f)for(int k=0;k<3;++k)pixel[k]/=pixel[3];else pixel={};
  if(locked_alpha)pixel[3]=sample(source,xx,yy)[3];
  const auto index=(std::size_t(y)*width+x)*4;for(int k=0;k<4;++k)result.rgba[index+k]=pixel[k];
 }}out=std::move(result);return true;
}
}
