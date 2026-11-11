#include "roughen_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace roughen { namespace {
double unit(double x){return std::clamp(x,0.,1.);}
std::uint32_t hash(std::uint32_t h){h^=h>>16;h*=0x7feb352dU;h^=h>>15;h*=0x846ca68bU;return h^(h>>16);}
double lattice(int x,int y,int z,std::uint32_t seed){return (hash(hash(hash(std::uint32_t(x)^seed)^std::uint32_t(y)*0x9e3779b9U)^std::uint32_t(z)*0x85ebca6bU)>>8)*(2./16777215)-1;}
double noise(double x,double y,double z,std::uint32_t seed){
 x=std::remainder(x,1048576.);y=std::remainder(y,1048576.);z=std::remainder(z,1048576.);
 const int ix=int(std::floor(x)),iy=int(std::floor(y)),iz=int(std::floor(z));
 auto ease=[](double v){return v*v*(3-2*v);};const double u=ease(x-ix),v=ease(y-iy),t=ease(z-iz);double sum=0;
 for(int k=0;k<2;k++)for(int j=0;j<2;j++)for(int i=0;i<2;i++)sum+=lattice(ix+i,iy+j,iz+k,seed)*(i?u:1-u)*(j?v:1-v)*(k?t:1-t);return sum;
}
double field(double x,double y,const Parameters& p){
 // AE's percentage scale has a much finer spatial grain than a pixel-sized
 // lattice. Measured contour variograms put the 100% base cell near 5 px.
 x=(x-p.offset_x)/(p.scale*.05*std::pow(2.,std::max(0.,p.stretch)*.04));
 y=(y-p.offset_y)/(p.scale*.05*std::pow(2.,std::max(0.,-p.stretch)*.04));
 double sum=0,total=0,weight=1,f=1,z=p.evolution/90.;
 if(p.cycle){z=std::fmod(z,p.cycles*4);if(z<0)z+=p.cycles*4;}
 for(int i=0;i<int(std::ceil(p.complexity));i++){
  const double w=weight*std::min(1.,p.complexity-i),zz=z+i*.319;const auto seed=std::uint32_t(p.seed)+std::uint32_t(i)*0x9e3779b9U;
  double n=noise(x*f,y*f,zz,seed);
  if(p.cycle){const double phase=z/(p.cycles*4);n=(1-phase)*n+phase*noise(x*f,y*f,zz-p.cycles*4,seed);}
  sum+=w*n;total+=w;f*=2;weight*=.5;
 }return total?sum/total*(1+.15*(p.complexity-1)):0;
}
// Three fractional box passes approximate AE's measured alpha feather. Keep
// five independent fields: alpha, occupied pixels, and straight RGB. Occupancy
// provides the original edge color when the feather extends into transparency.
bool prepare(const Image& src,double border,Prepared& c,const Cancel& cancel){
 if(c.source==src.rgba.data()&&c.width==src.width&&c.height==src.height&&c.border==border)return true;
 c.source=nullptr;int x0=src.width,y0=src.height,x1=0,y1=0;
 for(int y=0;y<src.height;y++){
  if(cancel&&cancel())return false;
  for(int x=0;x<src.width;x++){auto i=(std::size_t(y)*src.width+x)*4;for(int k=0;k<4;k++)if(!std::isfinite(src.rgba[i+k]))return false;if(src.rgba[i+3]>0){x0=std::min(x0,x);y0=std::min(y0,y);x1=std::max(x1,x+1);y1=std::max(y1,y+1);}}
 }
 const double radius=border*.375;const int r=int(std::floor(radius)),halo=3*(r+1)+1;const double fraction=radius-r;
 c.left=std::max(0,x0-halo);c.top=std::max(0,y0-halo);c.fw=x1?std::min(src.width,x1+halo)-c.left:0;c.fh=y1?std::min(src.height,y1+halo)-c.top:0;
 const int w=c.fw,h=c.fh;c.field.assign(std::size_t(w)*h*5,0);
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  const auto s=(std::size_t(y+c.top)*src.width+x+c.left)*4,d=(std::size_t(y)*w+x)*5;double a=unit(src.rgba[s+3]);c.field[d]=float(a);c.field[d+1]=a>0?1:0;
  if(a>0)for(int k=0;k<3;k++)c.field[d+2+k]=src.rgba[s+k];
 }
 const double a=(1-fraction)/(2*r+1),b=fraction/(2*r+3);
 // Keep a zero halo through all three passes. Cutting off intermediate blur
 // at the document edge would lose mass and produce a different frame edge
 // from an identical shape edge inside the document. Separable line buffers
 // retain that halo without allocating an enlarged full-frame image.
 auto blur_line=[&](std::vector<float>& line,std::vector<float>& temp){
  const int n=int(line.size()/5);
  for(int pass=0;pass<3;pass++){
   auto get=[&](int x,int k){return x<0||x>=n?0.f:line[std::size_t(x)*5+k];};
   double sum[5]{},wide[5]{};
   for(int j=-r-1;j<=r+1;j++)for(int k=0;k<5;k++){const float value=get(j,k);wide[k]+=value;if(j>=-r&&j<=r)sum[k]+=value;}
   for(int x=0;x<n;x++)for(int k=0;k<5;k++){
    temp[std::size_t(x)*5+k]=float(sum[k]*a+wide[k]*b);
    sum[k]+=double(get(x+r+1,k))-get(x-r,k);wide[k]+=double(get(x+r+2,k))-get(x-r-1,k);
   }
   line.swap(temp);
  }
 };
 if(w&&h){
  if(!pscpu::parallel_ranges(h,std::size_t(w)*h*5,cancel,[&](std::size_t first,std::size_t last,const auto& stop){
   std::vector<float> line(std::size_t(w+2*halo)*5),temp(line.size());
   for(int y=int(first);y<int(last);y++){
    if(stop())return false;std::fill(line.begin(),line.end(),0);
    const auto row=std::size_t(y)*w*5;std::copy_n(c.field.begin()+row,std::size_t(w)*5,line.begin()+std::size_t(halo)*5);
    blur_line(line,temp);std::copy_n(line.begin()+std::size_t(halo)*5,std::size_t(w)*5,c.field.begin()+row);
   }return true;
  }))return false;
  if(!pscpu::parallel_ranges(w,std::size_t(w)*h*5,cancel,[&](std::size_t first,std::size_t last,const auto& stop){
   std::vector<float> line(std::size_t(h+2*halo)*5),temp(line.size());
   for(int x=int(first);x<int(last);x++){
    if(stop())return false;std::fill(line.begin(),line.end(),0);
    for(int y=0;y<h;y++)for(int k=0;k<5;k++)line[std::size_t(y+halo)*5+k]=c.field[(std::size_t(y)*w+x)*5+k];
    blur_line(line,temp);
    for(int y=0;y<h;y++)for(int k=0;k<5;k++)c.field[(std::size_t(y)*w+x)*5+k]=line[std::size_t(y+halo)*5+k];
   }return true;
  }))return false;
 }
 c.width=src.width;c.height=src.height;c.border=border;c.source=src.rgba.data();return true;
}
} // namespace
Parameters defaults(){return {};}
bool valid(const Parameters& p){
 for(double v:{p.border,p.sharpness,p.influence,p.scale,p.stretch,p.offset_x,p.offset_y,p.complexity,p.evolution,p.cycles,p.seed,p.mix,p.color[0],p.color[1],p.color[2]})if(!std::isfinite(v))return false;
 return p.type>=0&&p.type<8&&p.border>=0&&p.border<=500&&p.sharpness>=0&&p.sharpness<=10&&p.influence>=0&&p.influence<=1&&p.scale>=10&&p.scale<=1000&&std::abs(p.stretch)<=100&&std::abs(p.offset_x)<=1e7&&std::abs(p.offset_y)<=1e7&&p.complexity>=1&&p.complexity<=10&&std::abs(p.evolution)<=1e9&&p.cycles>=1&&p.cycles<=88&&std::floor(p.cycles)==p.cycles&&p.seed>=0&&p.seed<=100000&&std::floor(p.seed)==p.seed&&p.mix>=0&&p.mix<=100&&std::all_of(p.color.begin(),p.color.end(),[](double v){return v>=0&&v<=1;});
}
bool render(const Image& src,const Parameters& p,Image& output,Prepared* cache,const Cancel& cancel){
 if(!src.valid()||!valid(p)||(cancel&&cancel()))return false;
 if(p.mix==0){output=src;return true;}
 Prepared local;auto& c=cache?*cache:local;if(!prepare(src,p.border,c,cancel))return false;
 Image out=src;const double mix=p.mix*.01;
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(int y=int(first);y<int(last);y++){
   if(stop())return false;for(int x=0;x<src.width;x++){
    const auto i=(std::size_t(y)*src.width+x)*4;const double original=unit(src.rgba[i+3]);
    if(x<c.left||y<c.top||x>=c.left+c.fw||y>=c.top+c.fh)continue;
    const float* v=&c.field[(std::size_t(y-c.top)*c.fw+x-c.left)*5];const double alpha=unit(v[0]);
    double n=0;if(p.influence>0&&alpha>0&&alpha<1){
     if(p.scale<80){for(double dy:{-.25,.25})for(double dx:{-.25,.25})n+=field(src.left+x+dx,src.top+y+dy,p)*.25;}
     else n=field(src.left+x,src.top+y,p);
    }
    n*=p.type>=6?.85:.45;
    if(p.type==3)n=std::copysign(std::sqrt(std::abs(n)),n);
    // A soft edge still reacts to the fractal. Sharpness steepens its alpha
    // transfer, rather than changing the spatial radius of the feather cache.
    const double b=unit(alpha+(n*2.5+(p.type==3?.2:0))*p.influence*alpha*(1-alpha)),contrast=b+(b-.5)*10*p.sharpness;
    const double raw=b*contrast;double a=unit(raw),color=0;std::array<double,3> rgb{};
    for(int k=0;k<3;k++)rgb[k]=original>0?src.rgba[i+k]:(v[1]>1e-12?v[2+k]/v[1]:0);
    if(p.type==1)color=alpha>=1?0:unit(2-raw);
    if(p.type==2)a=original*unit(b+(b-.5)*(2+10*p.sharpness));
    if(p.type==4||p.type==5){const double rust=1+(alpha-p.influence*(.6+.5*n))*(2+10*p.sharpness);a=original*unit(rust);if(p.type==5&&p.influence>0)color=unit(2-rust);}
    if(p.type==6||p.type==7){
     // Photocopy is an alpha transfer, not an RGB luminance key: AE's red,
     // green, blue, orange, white and gray fixtures share the same opacity.
     const double ink=std::clamp((.555*original-.055-.486*alpha*alpha+p.influence*n*alpha*(1-alpha)*.4)*(8+2*p.sharpness)/3,0.,original);
     if(p.type==6)a=std::min(original,ink);else{a=original;color=original>0?1-unit(ink/original):0;}
    }
    for(int k=0;k<3;k++)rgb[k]=rgb[k]*(1-color)+p.color[k]*color;
    const double blended=original*(1-mix)+a*mix;
    for(int k=0;k<3;k++)out.rgba[i+k]=blended>1e-12?float((src.rgba[i+k]*original*(1-mix)+rgb[k]*a*mix)/blended):float(original>0?rgb[k]:0);
    out.rgba[i+3]=float(blended);
   }
  }return true;
 }))return false;
 output=std::move(out);return true;
}
}
