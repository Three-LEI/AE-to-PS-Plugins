#include "creative_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace creative {
namespace {
constexpr double pi=3.14159265358979323846;
double smooth(double x){return x*x*(3-2*x);}
double fold(double x,double size){x=std::fmod(x,2*size);if(x<0)x+=2*size;return x<=size?x:2*size-x;}
std::uint32_t hash(std::uint32_t h){h^=h>>16;h*=0x7feb352dU;h^=h>>15;h*=0x846ca68bU;return h^(h>>16);}
double lattice(std::int64_t x,std::int64_t y,std::int64_t z,std::uint32_t seed){auto h=hash(std::uint32_t(x)^seed);h=hash(h^std::uint32_t(y)*0x9e3779b9U);h=hash(h^std::uint32_t(z)*0x85ebca6bU);return (h>>8)*(2./16777215)-1;}
double noise(double x,double y,double z,std::uint32_t seed,bool smoother=false){
 x=std::remainder(x,1048576.);y=std::remainder(y,1048576.);z=std::remainder(z,1048576.);
 const auto ix=std::int64_t(std::floor(x)),iy=std::int64_t(std::floor(y)),iz=std::int64_t(std::floor(z));
 auto ease=[&](double v){return smoother?v*v*v*(v*(v*6-15)+10):smooth(v);};const double u=ease(x-ix),v=ease(y-iy),t=ease(z-iz);double sum=0;
 for(int k=0;k<2;k++)for(int j=0;j<2;j++)for(int i=0;i<2;i++)sum+=lattice(ix+i,iy+j,iz+k,seed)*(i?u:1-u)*(j?v:1-v)*(k?t:1-t);return sum;
}
double field(double x,double y,const Parameters& p,std::uint32_t channel){
 double result=0,weight=1,total=0,frequency=1;const auto seed=std::uint32_t(p.seed)^channel;
 const double time=p.evolution/90.;double z=time;if(p.cycle){z=std::fmod(time,p.cycles*4);if(z<0)z+=p.cycles*4;}
 for(int i=0;i<int(std::ceil(p.complexity));i++){
  const bool smoother=p.type>=3&&p.type<=5;const double fraction=std::min(1.,p.complexity-i);double n=noise(x*frequency,y*frequency,z+i*.319,seed+std::uint32_t(i)*0x9e3779b9U,smoother);
  if(p.cycle){const double period=p.cycles*4,phase=z/period;n=(1-phase)*n+phase*noise(x*frequency,y*frequency,z-period+i*.319,seed+std::uint32_t(i)*0x9e3779b9U,smoother);}
  result+=n*weight*fraction;total+=weight*fraction;weight*=.5;frequency*=2;
 }return total?result/total:0;
}
std::array<double,2> pin(double x,double y,const Parameters& p,const Image& im){
 const double w=std::max(1.,std::min(p.size,std::min(im.width,im.height)*.25));std::array<double,2> a{1,1};
 auto edge=[&](double distance,int axis){const double factor=smooth(std::clamp(distance/w,0.,1.));if(p.pinning>=8){a[0]*=factor;a[1]*=factor;}else a[axis]*=factor;};
 constexpr int masks[]={0,15,12,3,1,2,4,8,15,12,3,1,2,4,8};const int mask=masks[p.pinning];
 if(mask&1)edge(x-im.left,0);if(mask&2)edge(im.left+im.width-1-x,0);if(mask&4)edge(y-im.top,1);if(mask&8)edge(im.top+im.height-1-y,1);
 return a;
}
std::array<float,4> sample(const Image& im,double x,double y,int mode,bool nearest=false){
 x-=im.left;y-=im.top;if(!std::isfinite(x)||!std::isfinite(y))return {};
 auto index=[&](int v,int n){if(mode>0&&n==1)return 0;if(mode==1)return std::clamp(v,0,n-1);if(mode==2){v%=n;if(v<0)v+=n;return v;}if(mode==3)return int(std::lround(fold(v,n-1)));return v;};
 if(mode==0&&(x<=-1||y<=-1||x>=im.width||y>=im.height))return {};
 if(nearest){x=std::round(x);y=std::round(y);}const int ix=int(std::floor(x)),iy=int(std::floor(y));const double u=x-ix,v=y-iy;std::array<double,4> sum{};
 for(int j=0;j<2;j++)for(int i=0;i<2;i++){const int xx=index(ix+i,im.width),yy=index(iy+j,im.height);if(xx<0||yy<0||xx>=im.width||yy>=im.height)continue;
  const auto n=(std::size_t(yy)*im.width+xx)*4;const double a=std::clamp(double(im.rgba[n+3]),0.,1.),w=(i?u:1-u)*(j?v:1-v);for(int k=0;k<3;k++)sum[k]+=w*a*im.rgba[n+k];sum[3]+=w*a;
 }
 std::array<float,4> out{};if(sum[3]>1e-12)for(int k=0;k<3;k++)out[k]=float(sum[k]/sum[3]);out[3]=float(sum[3]);return out;
}
std::array<float,4> blend(const float* a,std::array<float,4> b,double mix){if(mix==1)return b;const double alpha=a[3]*(1-mix)+b[3]*mix;for(int k=0;k<3;k++)b[k]=alpha>1e-12?float((a[k]*a[3]*(1-mix)+b[k]*b[3]*mix)/alpha):0;b[3]=float(alpha);return b;}
}
Parameters defaults(Kind kind,int w,int h,int left,int top){Parameters p;p.kind=kind;p.cx=left+w*.5;p.cy=top+h*.5;if(kind==Kind::turbulent)p.pinning=1;if(kind==Kind::sweep)p.cy=top+h*.25;if(kind==Kind::kaleida)p.size=20;return p;}
bool valid(const Parameters& p){
 if(int(p.kind)<0||int(p.kind)>2||p.type<0||p.type>8||p.pinning<0||p.pinning>14||p.edge<0||p.edge>3||p.shape<0||p.shape>2||p.reception<0||p.reception>2||p.mirroring<0||p.mirroring>8||p.quality<0||p.quality>2)return false;
 for(double v:{p.cx,p.cy,p.amount,p.size,p.complexity,p.evolution,p.cycles,p.seed,p.horizontal,p.vertical,p.mix,p.direction,p.width,p.intensity,p.edge_intensity,p.thickness,p.color[0],p.color[1],p.color[2],p.rotation})if(!std::isfinite(v))return false;
 return std::abs(p.cx)<=1e7&&std::abs(p.cy)<=1e7&&std::abs(p.amount)<=1e4&&p.size>=1&&p.size<=1e4&&p.complexity>=1&&p.complexity<=10&&std::abs(p.evolution)<=1e9&&p.cycles>=1&&p.cycles<=100&&std::floor(p.cycles)==p.cycles&&p.seed>=0&&p.seed<=2147483647&&std::floor(p.seed)==p.seed&&p.horizontal>=-500&&p.horizontal<=500&&p.vertical>=-500&&p.vertical<=500&&p.mix>=0&&p.mix<=100&&std::abs(p.direction)<=360000&&p.width>=0&&p.width<=4000&&p.intensity>=0&&p.intensity<=500&&p.edge_intensity>=0&&p.edge_intensity<=500&&p.thickness>=0&&p.thickness<=20&&std::abs(p.rotation)<=360000&&std::all_of(p.color.begin(),p.color.end(),[](double v){return v>=0&&v<=16;});
}
std::array<double,2> warp(double x,double y,const Parameters& p,const Image& im){
 if(p.kind==Kind::turbulent){
  const double nx=(x-p.cx)/p.size,ny=(y-p.cy)/p.size;double dx=0,dy=0;
  if(p.type==1||p.type==2||p.type==4||p.type==5){const double e=.05;dx=(field(nx+e,ny,p,0)-field(nx-e,ny,p,0))/(2*e);dy=(field(nx,ny+e,p,0)-field(nx,ny-e,p,0))/(2*e);if(p.type==2||p.type==5){double t=dx;dx=-dy;dy=t;}dx*=.5;dy*=.5;}
  else if(p.type>=6){dx=field(0,ny,p,0);dy=field(nx,0,p,0x9e3779b9U);}else {dx=field(nx,ny,p,0);dy=field(nx+17.31,ny-29.73,p,0x9e3779b9U);}
  if(p.type==0||p.type==3)dx=-dx;
  if(p.type==6)dx=0;if(p.type==7)dy=0;
  // Match the measured displacement energy, not AE's proprietary random lattice.
  const double gain=(p.type==0?.35:p.type==3?.6:p.type==1||p.type==2?-.45:p.type==4||p.type==5?.16:.5)*(1+.75*(p.complexity-1));
  const auto pinned=pin(x,y,p,im);return {x+dx*p.amount*gain*pinned[0]*p.horizontal*.01,y+dy*p.amount*gain*pinned[1]*p.vertical*.01};
 }
 if(p.kind==Kind::kaleida){
  const double scale=p.size*.005*im.width,r=p.rotation*pi/180,cs=std::cos(r),sn=std::sin(r);
  const double ox=p.floating?p.cx:im.left+im.width*.5,oy=p.floating?p.cy:im.top+im.height*.5,dx=x-ox,dy=y-oy;
  const double a=fold(dx,scale),b=fold(dy,scale);double u=-a,v=-b;const auto ix=std::int64_t(std::floor(dx/scale)),iy=std::int64_t(std::floor(dy/scale));
  auto saw=[&](double t){t=std::fmod(t,scale);if(t<0)t+=scale;return t;};
  if(p.mirroring==1){bool swap=(ix+iy)%2!=0;if(dy==0&&dx<0)swap=!swap;if(swap)std::swap(u,v);}
  if(p.mirroring==2&&ix%2!=0)std::swap(u,v);
  if(p.mirroring==3&&(iy%2!=0||dy==0))std::swap(u,v);
  if(p.mirroring==4)v=dy==0?-scale:-saw(dy);
  if(p.mirroring==5||p.mirroring==6){u=-std::min(a,b);v=-std::max(a,b);if(p.mirroring==6&&(ix+iy)%2!=0)u=-u;}
  if(p.mirroring==7){const double value=dx==0?scale:saw(dx);const bool reverse=dy==0||iy%2!=0;u=reverse?-(scale-value):-value;}
  if(p.mirroring==8){const double lo=std::min(a,b),hi=std::max(a,b);u=-lo;v=-hi;if(lo>hi*std::tan(pi/8)){u=-(hi-lo)*std::sqrt(.5);v=-(hi+lo)*std::sqrt(.5);}}
  return {p.cx+cs*u-sn*v,p.cy+sn*u+cs*v};
 }return {x,y};
}
bool render(const Image& src,const Parameters& p,Image& output,const Cancel& cancel){
 if(!src.valid()||!valid(p)||(cancel&&cancel()))return false;
 if(p.mix==0||(p.kind==Kind::turbulent&&(p.amount==0||(p.horizontal==0&&p.vertical==0)))||(p.kind==Kind::sweep&&p.intensity==0&&p.edge_intensity==0&&p.reception!=2)){output=src;return true;}
 for(float v:src.rgba)if(!std::isfinite(v))return false;
 Image out{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};const double r=p.direction*pi/180,nx=std::cos(r),ny=std::sin(r);
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(int y=int(first);y<int(last);y++){if(stop())return false;for(int x=0;x<src.width;x++){
   const auto i=(std::size_t(y)*src.width+x)*4;const double wx=src.left+x,wy=src.top+y;std::array<float,4> pixel;
   if(p.kind==Kind::sweep){
    const double d=std::abs((wx-p.cx)*nx+(wy-p.cy)*ny),q=p.width>0?std::max(0.,1-d/(p.width*2)):0;
    const double profile=p.shape==0?q:p.shape==1?smooth(q):q*q;double edge=0;
    if(p.thickness>0&&p.edge_intensity>0){
     const double e=p.thickness*.5,gx=double(sample(src,wx+e,wy,0)[3])-sample(src,wx-e,wy,0)[3],gy=double(sample(src,wx,wy+e,0)[3])-sample(src,wx,wy-e,0)[3];
     // Alpha bevel: ambient rim plus a light vector along the sweep tangent.
     // The measured beam profile gates the edge light as well as the body.
     edge=profile*std::max(0.,.55*std::max(std::abs(gx),std::abs(gy))-gx*ny+gy*nx);
    }
    const double light=profile*p.intensity*.01+edge*p.edge_intensity*.01;
    for(int k=0;k<3;k++){const double base=src.rgba[i+k];pixel[k]=float(p.reception==2?p.color[k]:p.reception==1?base*(1-std::min(1.,light))+p.color[k]*std::min(1.,light):base+light*p.color[k]);}
    pixel[3]=p.reception==2?float(src.rgba[i+3]*std::clamp(light,0.,1.)):src.rgba[i+3];
   }else if(p.quality==2){std::array<double,4> sum{};for(double oy:{-.25,.25})for(double ox:{-.25,.25}){const auto at=warp(wx+ox,wy+oy,p,src);const auto a=sample(src,at[0],at[1],p.edge);for(int k=0;k<3;k++)sum[k]+=a[k]*a[3]*.25;sum[3]+=a[3]*.25;}pixel={};if(sum[3]>1e-12)for(int k=0;k<3;k++)pixel[k]=float(sum[k]/sum[3]);pixel[3]=float(sum[3]);}
   else{const auto at=warp(wx,wy,p,src);pixel=sample(src,at[0],at[1],p.edge,p.quality==0);}
   pixel=blend(&src.rgba[i],pixel,p.mix*.01);std::copy(pixel.begin(),pixel.end(),out.rgba.begin()+i);
  }}return true;
 }))return false;
 output=std::move(out);return true;
}
}
