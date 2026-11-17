#include "dither_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
namespace dither {namespace {
using Pixel=std::array<float,3>;
double unit(double x){return std::clamp(x,0.,1.);}
double finite(float x){return std::isfinite(x)?double(x):0.;}
bool stop(const Cancel& c){return c&&c();}
constexpr Color weights{.2126,.7152,.0722};
double dot(const Color& a,const Color& b){double v=0;for(int k=0;k<3;k++)v+=weights[k]*a[k]*b[k];return v;}
Color sub(const Color& a,const Color& b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
Color lerp(const Color& a,const Color& b,double t){return {std::lerp(a[0],b[0],t),std::lerp(a[1],b[1],t),std::lerp(a[2],b[2],t)};}
Color working(Color a,bool linear){if(linear)for(auto& v:a)v=std::pow(unit(v),2.2);return a;}
Color tone(const Pixel& raw,const Parameters& p){Color c{unit(raw[0]),unit(raw[1]),unit(raw[2])};const double l=c[0]*weights[0]+c[1]*weights[1]+c[2]*weights[2];for(auto& v:c)v=std::pow(unit((std::lerp(l,v,p.saturation*.01)-.5)*p.contrast*.01+.5+p.brightness*.01),1/p.gamma);return working(c,p.linear_light);}
struct Pattern {
 int n=8;std::vector<int> ranks;std::vector<std::vector<double>> prefix;
 static int bayer(int x,int y,int size){constexpr int b[4]={0,2,3,1};int rank=0;for(int scale=1;scale<size;scale*=2)rank=rank*4+b[((y/scale)&1)*2+((x/scale)&1)];return rank;}
 explicit Pattern(int kind){
  n=kind<3?2<<kind:8;ranks.resize(n*n);
  if(kind<3)for(int y=0;y<n;y++)for(int x=0;x<n;x++)ranks[y*n+x]=bayer(x,y,n);
  else{
   std::vector<int> order(n*n);std::iota(order.begin(),order.end(),0);
   auto metric=[&](int i){const int x=i%n,y=i/n;const double a=x-(n-1)*.5,b=y-(n-1)*.5;if(kind==3)return a*a+b*b;if(kind==4)return double((x+y)&1);if(kind==5)return double(y);if(kind==6)return double(x);if(kind==7)return double((x+y)%n);return std::min(std::abs(a),std::abs(b));};
   std::stable_sort(order.begin(),order.end(),[&](int a,int b){auto x=metric(a),y=metric(b);return x!=y?x<y:bayer(a%n,a/n,n)<bayer(b%n,b/n,n);});for(int i=0;i<n*n;i++)ranks[order[i]]=i;
  }
  prefix.resize(n*n+1,std::vector<double>(std::size_t(n+1)*(n+1)));
  for(int k=0;k<=n*n;k++)for(int y=0;y<n;y++)for(int x=0;x<n;x++)prefix[k][(y+1)*(n+1)+x+1]=(ranks[y*n+x]<k?1:0)+prefix[k][y*(n+1)+x+1]+prefix[k][(y+1)*(n+1)+x]-prefix[k][y*(n+1)+x];
 }
 double threshold(std::int64_t x,std::int64_t y,double amount)const{const auto xx=std::uint64_t(x)&std::uint64_t(n-1),yy=std::uint64_t(y)&std::uint64_t(n-1);return std::lerp(.5,(ranks[yy*n+xx]+.5)/(n*n),amount);}
 double fraction(double left,double top,double extent,double t,double amount)const{
  if(amount<=0)return t>=.5?1:0;
  const int k=std::clamp(int(std::floor(((t-.5)/amount+.5)*n*n-.5))+1,0,n*n);if(k==0)return 0;if(k==n*n)return 1;
  left=std::fmod(left,n);if(left<0)left+=n;top=std::fmod(top,n);if(top<0)top+=n;
  const auto& data=prefix[k];auto partial=[&](double x,double y){const int ix=std::min(n-1,int(x)),iy=std::min(n-1,int(y));const double u=x-ix,v=y-iy;const auto i=iy*(n+1)+ix;return std::lerp(std::lerp(data[i],data[i+1],u),std::lerp(data[i+n+1],data[i+n+2],u),v);};
  auto sum=[&](double x,double y){double qx=std::floor(x/n),qy=std::floor(y/n),rx=x-qx*n,ry=y-qy*n;return qx*qy*k+qx*partial(n,ry)+qy*partial(rx,n)+partial(rx,ry);};
  return unit((sum(left+extent,top+extent)-sum(left,top+extent)-sum(left+extent,top)+sum(left,top))/(extent*extent));
 }
};
struct Palette {
 const Parameters& p;std::vector<Color> display,space;
 explicit Palette(const Parameters& values):p(values),display(palette_colors(p)){for(auto c:display)space.push_back(working(c,p.linear_light));}
 std::size_t nearest(const Color& value)const{
  std::size_t best=0;double error=1e100;for(std::size_t i=0;i<space.size();i++){auto d=sub(value,space[i]);auto e=dot(d,d);if(e<error){best=i;error=e;}}return best;
 }
 template<class Mix>Color channel_quantize(const Color& value,Mix mix)const{
  const int count=p.levels+2;auto one=[&](double v){int hi=0;while(hi<count&&space[p.palette==2?std::size_t(hi)*count*count:hi][0]<v)hi++;if(hi==0)return 0.;if(hi==count)return 1.;const double a=space[p.palette==2?std::size_t(hi-1)*count*count:hi-1][0],b=space[p.palette==2?std::size_t(hi)*count*count:hi][0];return std::lerp(double(hi-1)/(count-1),double(hi)/(count-1),mix(unit((v-a)/(b-a))));};
  if(p.palette==1){double v=one(value[0]*weights[0]+value[1]*weights[1]+value[2]*weights[2]);return {v,v,v};}return {one(value[0]),one(value[1]),one(value[2])};
 }
 template<class Mix>Color quantize(const Color& value,Mix mix)const{
  if(p.palette==1||p.palette==2)return channel_quantize(value,mix);
  const auto a=nearest(value);std::size_t b=a;double fraction=0,best=dot(sub(value,space[a]),sub(value,space[a]));
  for(std::size_t j=0;j<space.size();j++)if(j!=a){const auto direction=sub(space[j],space[a]);const double length=dot(direction,direction);if(length<=1e-20)continue;const double t=unit(dot(sub(value,space[a]),direction)/length);const auto delta=sub(value,lerp(space[a],space[j],t));const double error=dot(delta,delta);if(error<best-1e-16){best=error;b=j;fraction=t;}}
  return lerp(display[a],display[b],mix(fraction));
 }
 Color closest(const Color& value)const{if(p.palette==1||p.palette==2)return channel_quantize(value,[](double t){return t>.5?1.:0.;});return display[nearest(value)];}
};
}
bool valid(const Parameters& p){auto range=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};return p.algorithm>=0&&p.algorithm<4&&p.pattern>=0&&p.pattern<9&&p.palette>=0&&p.palette<6&&p.levels>=0&&p.levels<(p.palette==2?7:15)&&range(p.pixel_size,1e-6,64)&&range(p.amount,0,200)&&range(p.brightness,-100,100)&&range(p.contrast,0,400)&&range(p.gamma,.1,5)&&range(p.saturation,0,200)&&range(p.shift_x,-1e7,1e7)&&range(p.shift_y,-1e7,1e7)&&range(p.mix,0,100)&&std::all_of(p.colors.begin(),p.colors.end(),[&](const Color& c){return std::all_of(c.begin(),c.end(),[&](double v){return range(v,0,1);});});}
std::vector<Color> palette_colors(const Parameters& p){
 if(p.palette==0)return {p.colors[0],p.colors[1]};std::vector<Color> out;const int n=p.levels+2;
 if(p.palette==1)for(int i=0;i<n;i++){double v=double(i)/(n-1);out.push_back({v,v,v});}
 else if(p.palette==2)for(int r=0;r<n;r++)for(int g=0;g<n;g++)for(int b=0;b<n;b++)out.push_back({double(r)/(n-1),double(g)/(n-1),double(b)/(n-1)});
 else if(p.palette==3)out={Color{.07,.15,.09},Color{.24,.36,.17},Color{.55,.68,.33},Color{.86,.92,.68}};
 else if(p.palette==4)out={Color{.025,.055,.16},Color{.1,.22,.6},Color{.38,.66,.88},Color{.77,.98,1}};
 else out.assign(p.colors.begin(),p.colors.end());return out;
}
Parameters preset(int i){Parameters p;if(i==0){p.pattern=2;}if(i==1){p.palette=1;p.algorithm=1;}if(i==2){p.palette=3;p.pixel_size=3;}if(i==3){p.palette=4;p.pattern=2;p.pixel_size=3;p.amount=70;}if(i==4){p.palette=2;p.levels=0;p.pixel_size=3;p.saturation=110;}if(i==5){p.pattern=6;p.pixel_size=3;}return p;}
bool render(const Image& src,const Parameters& p,Image& out,const Cancel& cancel){
 pscpu::ParallelScope pool;if(!src.valid()||!valid(p)||stop(cancel))return false;if(p.mix==0){out=src;return true;}
 const Palette palette(p);const Pattern pattern(p.pattern);const double amount=p.amount*.01,block=p.pixel_size;
 Image result{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};const double mix=p.mix*.01;
 auto write=[&](std::size_t i,const Color& c){const double a=unit(finite(src.rgba[i+3]));for(int k=0;k<3;k++)result.rgba[i+k]=a>0?float(std::lerp(finite(src.rgba[i+k]),c[k],mix)):0;result.rgba[i+3]=float(a);};
 if(block<1){
  if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){for(auto y=lo;y<hi;y++){if(cancelled())return false;for(int x=0;x<src.width;x++){const auto i=(y*src.width+x)*4;const Pixel raw{float(finite(src.rgba[i])),float(finite(src.rgba[i+1])),float(finite(src.rgba[i+2]))};const auto v=tone(raw,p);Color c;
   if(p.algorithm==0&&amount>0)c=palette.quantize(v,[&](double t){return pattern.fraction((src.left+double(x)-p.shift_x)/block,(src.top+double(y)-p.shift_y)/block,1/block,t,amount);});
   else{c=palette.closest(v);if(p.algorithm!=3&&amount>0)c=lerp(c,palette.quantize(v,[](double t){return t;}),std::min(1.,amount));}write(i,c);
  }}return true;}))return false;
 }else{
  const auto gx=std::int64_t(std::floor((src.left-p.shift_x)/block)),gy=std::int64_t(std::floor((src.top-p.shift_y)/block));const double nx=std::ceil((double(src.left)+src.width-p.shift_x)/block)-gx,ny=std::ceil((double(src.top)+src.height-p.shift_y)/block)-gy;
  if(nx<1||ny<1||nx*ny>20000000)return false;const auto cols=std::size_t(nx),rows=std::size_t(ny);std::vector<Pixel> pixels(cols*rows);std::vector<unsigned char> visible(pixels.size());
  if(!pscpu::parallel_ranges(rows,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){for(auto y=lo;y<hi;y++){if(cancelled())return false;for(std::size_t x=0;x<cols;x++){
   const double l=p.shift_x+(gx+std::int64_t(x))*block-src.left,t=p.shift_y+(gy+std::int64_t(y))*block-src.top;const int x0=std::clamp(int(std::floor(l)),0,src.width),x1=std::clamp(int(std::ceil(l+block)),0,src.width),y0=std::clamp(int(std::floor(t)),0,src.height),y1=std::clamp(int(std::ceil(t+block)),0,src.height);Color sum{};double weight=0;
   for(int yy=y0;yy<y1;yy++)for(int xx=x0;xx<x1;xx++){const auto i=(std::size_t(yy)*src.width+xx)*4;const double area=std::max(0.,std::min(l+block,xx+1.)-std::max(l,double(xx)))*std::max(0.,std::min(t+block,yy+1.)-std::max(t,double(yy))),a=unit(finite(src.rgba[i+3]))*area;weight+=a;for(int k=0;k<3;k++)sum[k]+=finite(src.rgba[i+k])*a;}
   const auto n=y*cols+x;visible[n]=weight>1e-12;if(visible[n])for(int k=0;k<3;k++)pixels[n][k]=float(unit(sum[k]/weight));
  }}return true;}))return false;
  if((p.algorithm==1||p.algorithm==2)&&amount>0){
   std::vector<Color> errors(cols+4),next(cols+4),second(cols+4);
   for(std::size_t y=0;y<rows;y++){
    if(stop(cancel))return false;const bool reverse=p.serpentine&&((gy+std::int64_t(y))&1);const int sign=reverse?-1:1;
    for(std::size_t j=0;j<cols;j++){const auto x=reverse?cols-1-j:j,n=y*cols+x,at=x+2;if(!visible[n])continue;auto v=tone(pixels[n],p);for(int k=0;k<3;k++)v[k]=unit(v[k]+errors[at][k]);const auto color=palette.closest(v),q=working(color,p.linear_light);for(int k=0;k<3;k++){pixels[n][k]=float(color[k]);const double e=(v[k]-q[k])*amount;
     if(p.algorithm==1){errors[at+sign][k]+=e*7/16;next[at-sign][k]+=e*3/16;next[at][k]+=e*5/16;next[at+sign][k]+=e/16;}
     else{errors[at+sign][k]+=e/8;errors[at+2*sign][k]+=e/8;next[at-sign][k]+=e/8;next[at][k]+=e/8;next[at+sign][k]+=e/8;second[at][k]+=e/8;}
    }}errors.swap(next);next.swap(second);std::fill(second.begin(),second.end(),Color{});
   }
  }else if(!pscpu::parallel_ranges(rows,pixels.size(),cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){for(auto y=lo;y<hi;y++){if(cancelled())return false;for(std::size_t x=0;x<cols;x++){const auto n=y*cols+x;if(!visible[n])continue;const auto v=tone(pixels[n],p);const double threshold=pattern.threshold(gx+std::int64_t(x),gy+std::int64_t(y),amount);const auto c=p.algorithm==0&&amount>0?palette.quantize(v,[&](double t){return t>=threshold?1.:0.;}):palette.closest(v);for(int k=0;k<3;k++)pixels[n][k]=float(c[k]);}}return true;}))return false;
  if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){for(auto y=lo;y<hi;y++){if(cancelled())return false;const auto yy=std::clamp(std::int64_t(std::floor((src.top+double(y)+.5-p.shift_y)/block))-gy,std::int64_t(0),std::int64_t(rows)-1);for(int x=0;x<src.width;x++){const auto xx=std::clamp(std::int64_t(std::floor((src.left+double(x)+.5-p.shift_x)/block))-gx,std::int64_t(0),std::int64_t(cols)-1);const auto& c=pixels[std::size_t(yy)*cols+std::size_t(xx)];write((y*src.width+x)*4,{c[0],c[1],c[2]});}}return true;}))return false;
 }
 if(stop(cancel))return false;out=std::move(result);return true;
}
}
