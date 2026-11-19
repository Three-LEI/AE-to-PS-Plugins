#include "caustics_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <cfenv>
#include <deque>
namespace caustics {namespace {
constexpr double tau=2*std::numbers::pi;
double unit(double x){return std::clamp(x,0.,1.);}
double clean(float x){return std::isfinite(x)?double(x):0.;}
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
struct Node {double h=0,x=0,y=0;};
Node noise(double x,double y,std::uint32_t seed){
 const auto ix=std::int64_t(std::floor(x)),iy=std::int64_t(std::floor(y));const double a=x-std::floor(x),b=y-std::floor(y);
 const auto fade=[](double t){return t*t*t*(t*(t*6-15)+10);};const auto deriv=[](double t){return 30*t*t*(t-1)*(t-1);};
 auto at=[&](std::int64_t u,std::int64_t v){return double(hash(std::uint32_t(u)*0x8da6b343u^std::uint32_t(v)*0xd8163841u^seed))/2147483648.-1;};
 const double v00=at(ix,iy),v10=at(ix+1,iy),v01=at(ix,iy+1),v11=at(ix+1,iy+1),u=fade(a),v=fade(b);
 return {std::lerp(std::lerp(v00,v10,u),std::lerp(v01,v11,u),v),std::lerp(v10-v00,v11-v01,v)*deriv(a),std::lerp(v01-v00,v11-v10,u)*deriv(b)};
}
Node procedural(double x,double y,const Parameters& p){
 x=(x-p.shift_x)/p.scale;y=(y-p.shift_y)/p.scale;const double phase=std::remainder(p.evolution,360.)*tau/360;
 Node sum;double weights=0;
 const int count=int(std::ceil(p.complexity));
 for(int j=0;j<count;j++){
  const double f=std::pow(2.,j),weight=std::pow(.48,j)*std::min(1.,p.complexity-j),a=.37*j,c=std::cos(a),s=std::sin(a);
  const double u=(c*x-s*y)*f+13.71*j+.8*std::cos(phase+j),v=(s*x+c*y)*f+8.31*j+.8*std::sin(phase+j);
  const auto n=noise(u,v,p.seed+std::uint32_t(j)*12071u);sum.h+=weight*n.h;sum.x+=weight*f*(n.x*c+n.y*s);sum.y+=weight*f*(-n.x*s+n.y*c);weights+=weight;
 }
 sum.h/=weights;sum.x/=weights;sum.y/=weights;
 if(p.wave==1){
  // Crossing, irregular wave fronts remain controllable from the same height field.
  const double a=tau*(x+.17*sum.h)+phase,b=tau*(.32*x+.83*y+.11*sum.h)-phase*.7;
  const double ax=tau*(1+.17*sum.x),ay=tau*.17*sum.y,bx=tau*(.32+.11*sum.x),by=tau*(.83+.11*sum.y);
  sum={.55*std::sin(a)+.45*std::sin(b),.55*std::cos(a)*ax+.45*std::cos(b)*bx,.55*std::cos(a)*ay+.45*std::cos(b)*by};
 }else if(p.wave==2){
  const double r=std::hypot(x,y),a=tau*(r+.12*sum.h)-phase;
  const double dx=r>1e-9?x/r:0,dy=r>1e-9?y/r:0;
  sum={std::cos(a),-std::sin(a)*tau*(dx+.12*sum.x),-std::sin(a)*tau*(dy+.12*sum.y)};
 }
 return {sum.h*p.height,sum.x*p.height/p.scale,sum.y*p.height/p.scale};
}
double coordinate(double x,int size,int border){
 if(border==0)return std::clamp(x,0.,double(size-1));
 if(border==1){if(size==1)return 0;const double period=2.*(size-1);x=std::fmod(x,period);if(x<0)x+=period;return x>size-1?period-x:x;}
 if(border==2){x=std::fmod(x,double(size));return x<0?x+size:x;}return x;
}
std::array<double,4> sample(const Image& img,double x,double y,int border){
 x=coordinate(x,img.width,border);y=coordinate(y,img.height,border);
 // Transparent borders blend continuously across the last pixel, in premultiplied RGB.
 const int ix=int(std::floor(x)),iy=int(std::floor(y));const double tx=x-ix,ty=y-iy;std::array<double,4> value{};
 for(int j=0;j<2;j++)for(int i=0;i<2;i++){
  int xx=ix+i,yy=iy+j;const double w=(i?tx:1-tx)*(j?ty:1-ty);if(w==0)continue;
  if(border==3&&(xx<0||yy<0||xx>=img.width||yy>=img.height))continue;
  if(border==2){xx=(xx%img.width+img.width)%img.width;yy=(yy%img.height+img.height)%img.height;}else{xx=std::clamp(xx,0,img.width-1);yy=std::clamp(yy,0,img.height-1);}
  const auto k=(std::size_t(yy)*img.width+xx)*4;const double a=unit(clean(img.rgba[k+3]));value[3]+=w*a;
  for(int c=0;c<3;c++)value[c]+=w*a*clean(img.rgba[k+c]);
 }
 if(value[3]>1e-12)for(int c=0;c<3;c++)value[c]/=value[3];return value;
}
struct Surface {
 const Parameters& p;const Image* map;
 double raw(double x,double y)const{
  const auto a=sample(*map,x-map->left-p.shift_x-.5,y-map->top-p.shift_y-.5,p.border);
  double value=p.channel==4?a[3]:p.channel?unit(a[p.channel-1]):unit(.2126*a[0]+.7152*a[1]+.0722*a[2]);
  if(p.channel!=4)value=.5+(value-.5)*a[3];return (value-.5)*2*p.height;
 }
 double smoothed(double x,double y)const{
  if(p.map_softness==0)return raw(x,y);double value=0;const double d=p.map_softness*.65;
  for(int j=-1;j<=1;j++)for(int i=-1;i<=1;i++)value+=raw(x+i*d,y+j*d)*(i==0?2:1)*(j==0?2:1);return value/16;
 }
 Node operator()(double x,double y)const{
  if(p.surface==0)return procedural(x,y,p);
  const double d=std::max(.75,p.map_softness*.25);
  return {smoothed(x,y),(smoothed(x+d,y)-smoothed(x-d,y))/(2*d),(smoothed(x,y+d)-smoothed(x,y-d))/(2*d)};
 }
};
struct Grid {
 double left=0,top=0,step=1;int width=0,height=0;std::vector<Node> nodes;
 Node at(double x,double y)const{
  x=std::clamp((x-left)/step,0.,double(width-1));y=std::clamp((y-top)/step,0.,double(height-1));const int ix=int(x),iy=int(y);const double tx=x-ix,ty=y-iy;Node v;
  for(int j=0;j<2;j++)for(int i=0;i<2;i++){const double w=(i?tx:1-tx)*(j?ty:1-ty);if(w==0)continue;const auto& n=nodes[std::size_t(std::min(iy+j,height-1))*width+std::min(ix+i,width-1)];v.h+=w*n.h;v.x+=w*n.x;v.y+=w*n.y;}return v;
 }
};
struct Transport {Grid grid;std::vector<float> density;std::vector<std::array<double,4>> bottom;std::size_t bytes()const{return grid.nodes.capacity()*sizeof(Node)+density.capacity()*sizeof(float)+bottom.capacity()*sizeof(std::array<double,4>);}};
std::array<double,2> bend(const Node& n,const Parameters& p,double multiplier){
 if(p.depth==0||p.refractive_index==1||multiplier==0)return {};
 const double z=1/std::sqrt(1+n.x*n.x+n.y*n.y),eta=1/p.refractive_index;
 const double k=std::sqrt(std::max(0.,1-eta*eta*(1-z*z)))-eta*z;
 const double tz=eta+k*z,factor=p.depth*k*z/std::max(1e-9,tz)*multiplier;
 const double dx=n.x*factor,dy=n.y*factor,limit=512.,ratio=std::min(1.,limit/std::max(1e-9,std::hypot(dx,dy)));
 return {dx*ratio,dy*ratio};
}
std::array<double,4> refracted(const Image& src,const Grid& grid,const Parameters& p,RenderView view,double xx,double yy){
 double u=xx,v=yy;for(int j=0;j<4;j++){const auto d=bend(grid.at(u,v),p,p.refraction*.01);u=std::lerp(u,xx-d[0],.65);v=std::lerp(v,yy-d[1],.65);}
 return sample(src,(u-view.left)/view.step-.5,(v-view.top)/view.step-.5,p.border);
}
// Separable, normalized Gaussian. Blur is in native pixels and includes the
// draft pixel footprint, preventing narrow rays from becoming flickering dots.
bool blur(std::vector<float>& f,int w,int h,double sigma,const Cancel& cancel){
 if(sigma<.05)return true;const int radius=std::min(128,int(std::ceil(sigma*3)));std::vector<double> kernel(2*radius+1);double sum=0;
 for(int j=-radius;j<=radius;j++){kernel[j+radius]=std::exp(-.5*j*j/(sigma*sigma));sum+=kernel[j+radius];}for(auto& v:kernel)v/=sum;
 std::vector<float> temp(f.size());
 for(int axis=0;axis<2;axis++)if(!pscpu::parallel_ranges(h,f.size()*kernel.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& stop){
  for(auto y=lo;y<hi;y++){if(stop())return false;for(int x=0;x<w;x++){double v=0;for(int j=-radius;j<=radius;j++){const int xx=axis?x:std::clamp(x+j,0,w-1),yy=axis?std::clamp(int(y)+j,0,h-1):int(y);v+=kernel[j+radius]*(axis?temp[std::size_t(yy)*w+xx]:f[std::size_t(yy)*w+xx]);}if(axis)f[y*w+x]=float(v);else temp[y*w+x]=float(v);}}return true;
 }))return false;
 return true;
}
}
struct RenderCache::State {
 struct Key {
  Parameters p;RenderView view;int width=0,height=0,map_width=0,map_height=0,map_left=0,map_top=0,rounding=0,source_left=0,source_top=0;unsigned mxcsr=0;const float* map=nullptr;const float* source=nullptr;
  bool operator==(const Key& other)const{return p==other.p&&view.left==other.view.left&&view.top==other.view.top&&view.width==other.view.width&&view.height==other.view.height&&view.step==other.view.step&&width==other.width&&height==other.height&&map_width==other.map_width&&map_height==other.map_height&&map_left==other.map_left&&map_top==other.map_top&&rounding==other.rounding&&mxcsr==other.mxcsr&&map==other.map&&source==other.source&&source_left==other.source_left&&source_top==other.source_top;}
 };
 struct Entry {Key key;std::shared_ptr<const Transport> transport;};std::deque<Entry> entries;
};
RenderCache::RenderCache(std::size_t budget):state_(std::make_unique<State>()),budget_(budget){}
RenderCache::~RenderCache()=default;
void RenderCache::clear(){state_->entries.clear();bytes_=0;}
bool valid(const Parameters& p){
 auto r=[](double v,double lo,double hi){return std::isfinite(v)&&v>=lo&&v<=hi;};
 for(const auto& color:{p.light,p.dark})for(auto v:color)if(!r(v,0,1))return false;
 return p.surface>=0&&p.surface<=2&&p.wave>=0&&p.wave<=2&&p.channel>=0&&p.channel<=4&&p.output>=0&&p.output<=3&&p.blend>=0&&p.blend<=3&&p.border>=0&&p.border<=3&&p.quality>=1&&p.quality<=3&&r(p.scale,8,4000)&&r(p.complexity,1,5)&&r(p.height,0,500)&&r(p.evolution,-1e9,1e9)&&r(p.shift_x,-1e7,1e7)&&r(p.shift_y,-1e7,1e7)&&r(p.map_softness,0,200)&&r(p.depth,0,2000)&&r(p.refractive_index,1,3)&&r(p.focus,0,8)&&r(p.softness,0,100)&&r(p.intensity,0,1000)&&r(p.threshold,0,5)&&r(p.refraction,0,200)&&r(p.background,0,200)&&r(p.mix,0,100);
}
Parameters preset(int i){
 Parameters p;switch(i){
  case 1:p.scale=230;p.complexity=2;p.height=24;p.depth=180;p.focus=1.8;p.intensity=130;p.light={.7,.94,1};break;
  case 2:p.scale=115;p.complexity=2.2;p.height=12;p.focus=1.3;p.softness=1.2;p.intensity=85;p.light={1,.82,.46};break;
  case 3:p.wave=1;p.scale=190;p.height=12;p.complexity=2;p.depth=100;p.refraction=100;p.focus=1;p.intensity=65;p.softness=2;p.light={.6,.85,1};break;
  case 4:p.wave=2;p.scale=180;p.height=10;p.depth=95;p.focus=1.25;p.intensity=115;p.softness=1.8;p.output=1;p.dark={0,.025,.07};p.light={.3,.86,1};break;
  case 5:p.scale=125;p.complexity=3.2;p.height=14;p.focus=1.7;p.softness=.8;p.intensity=150;p.output=2;p.light={1,1,1};break;
  case 6:p.refraction=0;break;
 }return p;
}
bool render(const Image& src,const Parameters& p,Image& out,const Image* map,const Cancel& cancel,RenderView view,RenderCache* cache){
 if(!src.valid()||!valid(p)||&src==&out||!std::isfinite(view.step)||view.step<1)return false;
 if(p.surface==2&&(!map||!map->valid()))return false;
 if(p.surface==1&&(!map||!map->valid()))map=&src;
 if(view.width==0){view={double(src.left),double(src.top),double(src.width),double(src.height),1};}
 if(!std::isfinite(view.left)||!std::isfinite(view.top)||!std::isfinite(view.width)||!std::isfinite(view.height)||view.width<=0||view.height<=0||view.width>2000000||view.height>2000000)return false;
 const auto stop=[&]{return cancel&&cancel();};if(stop())return false;
 if(p.mix==0){out=src;return true;}
 pscpu::ParallelScope parallel;
 Image result{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};const int w=src.width,h=src.height;const double step=view.step;
 const bool refract=p.refraction>0&&p.height>0&&p.depth>0&&p.refractive_index>1;
 RenderCache::State::Key key;key.p=p;key.view=view;key.width=w;key.height=h;key.rounding=std::fegetround();
 if(refract){key.source=src.rgba.data();key.source_left=src.left;key.source_top=src.top;}
#if defined(__SSE__) || defined(_M_X64)
 key.mxcsr=_mm_getcsr()&~63u;
#endif
 // These are composition controls. None changes the water or photon field.
 key.p.output=key.p.blend=0;key.p.intensity=100;key.p.threshold=1;key.p.background=100;key.p.mix=100;
 key.p.light={.65,.9,1};key.p.dark={0,.04,.12};key.p.preserve_alpha=true;
 if(p.surface&&map){key.map=map->rgba.data();key.map_width=map->width;key.map_height=map->height;key.map_left=map->left;key.map_top=map->top;}
 std::shared_ptr<const Transport> transport;std::shared_ptr<Transport> building;
 if(cache)for(auto it=cache->state_->entries.begin();it!=cache->state_->entries.end();++it)if(it->key==key){
  transport=it->transport;auto entry=std::move(*it);cache->state_->entries.erase(it);cache->state_->entries.push_front(std::move(entry));++cache->hits_;break;
 }
 if(!transport){
 building=std::make_shared<Transport>();auto& grid=building->grid;auto& density=building->density;
 // Refraction and light focusing share the surface but have independent gains.
 const double margin=std::min(512.,p.depth*std::max(p.focus,p.refraction*.01)*std::sqrt(p.refractive_index*p.refractive_index-1))+step*4;
 grid.step=std::max(1.,(p.surface?std::max(2.,p.map_softness*.35):p.scale/(24*std::pow(2.,p.complexity-1))));
 grid.step=std::max(grid.step,std::sqrt((view.width+2*margin+8)*(view.height+2*margin+8)/4000000.));
 grid.left=std::floor((view.left-margin)/grid.step)*grid.step;grid.top=std::floor((view.top-margin)/grid.step)*grid.step;
 grid.width=int(std::ceil((view.left+view.width+margin-grid.left)/grid.step))+2;grid.height=int(std::ceil((view.top+view.height+margin-grid.top)/grid.step))+2;
 grid.nodes.resize(std::size_t(grid.width)*grid.height);Surface surface{p,map};
 if(!pscpu::parallel_ranges(grid.height,grid.nodes.size()*40,stop,[&](std::size_t lo,std::size_t hi,const auto& aborted){
  for(auto y=lo;y<hi;y++){if(aborted())return false;for(int x=0;x<grid.width;x++)grid.nodes[y*grid.width+x]=surface(grid.left+x*grid.step,grid.top+y*grid.step);}return true;
 }))return false;
 density.assign(std::size_t(w)*h,0);
 const bool flat=p.height==0||p.focus==0||p.depth==0||p.refractive_index==1;
 if(flat)std::fill(density.begin(),density.end(),1);
 else{
  // Generate ray contributions in parallel, then accumulate in the original
  // emitter order. No atomics or changed floating-point summation order.
  double cell=step/p.quality;cell=std::max(cell,std::sqrt((view.width+2*margin)*(view.height+2*margin)/80000000.));
  const auto x0=std::int64_t(std::floor((view.left-margin)/cell)),x1=std::int64_t(std::ceil((view.left+view.width+margin)/cell));
  const auto y0=std::int64_t(std::floor((view.top-margin)/cell)),y1=std::int64_t(std::ceil((view.top+view.height+margin)/cell));const double energy=cell*cell/(step*step);
  struct Contribution {int x=-2,y=-2;float weight[4]{};};
  constexpr std::int64_t batch=262144;const auto columns=x1-x0,total=columns*(y1-y0);
  std::vector<Contribution> contributions(std::size_t(std::min(batch,total)));
  for(std::int64_t first=0;first<total;first+=batch){
   if(stop())return false;const auto count=std::min(batch,total-first);
   if(!pscpu::parallel_ranges(std::size_t(count),std::size_t(count)*40,stop,[&](std::size_t lo,std::size_t hi,const auto& aborted){
   for(auto index=lo;index<hi;index++){
    if((index&2047)==0&&aborted())return false;const auto position=first+std::int64_t(index),x=x0+position%columns,y=y0+position/columns;auto& contribution=contributions[index];contribution.x=-2;
    // Stratified jitter removes regular emitter-grid patterns without animation noise.
    const auto seed=hash(std::uint32_t(x)*0x8da6b343u^std::uint32_t(y)*0xd8163841u^p.seed);
    const double jx=.5+.65*(double(seed&65535)/65535.-.5),jy=.5+.65*(double(seed>>16)/65535.-.5),xx=(x+jx)*cell,yy=(y+jy)*cell;
    const auto d=bend(grid.at(xx,yy),p,p.focus);const double px=(xx+d[0]-view.left)/step-.5,py=(yy+d[1]-view.top)/step-.5;
    if(px<=-1||py<=-1||px>=w||py>=h)continue;const int ix=int(std::floor(px)),iy=int(std::floor(py));const double tx=px-ix,ty=py-iy;
    contribution.x=ix;contribution.y=iy;for(int j=0;j<2;j++)for(int i=0;i<2;i++)contribution.weight[j*2+i]=float(energy*(i?tx:1-tx)*(j?ty:1-ty));
   }
   return true;}))return false;
   for(std::int64_t index=0;index<count;index++){
    if((index&4095)==0&&stop())return false;const auto& q=contributions[std::size_t(index)];if(q.x==-2)continue;
    for(int j=0;j<2;j++)for(int i=0;i<2;i++){const int u=q.x+i,v=q.y+j;if(u>=0&&u<w&&v>=0&&v<h)density[std::size_t(v)*w+u]+=q.weight[j*2+i];}
   }
  }
  if(!blur(density,w,h,std::hypot(p.softness/step,step>1?.45:0.),stop))return false;
 }
 if(cache&&refract&&building->bytes()+std::size_t(w)*h*sizeof(std::array<double,4>)<=cache->budget_){
  building->bottom.resize(std::size_t(w)*h);
  if(!pscpu::parallel_ranges(h,std::size_t(w)*h*40,stop,[&](std::size_t lo,std::size_t hi,const auto& aborted){
   for(auto y=lo;y<hi;y++){if(aborted())return false;for(int x=0;x<w;x++)building->bottom[y*w+x]=refracted(src,grid,p,view,view.left+(x+.5)*step,view.top+(y+.5)*step);}return true;
  }))return false;
 }
 transport=building;
 }
 const auto& grid=transport->grid;const auto& density=transport->density;
 if(!pscpu::parallel_ranges(h,std::size_t(w)*h*20,stop,[&](std::size_t lo,std::size_t hi,const auto& aborted){
  for(auto y=lo;y<hi;y++){
   if(aborted())return false;
   for(int x=0;x<w;x++){
    const auto at=(y*w+x)*4;const double xx=view.left+(x+.5)*step,yy=view.top+(y+.5)*step;const auto n=grid.at(xx,yy);
    const double concentration=std::max(0.,double(density[y*w+x])-p.threshold),coverage=1-std::exp(-concentration*p.intensity*.012);
    std::array<double,4> before{};for(int c=0;c<4;c++)before[c]=clean(src.rgba[at+c]);before[3]=unit(before[3]);auto bottom=before;
    if(p.output==0&&refract){
     // Inverse lookup with a damped fixed-point solve, avoiding a forward-warp hole mask.
     bottom=transport->bottom.empty()?refracted(src,grid,p,view,xx,yy):transport->bottom[y*w+x];
    }
    std::array<double,4> effect{};
    if(p.output==3){const double value=unit(.5+n.h/std::max(1e-9,2*p.height));effect={value,value,value,1};}
    else if(p.output==1){for(int c=0;c<3;c++)effect[c]=std::lerp(p.dark[c],p.light[c],coverage);effect[3]=p.preserve_alpha?before[3]:1;}
    else if(p.output==2){for(int c=0;c<3;c++)effect[c]=p.light[c];effect[3]=coverage*(p.preserve_alpha?before[3]:1);}
    else{
     for(int c=0;c<3;c++){
      const double base=bottom[c]*p.background*.01,light=p.light[c]*coverage;
      if(p.blend==0)effect[c]=base+(1-unit(base))*light;
      else if(p.blend==1)effect[c]=base+light;
      else if(p.blend==2)effect[c]=base*std::max(0.,1+(double(density[y*w+x])-1)*p.intensity*.01*p.light[c]);
      else effect[c]=base<.5?2*base*std::lerp(.5,p.light[c],coverage):1-2*(1-base)*(1-std::lerp(.5,p.light[c],coverage));
     }
     effect[3]=p.preserve_alpha?before[3]:bottom[3];
    }
    // Mix in associated RGB so transparent light output has no colored fringe.
    const double t=p.mix*.01,a=std::lerp(before[3],effect[3],t);result.rgba[at+3]=float(unit(a));
    for(int c=0;c<3;c++)result.rgba[at+c]=a>1e-12?float(std::lerp(before[c]*before[3],effect[c]*effect[3],t)/a):0;
   }
  }return true;
 }))return false;
 if(stop())return false;
 if(cache&&building){
  ++cache->builds_;const auto bytes=building->bytes();
  if(bytes<=cache->budget_){
   while(!cache->state_->entries.empty()&&(cache->state_->entries.size()>=2||cache->bytes_.load()+bytes>cache->budget_)){cache->bytes_-=cache->state_->entries.back().transport->bytes();cache->state_->entries.pop_back();}
   cache->state_->entries.push_front({key,std::move(building)});cache->bytes_+=bytes;
  }
 }
 out=std::move(result);return true;
}
}
