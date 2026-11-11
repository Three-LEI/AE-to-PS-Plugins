#include "jpeg_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
namespace jpegfx {
bool valid(const Parameters& p){
 auto between=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};
 return between(p.quality,1,100)&&between(p.resolution,1,64)&&between(p.aspect,.125,8)&&p.chroma>=0&&p.chroma<=3&&between(p.chroma_damage,0,400)&&
 between(p.all_freq,0,8)&&between(p.x_freq,0,8)&&between(p.y_freq,0,8)&&between(p.low_freq,0,8)&&between(p.mid_freq,0,8)&&between(p.high_freq,0,8)&&between(p.affect_luma,0,2)&&between(p.affect_chroma,0,2)&&
 between(p.error_rate,0,64)&&between(p.density,0,100)&&between(p.error_amp,0,10)&&between(p.coherence,0,32)&&between(p.block_shift,0,256)&&between(p.color_error,0,200)&&p.error_mode>=0&&p.error_mode<=5&&p.seed>=0&&p.seed<=1000000&&p.variation>=0&&p.variation<=1000000&&
 between(p.lights,0,8)&&between(p.darks,-1,1)&&between(p.saturation,0,4)&&between(p.mix,0,100)&&p.mask_mode>=0&&p.mask_mode<=1&&between(p.mask_blur,0,256);
}
Parameters preset(int i){
 Parameters p;
 switch(i){
 case 1:p.quality=3;p.resolution=2;break;
 case 2:p.quality=18;p.error_rate=2;p.density=35;p.coherence=3;p.error_amp=1.2;break;
 case 3:p.quality=25;p.error_rate=1;p.density=55;p.error_mode=3;p.block_shift=40;break;
 case 4:p.quality=7;p.chroma_damage=250;p.error_rate=2;p.error_mode=5;p.color_error=85;p.density=50;break;
 case 5:p.quality=35;p.resolution=2;p.high_freq=3;p.mid_freq=1.5;p.error_rate=.5;p.error_mode=4;p.density=25;break;
 default:break;
 }return p;
}
namespace {
struct Plane {int w=0,h=0;std::vector<float> data;Plane(int x,int y):w(x),h(y),data(std::size_t(x)*y){};float at(int x,int y)const{return data[std::size_t(std::clamp(y,0,h-1))*w+std::clamp(x,0,w-1)];}};
double finite(float v){return std::isfinite(v)?double(v):0.;}
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;return x^(x>>16);}
double unit(std::uint32_t n){return double(hash(n))/4294967296.;}
double signed_unit(std::uint32_t n){return unit(n)*2-1;}
double sample(const Plane& p,double x,double y){const int ix=int(std::floor(x)),iy=int(std::floor(y));const double fx=x-ix,fy=y-iy;return (p.at(ix,iy)*(1-fx)+p.at(ix+1,iy)*fx)*(1-fy)+(p.at(ix,iy+1)*(1-fx)+p.at(ix+1,iy+1)*fx)*fy;}
constexpr int luminance[64]={16,11,10,16,24,40,51,61,12,12,14,19,26,58,60,55,14,13,16,24,40,57,69,56,14,17,22,29,51,87,80,62,18,22,37,56,68,109,103,77,24,35,55,64,81,104,113,92,49,64,78,87,103,121,120,101,72,92,95,98,112,100,103,99};
constexpr int chrominance[64]={17,18,24,47,99,99,99,99,18,21,26,66,99,99,99,99,24,26,56,99,99,99,99,99,47,66,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99,99};
const auto basis=[] {std::array<std::array<double,8>,8> b{};for(int u=0;u<8;++u)for(int x=0;x<8;++x)b[u][x]=std::sqrt(u? .25:.125)*std::cos((2*x+1)*u*std::numbers::pi/16);return b;}();
bool transform(const Plane& input,Plane& output,int channel,const Parameters& p,const Cancel& cancel){
 const int blocks_x=(input.w+7)/8,blocks_y=(input.h+7)/8;
 std::array<double,64> steps{},gain{};const double scale=p.quality<50?50/p.quality:2-p.quality/50;
 for(int v=0;v<8;++v)for(int u=0;u<8;++u){const int k=v*8+u;steps[k]=std::max(1.,(channel?chrominance[k]:luminance[k])*scale*(channel?p.chroma_damage*.01:1.));
  const double band=u+v<4?p.low_freq:u+v<9?p.mid_freq:p.high_freq;
  const double g=p.all_freq*std::pow(p.x_freq,u/7.)*std::pow(p.y_freq,v/7.)*band;gain[k]=k?1+(g-1)*(channel?p.affect_chroma:p.affect_luma):1;
 }
 return pscpu::parallel_ranges(blocks_y,std::size_t(input.w)*input.h,cancel,[&](std::size_t begin,std::size_t end,const auto& stop){
  for(int by=int(begin);by<int(end);++by){if(stop())return false;
   for(int bx=0;bx<blocks_x;++bx){if((bx&31)==0&&stop())return false;
    const int ny=p.flip_noise?blocks_y-1-by:by,group=std::max(1,int(std::lround(1+p.coherence*2)));
    const auto seed=hash(std::uint32_t(p.seed)^hash(std::uint32_t(p.variation)+0xa12345U));
    const auto region=hash(std::uint32_t(bx/group)*0x9e3779b9U^std::uint32_t(ny/group)*0x85ebca6bU^seed);
    const auto block=hash(std::uint32_t(bx)*0x27d4eb2dU^std::uint32_t(ny)*0x165667b1U^seed);
    const bool broken=p.error_rate>0&&p.error_amp>0&&unit(region)<p.density*.01&&unit(block+71)<std::min(1.,p.error_rate*4);
    const int mode=p.error_mode?p.error_mode:1+int(hash(block)%5);
    int ox=bx*8,oy=by*8;
    if(broken&&mode==3){ox+=int(std::lround(signed_unit(block+17)*p.block_shift*p.error_amp));oy+=int(std::lround(signed_unit(block+23)*p.block_shift*p.error_amp*.2));}
    double data[8][8]{},temp[8][8]{},coeff[8][8]{};
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)data[y][x]=(input.at(ox+x,oy+y)-.5)*255;
    for(int y=0;y<8;++y)for(int u=0;u<8;++u)for(int x=0;x<8;++x)temp[y][u]+=data[y][x]*basis[u][x];
    for(int v=0;v<8;++v)for(int u=0;u<8;++u){double c=0;for(int y=0;y<8;++y)c+=temp[y][u]*basis[v][y];const int k=v*8+u;coeff[v][u]=std::round(c*gain[k]/steps[k])*steps[k];}
    if(broken){
     const auto cs=hash(block+std::uint32_t(channel)*0x5011U);const int events=std::clamp(int(std::ceil(p.error_rate*4)),1,64);
     if(mode==1)for(int e=0;e<events;++e){const auto r=hash(cs+std::uint32_t(e)*719);const int k=int(r%64);coeff[k/8][k%8]+=signed_unit(r+5)*steps[k]*p.error_amp*(2+std::sqrt(p.error_rate)*3);}
     if(mode==2)coeff[0][0]+=signed_unit(cs+11)*p.error_amp*640;
     if(mode==4)for(int k=1;k<64;++k)if(unit(cs+std::uint32_t(k)*113)<std::min(1.,p.error_rate*.2))coeff[k/8][k%8]=0;
     if(mode==5&&channel)coeff[0][0]+=signed_unit(cs+29)*p.color_error*.01*p.error_amp*900;
    }
    for(auto& row:temp)for(auto& x:row)x=0;
    for(int v=0;v<8;++v)for(int x=0;x<8;++x)for(int u=0;u<8;++u)temp[v][x]+=std::clamp(coeff[v][u],-65536.,65536.)*basis[u][x];
    for(int y=0;y<8&&by*8+y<input.h;++y)for(int x=0;x<8&&bx*8+x<input.w;++x){double value=0;for(int v=0;v<8;++v)value+=temp[v][x]*basis[v][y];output.data[std::size_t(by*8+y)*input.w+bx*8+x]=float(value/255+.5);}
   }
  }return true;
 });
}
bool blur_mask(Plane& p,int radius,const Cancel& cancel){
 if(!radius)return true;Plane tmp(p.w,p.h);
 for(int y=0;y<p.h;++y){if(cancel&&cancel())return false;double sum=0;for(int k=-radius;k<=radius;++k)sum+=p.at(k,y);for(int x=0;x<p.w;++x){tmp.data[std::size_t(y)*p.w+x]=float(sum/(radius*2+1));sum+=p.at(x+radius+1,y)-p.at(x-radius,y);}}
 for(int x=0;x<p.w;++x){if(cancel&&cancel())return false;double sum=0;for(int k=-radius;k<=radius;++k)sum+=tmp.at(x,k);for(int y=0;y<p.h;++y){p.data[std::size_t(y)*p.w+x]=float(sum/(radius*2+1));sum+=tmp.at(x,y+radius+1)-tmp.at(x,y-radius);}}return true;
}
}
bool render(const Image& src,const Parameters& p,Image& out,const Image* mask,const Cancel& cancel){
 if(!src.valid()||!valid(p)||(mask&&!mask->valid())||(cancel&&cancel()))return false;
 const bool neutral=p.quality==100&&p.resolution==1&&p.aspect==1&&p.chroma==0&&p.all_freq==1&&p.x_freq==1&&p.y_freq==1&&p.low_freq==1&&p.mid_freq==1&&p.high_freq==1&&(p.error_rate==0||p.error_amp==0||p.density==0)&&p.lights==1&&p.darks==0&&p.saturation==1&&p.preserve_hdr;
 if(p.mix==0||neutral){out=src;return true;}
 const double sx=std::max(1.,p.resolution*p.aspect),sy=p.resolution;const int w=std::max(1,int(std::ceil(src.width/sx))),h=std::max(1,int(std::ceil(src.height/sy)));
 // Area averaging before downsampling prevents high-frequency aliasing.
 Plane yy(w,h),cb(w,h),cr(w,h);
 if(!pscpu::parallel_ranges(h,std::size_t(src.width)*src.height,cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(int y=int(begin);y<int(end);++y){if(stop())return false;for(int x=0;x<w;++x){double rgb[3]{},alpha=0;const int x0=int(std::floor(x*sx)),x1=std::min(src.width,int(std::ceil((x+1)*sx))),y0=int(std::floor(y*sy)),y1=std::min(src.height,int(std::ceil((y+1)*sy)));
  for(int iy=y0;iy<y1;++iy)for(int ix=x0;ix<x1;++ix){const double weight=std::max(0.,std::min((x+1)*sx,double(ix+1))-std::max(x*sx,double(ix)))*std::max(0.,std::min((y+1)*sy,double(iy+1))-std::max(y*sy,double(iy)));const auto n=(std::size_t(iy)*src.width+ix)*4;const double a=std::clamp(finite(src.rgba[n+3]),0.,1.)*weight;for(int k=0;k<3;++k)rgb[k]+=std::clamp(finite(src.rgba[n+k]),0.,1.)*a;alpha+=a;}
  if(alpha>1e-12)for(auto& c:rgb)c/=alpha;const double l=.299*rgb[0]+.587*rgb[1]+.114*rgb[2];const auto n=std::size_t(y)*w+x;yy.data[n]=float(l);cb.data[n]=float(.5+(rgb[2]-l)/1.772);cr.data[n]=float(.5+(rgb[0]-l)/1.402);
 }}return true;}))return false;
 const int hx=p.chroma==1||p.chroma==2?2:1,hy=p.chroma==2?2:1;
 auto reduce=[&](const Plane& in){Plane r((w+hx-1)/hx,(h+hy-1)/hy);for(int y=0;y<r.h;++y)for(int x=0;x<r.w;++x){double sum=0;for(int j=0;j<hy;++j)for(int i=0;i<hx;++i)sum+=in.at(x*hx+i,y*hy+j);r.data[std::size_t(y)*r.w+x]=float(sum/(hx*hy));}return r;};
 auto ci=reduce(cb),ri=reduce(cr);std::vector<float>().swap(cb.data);std::vector<float>().swap(cr.data);Plane yo(w,h),co(ci.w,ci.h),ro(ri.w,ri.h);
 if(!transform(yy,yo,0,p,cancel))return false;
 if(p.chroma!=3&&(!transform(ci,co,1,p,cancel)||!transform(ri,ro,2,p,cancel)))return false;
 std::vector<float>().swap(yy.data);std::vector<float>().swap(ci.data);std::vector<float>().swap(ri.data);
 Plane matte(mask?src.width:1,mask?src.height:1);
 if(mask){for(int y=0;y<src.height;++y){if(cancel&&cancel())return false;for(int x=0;x<src.width;++x){const int mx=std::clamp(x+src.left-mask->left,0,mask->width-1),my=std::clamp(y+src.top-mask->top,0,mask->height-1);const auto n=(std::size_t(my)*mask->width+mx)*4;double a=0;
  if(x+src.left>=mask->left&&y+src.top>=mask->top&&x+src.left<mask->left+mask->width&&y+src.top<mask->top+mask->height){const double alpha=std::clamp(finite(mask->rgba[n+3]),0.,1.);a=p.mask_mode?alpha:std::clamp(.299*finite(mask->rgba[n])+.587*finite(mask->rgba[n+1])+.114*finite(mask->rgba[n+2]),0.,1.)*alpha;}matte.data[std::size_t(y)*src.width+x]=float(a);
 }}if(!blur_mask(matte,int(std::lround(p.mask_blur)),cancel))return false;}
 Image result=src;
 if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t begin,std::size_t end,const auto& stop){for(int y=int(begin);y<int(end);++y){if(stop())return false;for(int x=0;x<src.width;++x){const double gx=(x+.5)/sx-.5,gy=(y+.5)/sy-.5,l=sample(yo,gx,gy);double b=p.chroma==3?0:sample(co,(gx+.5)/hx-.5,(gy+.5)/hy-.5)-.5,r=p.chroma==3?0:sample(ro,(gx+.5)/hx-.5,(gy+.5)/hy-.5)-.5;
  double rgb[3]={l+1.402*r,l-.344136*b-.714136*r,l+1.772*b};const auto n=(std::size_t(y)*src.width+x)*4;const double m=(mask?(p.invert_mask?1-matte.data[std::size_t(y)*src.width+x]:matte.data[std::size_t(y)*src.width+x]):1)*p.mix*.01;
  if(m==0)continue;
  for(int k=0;k<3;++k){const double original=finite(src.rgba[n+k]),value=std::clamp(rgb[k],0.,1.);double processed=(l+(value-l)*p.saturation)*p.lights+p.darks*(1-std::clamp(l,0.,1.));processed=std::clamp(processed,0.,1.);if(p.preserve_hdr)processed+=original-std::clamp(original,0.,1.);result.rgba[n+k]=float(original+(processed-original)*m);}
 }}return true;}))return false;
 out=std::move(result);return true;
}
}
