#include "ascii_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
namespace asciifx {
namespace {
double unit(double x){return std::clamp(x,0.,1.);}
double finite(float x){return std::isfinite(x)?double(x):0.;}
bool stop(const Cancel& c){return c&&c();}
struct Cell {std::array<float,3> rgb{};double value=0;bool visible=false;std::size_t glyph=0;};
std::size_t nearest(const GlyphAtlas& atlas,double value){
 value=unit(value);auto at=std::lower_bound(atlas.glyphs.begin(),atlas.glyphs.end(),value,[](const Glyph& g,double v){return g.level<v;});
 if(at==atlas.glyphs.begin())return 0;if(at==atlas.glyphs.end())return atlas.glyphs.size()-1;
 const auto hi=std::size_t(at-atlas.glyphs.begin());return value-atlas.glyphs[hi-1].level<=at->level-value?hi-1:hi;
}
}
void Glyph::assign(int w,int h,const std::vector<float>& mask){
 width=w;height=h;integral.clear();density=level=0;if(w<1||h<1||w>512||h>512||mask.size()!=std::size_t(w)*h)return;
 integral.assign(std::size_t(w+1)*(h+1),0);
 for(int y=0;y<h;y++){double row=0;for(int x=0;x<w;x++){row+=unit(finite(mask[std::size_t(y)*w+x]));integral[std::size_t(y+1)*(w+1)+x+1]=integral[std::size_t(y)*(w+1)+x+1]+row;}}
 density=integral.back()/(w*h);
}
double Glyph::area(double left,double top,double right,double bottom)const{
 if(right<=left||bottom<=top)return 0;
 auto sum=[&](double x,double y){
  x=std::clamp(x*width,0.,double(width));y=std::clamp(y*height,0.,double(height));
  const int ix=std::min(int(x),width-1),iy=std::min(int(y),height-1);const double u=x-ix,v=y-iy;const auto i=std::size_t(iy)*(width+1)+ix;
  return std::lerp(std::lerp(integral[i],integral[i+1],u),std::lerp(integral[i+width+1],integral[i+width+2],u),v);
 };
 return std::max(0.,sum(right,bottom)-sum(left,bottom)-sum(right,top)+sum(left,top))/(width*height);
}
void GlyphAtlas::normalize(){
 std::stable_sort(glyphs.begin(),glyphs.end(),[](const Glyph& a,const Glyph& b){return a.density<b.density;});
 const double maximum=glyphs.empty()?0:glyphs.back().density;for(auto& g:glyphs)g.level=maximum>0?g.density/maximum:0;
}
bool GlyphAtlas::valid()const{
 if(glyphs.size()<2||glyphs.size()>96||glyphs.front().density>1e-9||glyphs.back().density<=1e-9)return false;
 double previous=-1;for(const auto& g:glyphs){if(g.width<1||g.height<1||g.width>512||g.height>512||g.integral.size()!=std::size_t(g.width+1)*(g.height+1)||!std::isfinite(g.density)||!std::isfinite(g.level)||g.density<0||g.density>1||g.level<previous||g.level<0||g.level>1)return false;previous=g.level;}return true;
}
bool valid(const Parameters& p){
 auto range=[](double v,double a,double b){return std::isfinite(v)&&v>=a&&v<=b;};
 return p.charset>=0&&p.charset<4&&p.font>=0&&p.font<3&&p.dither>=0&&p.dither<3&&p.color_mode>=0&&p.color_mode<2&&p.background>=0&&p.background<2&&range(p.cell_width,1e-6,256)&&range(p.cell_aspect,.5,3)&&range(p.fill,20,100)&&range(p.dither_amount,0,100)&&range(p.brightness,-100,100)&&range(p.contrast,0,400)&&range(p.gamma,.1,5)&&range(p.shift_x,-1e7,1e7)&&range(p.shift_y,-1e7,1e7)&&range(p.mix,0,100)&&range(p.color_gain,0,400)&&std::all_of(p.ink.begin(),p.ink.end(),[&](double x){return range(x,0,1);})&&std::all_of(p.paper.begin(),p.paper.end(),[&](double x){return range(x,0,1);});
}
Parameters preset(int i){
 Parameters p;
 if(i==0){p.ink={.2,1,.45};p.paper={.005,.02,.01};}
 if(i==1){p.invert=true;p.ink={.02,.02,.025};p.paper={1,.98,.93};p.font=1;p.contrast=110;p.gamma=1;}
 if(i==2){p.color_mode=1;p.ink={1,1,1};p.paper={0,0,0};p.gamma=1.4;p.fill=95;}
 if(i==3){p.charset=2;p.dither=2;p.cell_width=10;p.ink={.25,.8,1};p.paper={.005,.015,.03};p.bold=true;}
 if(i==4){p.charset=1;p.font=1;p.dither=2;p.cell_width=8;p.cell_aspect=1.5;p.ink={1,.8,.5};p.paper={.025,.012,.005};}
 if(i==5){p.background=1;p.ink={1,1,1};p.dither=1;p.bold=true;}
 return p;
}
bool render(const Image& src,const Parameters& p,const GlyphAtlas& atlas,Image& out,const Cancel& cancel){
 pscpu::ParallelScope pool;
 if(!src.valid()||!valid(p)||!atlas.valid()||stop(cancel))return false;
 if(p.mix==0){out=src;return true;}
 const double cw=p.cell_width,ch=cw*p.cell_aspect;
 const auto gx=std::int64_t(std::floor((src.left-p.shift_x)/cw)),gy=std::int64_t(std::floor((src.top-p.shift_y)/ch));
 const double nx=std::ceil((double(src.left)+src.width-p.shift_x)/cw)-gx,ny=std::ceil((double(src.top)+src.height-p.shift_y)/ch)-gy;
 if(nx<1||ny<1||nx>4194304||ny>4194304||nx*ny>4194304)return false;
 const auto cols=std::size_t(nx),rows=std::size_t(ny);std::vector<Cell> cells(cols*rows);
 if(!pscpu::parallel_ranges(rows,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if(cancelled())return false;for(std::size_t x=0;x<cols;x++){
   const double left=p.shift_x+(gx+std::int64_t(x))*cw-src.left,top=p.shift_y+(gy+std::int64_t(y))*ch-src.top;
   const int x0=std::clamp(int(std::floor(std::max(-1.,left))),0,src.width),x1=std::clamp(int(std::ceil(std::min(double(src.width),left+cw))),0,src.width);
   const int y0=std::clamp(int(std::floor(std::max(-1.,top))),0,src.height),y1=std::clamp(int(std::ceil(std::min(double(src.height),top+ch))),0,src.height);
   double sum[3]{},weight=0;
   for(int yy=y0;yy<y1;yy++)for(int xx=x0;xx<x1;xx++){
    const double area=std::max(0.,std::min(left+cw,xx+1.)-std::max(left,double(xx)))*std::max(0.,std::min(top+ch,yy+1.)-std::max(top,double(yy)));
    const auto n=(std::size_t(yy)*src.width+xx)*4;const double a=unit(finite(src.rgba[n+3]))*area;weight+=a;for(int k=0;k<3;k++)sum[k]+=std::max(0.,finite(src.rgba[n+k]))*a;
   }
   auto& cell=cells[y*cols+x];cell.visible=weight>1e-12;if(!cell.visible)continue;
   for(int k=0;k<3;k++)cell.rgb[k]=float(std::min(double(std::numeric_limits<float>::max()),sum[k]/weight));
   double v=unit(.2126*cell.rgb[0]+.7152*cell.rgb[1]+.0722*cell.rgb[2]);v=std::pow(unit((v-.5)*p.contrast*.01+.5+p.brightness*.01),1/p.gamma);cell.value=p.invert?1-v:v;
  }}return true;
 }))return false;
 const double amount=p.dither_amount*.01;
 if(p.dither==2&&amount>0){
  std::vector<double> errors(cols+2),next(cols+2);
  for(std::size_t y=0;y<rows;y++){
   if(stop(cancel))return false;const bool reverse=((gy+std::int64_t(y))&1)!=0;
   for(std::size_t j=0;j<cols;j++){
    const auto x=reverse?cols-1-j:j;auto& cell=cells[y*cols+x];if(!cell.visible)continue;const auto at=x+1;
    const double v=unit(cell.value+errors[at]);cell.glyph=nearest(atlas,v);const double e=(v-atlas.glyphs[cell.glyph].level)*amount;
    const auto forward=reverse?at-1:at+1,back=reverse?at+1:at-1;errors[forward]+=e*7/16;next[back]+=e*3/16;next[at]+=e*5/16;next[forward]+=e/16;
   }
   errors.swap(next);std::fill(next.begin(),next.end(),0);
  }
 }else{
  constexpr int bayer[16]={0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5};
  for(std::size_t y=0;y<rows;y++){if(stop(cancel))return false;for(std::size_t x=0;x<cols;x++){
   auto& cell=cells[y*cols+x];double v=cell.value;
   cell.glyph=cell.visible?nearest(atlas,v):0;
   if(cell.visible&&p.dither==1&&amount>0){
    const auto at=std::lower_bound(atlas.glyphs.begin(),atlas.glyphs.end(),v,[](const Glyph& g,double value){return g.level<value;});
    if(at!=atlas.glyphs.begin()&&at!=atlas.glyphs.end()){
     const auto hi=std::size_t(at-atlas.glyphs.begin());const double low=atlas.glyphs[hi-1].level,span=at->level-low;
     const auto xx=std::uint64_t(gx+std::int64_t(x))&3u,yy=std::uint64_t(gy+std::int64_t(y))&3u;
     const double threshold=std::lerp(.5,(bayer[yy*4+xx]+.5)/16.,amount);cell.glyph=span>0&&(v-low)/span>=threshold?hi:hi-1;
    }
   }
  }}
 }
 Image result{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};const double fill=p.fill*.01,mix=p.mix*.01,gain=p.color_gain*.01;
 if(!pscpu::parallel_ranges(src.height,std::size_t(src.width)*src.height,cancel,[&](std::size_t lo,std::size_t hi,const auto& cancelled){
  for(auto y=lo;y<hi;y++){if(cancelled())return false;for(int x=0;x<src.width;x++){
   const double dx=double(src.left)+x,dy=src.top+double(y);double coverage=0,colour[3]{};
   auto contribute=[&](std::int64_t ix,std::int64_t iy,double l,double t,double rr,double bb,bool average){
    const auto xx=ix-gx,yy=iy-gy;if(xx<0||yy<0||std::uint64_t(xx)>=cols||std::uint64_t(yy)>=rows)return;
    const auto& cell=cells[std::size_t(yy)*cols+std::size_t(xx)];if(!cell.visible)return;const auto& glyph=atlas.glyphs[cell.glyph];double a;
    if(average)a=glyph.density*fill*fill;
    else{const double left=p.shift_x+ix*cw,top=p.shift_y+iy*ch;const auto u=[&](double px){return ((px-left)/cw-.5)/fill+.5;};const auto v=[&](double py){return ((py-top)/ch-.5)/fill+.5;};a=glyph.area(u(l),v(t),u(rr),v(bb))*cw*ch*fill*fill;}
    coverage+=a;const double maximum=std::max({.001f,cell.rgb[0],cell.rgb[1],cell.rgb[2]});for(int k=0;k<3;k++)colour[k]+=a*gain*(p.color_mode?cell.rgb[k]/maximum:p.ink[k]);
   };
   if(std::min(cw,ch)<1.5)contribute(std::int64_t(std::floor((dx+.5-p.shift_x)/cw)),std::int64_t(std::floor((dy+.5-p.shift_y)/ch)),dx,dy,dx+1,dy+1,true);
   else{
    const auto x0=std::int64_t(std::floor((dx-p.shift_x)/cw)),x1=std::int64_t(std::floor((dx+1-1e-9-p.shift_x)/cw)),y0=std::int64_t(std::floor((dy-p.shift_y)/ch)),y1=std::int64_t(std::floor((dy+1-1e-9-p.shift_y)/ch));
    for(auto iy=y0;iy<=y1;iy++)for(auto ix=x0;ix<=x1;ix++){const double l=std::max(dx,p.shift_x+ix*cw),t=std::max(dy,p.shift_y+iy*ch),rr=std::min(dx+1,p.shift_x+(ix+1)*cw),bb=std::min(dy+1,p.shift_y+(iy+1)*ch);if(rr>l&&bb>t)contribute(ix,iy,l,t,rr,bb,false);}
   }
   coverage=unit(coverage);const auto n=(y*src.width+x)*4;const double a=unit(finite(src.rgba[n+3])),ea=p.background?a*coverage:a,alpha=std::lerp(a,ea,mix);
   for(int k=0;k<3;k++){const double premult=p.background?colour[k]:colour[k]+p.paper[k]*(1-coverage);result.rgba[n+k]=alpha>0?float((finite(src.rgba[n+k])*a*(1-mix)+premult*a*mix)/alpha):0;}
   result.rgba[n+3]=float(alpha);
  }}return true;
 }))return false;
 if(stop(cancel))return false;out=std::move(result);return true;
}
}
