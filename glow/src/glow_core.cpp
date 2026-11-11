#include "glow_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace glow {
namespace {
float unit(float x){return std::clamp(std::isfinite(x)?x:0.f,0.f,1.f);}
float sane(float x){return std::isfinite(x)?std::clamp(x,-65504.f,65504.f):0.f;}
bool stopped(const Cancel& c){return c&&c();}
using Pixel=std::array<float,4>;
Pixel sample(const Image& im,double x,double y,bool edge){
 if(edge){x=std::clamp(x,0.,double(im.width-1));y=std::clamp(y,0.,double(im.height-1));}
 if(x<=-1||y<=-1||x>=im.width||y>=im.height)return {};
 const int ix=int(std::floor(x)),iy=int(std::floor(y));const float fx=float(x-ix),fy=float(y-iy);
 Pixel out{};
 for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx){
  const int xx=ix+dx,yy=iy+dy;if(xx<0||yy<0||xx>=im.width||yy>=im.height)continue;
  const float w=(dx?fx:1-fx)*(dy?fy:1-fy);const auto i=(std::size_t(yy)*im.width+xx)*4;
  for(int k=0;k<4;++k)out[k]+=im.rgba[i+k]*w;
 }
 return out;
}
Pixel mapped(const Image& im,int x,int y,const Image& canvas){
 return sample(im,(x+.5)*im.width/canvas.width-.5,(y+.5)*im.height/canvas.height-.5,true);
}
// Fractional running box, normalized over its full support. O(pixels), including
// very large radii; constant input remains constant with repeat-edge padding.
bool box(Image& im,double radius,bool vertical,bool edge,const Cancel& cancel){
 if(radius<1e-6)return true;
 const int length=vertical?im.height:im.width,lines=vertical?im.width:im.height;
 const int r=int(std::floor(radius));const double fraction=radius-r,divisor=2*radius+1;
 std::vector<float> output(im.rgba.size());std::vector<double> prefix(std::size_t(length+1)*4);
 for(int line=0;line<lines;++line){
  if(stopped(cancel))return false;
  auto index=[&](int p){return (vertical?std::size_t(p)*im.width+line:std::size_t(line)*im.width+p)*4;};
  for(int k=0;k<4;++k)prefix[k]=0;
  for(int p=0;p<length;++p)for(int k=0;k<4;++k)prefix[std::size_t(p+1)*4+k]=prefix[std::size_t(p)*4+k]+im.rgba[index(p)+k];
  for(int p=0;p<length;++p){
   const int lo=p-r,hi=p+r;
   for(int k=0;k<4;++k){
    auto get=[&](int n){return n<0?(edge?im.rgba[index(0)+k]:0.f):n>=length?(edge?im.rgba[index(length-1)+k]:0.f):im.rgba[index(n)+k];};
    double sum=prefix[std::size_t(std::min(length,hi+1))*4+k]-prefix[std::size_t(std::max(0,lo))*4+k];
    if(edge){sum+=std::max(0,-lo)*get(0)+std::max(0,hi-length+1)*get(length-1);}
    sum+=fraction*(get(lo-1)+get(hi+1));output[index(p)+k]=float(sum/divisor);
   }
  }
 }
 im.rgba.swap(output);return true;
}
bool blur(Image& im,double sx,double sy,bool edge,const Cancel& c){
 const double rx=(std::sqrt(4*sx*sx+1)-1)*.5,ry=(std::sqrt(4*sy*sy+1)-1)*.5;
 for(int pass=0;pass<3;++pass)if(!box(im,rx,false,edge,c)||!box(im,ry,true,edge,c))return false;
 return true;
}
Image reduce(const Image& src,int factor,const Cancel& c){
 Image out{(src.width+factor-1)/factor,(src.height+factor-1)/factor,0,0,{}};
 out.rgba.resize(std::size_t(out.width)*out.height*4);
 const double dx=double(src.width)/out.width,dy=double(src.height)/out.height;
 for(int y=0;y<out.height;++y){if(stopped(c))return {};
  for(int x=0;x<out.width;++x){std::array<double,4> sum{};
   const double left=x*dx,right=(x+1)*dx,top=y*dy,bottom=(y+1)*dy;
   for(int yy=int(top);yy<std::min(src.height,int(std::ceil(bottom)));++yy)for(int xx=int(left);xx<std::min(src.width,int(std::ceil(right)));++xx){
    const double weight=(std::min(right,double(xx+1))-std::max(left,double(xx)))*(std::min(bottom,double(yy+1))-std::max(top,double(yy)));
    const auto i=(std::size_t(yy)*src.width+xx)*4;for(int k=0;k<4;++k)sum[k]+=src.rgba[i+k]*weight;
   }
   const auto i=(std::size_t(y)*out.width+x)*4;for(int k=0;k<4;++k)out.rgba[i+k]=float(sum[k]/(dx*dy));
  }
 }return out;
}
// Rotate a low-resolution working buffer, then blur on its principal axes.
// Rotation is only required for an anisotropic glow and never rotates the source.
Image oriented(const Image& src,double sx,double sy,double degrees,bool edge,const Cancel& c){
 if(std::abs(sx-sy)<1e-6||std::abs(std::remainder(degrees,180.))<1e-6){Image out=src;if(!blur(out,sx,sy,edge,c))return {};return out;}
 const double a=degrees*3.14159265358979323846/180,cs=std::cos(a),sn=std::sin(a);
 const int w=int(std::ceil(std::abs(src.width*cs)+std::abs(src.height*sn)))+2;
 const int h=int(std::ceil(std::abs(src.width*sn)+std::abs(src.height*cs)))+2;
 Image rot{w,h,0,0,{}};rot.rgba.resize(std::size_t(w)*h*4);
 for(int y=0;y<h;++y){if(stopped(c))return {};for(int x=0;x<w;++x){
  const double dx=x-(w-1)*.5,dy=y-(h-1)*.5;
  const auto v=sample(src,cs*dx-sn*dy+(src.width-1)*.5,sn*dx+cs*dy+(src.height-1)*.5,edge);
  const auto i=(std::size_t(y)*w+x)*4;for(int k=0;k<4;++k)rot.rgba[i+k]=v[k];
 }}
 if(!blur(rot,sx,sy,edge,c))return {};
 Image out=src;
 for(int y=0;y<src.height;++y){if(stopped(c))return {};for(int x=0;x<src.width;++x){
  const double dx=x-(src.width-1)*.5,dy=y-(src.height-1)*.5;
  const auto v=sample(rot,cs*dx+sn*dy+(w-1)*.5,-sn*dx+cs*dy+(h-1)*.5,edge);
  const auto i=(std::size_t(y)*src.width+x)*4;for(int k=0;k<4;++k)out.rgba[i+k]=v[k];
 }}return out;
}
double cycle(double x,int mode,double midpoint){
 x-=std::floor(x);
 if(mode==0)x=1-std::abs(2*x-1);else if(mode==2)x=1-x;
 const double m=std::clamp(midpoint*.01,.001,.999);
 return x<m?.5*x/m:.5+.5*(x-m)/(1-m);
}
}
bool Image::valid() const {return width>0&&height>0&&std::size_t(width)*height<=std::size_t(16)*1024*1024&&rgba.size()==std::size_t(width)*height*4;}
Parameters defaults(Kind kind){Parameters p;p.kind=kind;if(kind==Kind::deep){p.threshold=0;p.radius=100;p.blend=Blend::screen;p.color_a={1,.20,.28};p.color_b={1,.55,.20};}return p;}
bool valid(const Parameters& p){
 if(p.kind!=Kind::classic&&p.kind!=Kind::deep)return false;
 const double values[]={p.threshold,p.radius,p.intensity,p.loops,p.phase,p.midpoint,p.smoothness,p.saturation_bias,p.gamma,p.gamma_amount,p.aspect,p.angle,p.chromatic_shift,p.chromatic_spread,p.tint_amount,p.source_opacity,p.texture_opacity,p.texture_glow,p.texture_exposure,p.texture_gamma};
 for(double x:values)if(!std::isfinite(x))return false;
 for(auto a:{p.color_a,p.color_b})for(double x:a)if(!std::isfinite(x)||x<0||x>1)return false;
 return p.threshold>=0&&p.threshold<=1000&&p.radius>=0&&p.radius<=4096&&p.intensity>=0&&p.intensity<=100&&p.loops>=0&&p.loops<=100&&std::abs(p.phase)<=360000&&p.midpoint>=0&&p.midpoint<=100&&p.smoothness>=0&&p.smoothness<=100&&p.saturation_bias>=0&&p.saturation_bias<=100&&p.gamma>=.1&&p.gamma<=5&&p.gamma_amount>=0&&p.gamma_amount<=100&&p.aspect>=.05&&p.aspect<=20&&std::abs(p.angle)<=360000&&std::abs(p.chromatic_shift)<=100&&p.chromatic_spread>=0&&p.chromatic_spread<=100&&p.tint_amount>=0&&p.tint_amount<=100&&p.source_opacity>=0&&p.source_opacity<=100&&p.texture_opacity>=0&&p.texture_opacity<=100&&p.texture_glow>=0&&p.texture_glow<=100&&std::abs(p.texture_exposure)<=10&&p.texture_gamma>=.1&&p.texture_gamma<=5&&int(p.blend)>=0&&int(p.blend)<=4&&p.based_on>=0&&p.based_on<=1&&p.composite>=0&&p.composite<=2&&p.colors>=0&&p.colors<=1&&p.looping>=0&&p.looping<=2&&p.dimensions>=0&&p.dimensions<=2&&p.falloff>=0&&p.falloff<=1&&p.threshold_mode>=0&&p.threshold_mode<=2&&p.tint>=0&&p.tint<=2&&p.tone>=0&&p.tone<=2&&p.quality>=0&&p.quality<=2&&p.view>=0&&p.view<=2&&p.mask_mode>=0&&p.mask_mode<=1;
}
bool render(const Image& src,const Parameters& p,Image& output,const Image* mask,const Image* texture,const Cancel& cancel){
 if(!src.valid()||!valid(p)||(mask&&!mask->valid())||(texture&&!texture->valid())||stopped(cancel))return false;
 const bool deep=p.kind==Kind::deep;const double gamma=deep?1+(p.gamma-1)*p.gamma_amount*.01:1;
#ifdef ZAYU_GLOW_FULL_WORK_REFERENCE
 const bool bounded_work=false; // Regression reference for sparse-canvas optimization.
#else
 const bool bounded_work=deep;
#endif
 auto linear=[&](double v){return std::pow(std::max(0.,v),gamma);};
 auto encode=[&](double v){return std::pow(std::max(0.,v),1/gamma);};
 Image seed{src.width,src.height,src.left,src.top,{}};seed.rgba.resize(src.rgba.size());
 int seed_left=src.width,seed_top=src.height,seed_right=0,seed_bottom=0;
 for(int y=0;y<src.height;++y){if(stopped(cancel))return false;for(int x=0;x<src.width;++x){
  const auto i=(std::size_t(y)*src.width+x)*4;const float a=unit(src.rgba[i+3]);
  const double r=std::max(0.f,sane(src.rgba[i])),g=std::max(0.f,sane(src.rgba[i+1])),b=std::max(0.f,sane(src.rgba[i+2]));
  const double maximum=std::max({r,g,b}),minimum=std::min({r,g,b}),luma=.2126*r+.7152*g+.0722*b;
  const double brightness=p.based_on==1?a:deep?(p.threshold_mode==1?maximum:p.threshold_mode==2?a:luma):maximum;
  const double threshold=p.threshold*.01,knee=deep?p.smoothness*.01:0;
  double gate=brightness>=threshold?1:0;
  if(knee>0){const auto t=std::clamp((brightness-threshold+knee)/(2*knee),0.,1.);gate=t*t*(3-2*t);}
  if(brightness<=0)gate=0;
  if(deep&&p.saturation_bias>0)gate*=std::pow(maximum>0?(maximum-minimum)/maximum:0,p.saturation_bias*.02);
  if(mask){const auto m=mapped(*mask,x,y,src);double v=p.mask_mode==0?unit(m[3]):unit(.2126f*m[0]+.7152f*m[1]+.0722f*m[2])*unit(m[3]);gate*=p.invert_mask?1-v:v;}
  for(int k=0;k<3;++k)seed.rgba[i+k]=float((p.based_on==1?1:linear(sane(src.rgba[i+k])))*a*gate);
  seed.rgba[i+3]=float(a*gate);
  if(a*gate>0){seed_left=std::min(seed_left,x);seed_top=std::min(seed_top,y);seed_right=std::max(seed_right,x+1);seed_bottom=std::max(seed_bottom,y+1);}
 }}
 Image glow{src.width,src.height,src.left,src.top,{}};glow.rgba.resize(src.rgba.size());
 // Deep bloom is additive across a logarithmic range of spatial scales. Keep
 // a subpixel near-field at large radius instead of stretching every scale:
 // a 1000px radius must retain the bright halo next to a thin stroke.
 // These are independent visual weights, not normalized averaging weights.
 constexpr double exponential[]={.55,.60,.65,.70,.75,.70,.60,.48,.32,.16};
 constexpr double inverse[]={.40,.45,.50,.55,.60,.65,.70,.75,.70,.60};
 const int count=deep?(p.intensity==0||p.view==2||seed_right<=seed_left?0:10):1;
 const double widest=p.radius*.5;
 const double narrowest=std::min(widest,.6*(1-std::exp(-p.radius/3)));
 double total_weight=0;
 for(int level=0;level<count;++level){
  if(stopped(cancel))return false;
  const double sigma=deep?(narrowest>0?narrowest*std::pow(widest/narrowest,level/9.):0):p.radius/3;
  const double stretch=deep?std::sqrt(p.aspect):1;
  double sx=sigma*stretch,sy=sigma/stretch;
  if(!deep&&p.dimensions==1)sy=0;if(!deep&&p.dimensions==2)sx=0;
  int factor=1;const double target=deep?(p.quality==0?3:p.quality==1?6:12):12;
  // Preserve the narrow axis when anisotropy is high.
  const double minor=std::min(sx,sy)>0?std::min(sx,sy):std::max(sx,sy);
  while(minor/factor>target&&factor<64)factor*=2;
  int origin_x=0,origin_y=0,span_width=src.width,span_height=src.height;
  int left=0,top=0,right=src.width,bottom=src.height;
  if(bounded_work){
   // Conservative finite-kernel footprint, including resampling/rotation. Thin
   // text should not spend every near-field pass on a mostly empty canvas.
   const int padding=int(std::ceil(6*std::max(sx,sy)+2*factor+4));
   left=std::max(0,seed_left-padding);top=std::max(0,seed_top-padding);
   right=std::min(src.width,seed_right+padding);bottom=std::min(src.height,seed_bottom+padding);
  }
  Image work;
  if(bounded_work&&factor==1&&std::abs(sx-sy)<1e-6){
   origin_x=left;origin_y=top;span_width=right-left;span_height=bottom-top;
   work={span_width,span_height,0,0,{}};work.rgba.resize(std::size_t(span_width)*span_height*4);
   for(int y=0;y<span_height;++y){if(stopped(cancel))return false;
    std::copy_n(seed.rgba.data()+(std::size_t(y+top)*src.width+left)*4,std::size_t(span_width)*4,work.rgba.data()+std::size_t(y)*span_width*4);}
  }else work=factor==1?seed:reduce(seed,factor,cancel);
  if(!work.valid())return false;
  work=oriented(work,sx*work.width/span_width,sy*work.height/span_height,deep?p.angle:0,p.repeat_edges,cancel);if(!work.valid())return false;
  const double weight=deep?(p.falloff==0?exponential[level]:inverse[level]):1;
  total_weight+=weight;
  const auto accumulation_units=std::size_t(right-left)*(bottom-top)*4;
  if(!pscpu::parallel_ranges(bottom-top,deep&&accumulation_units>=1048576?accumulation_units:0,cancel,
     [&](std::size_t first,std::size_t last,const auto& check){
  for(int y=top+int(first);y<top+int(last);++y){if(check())return false;for(int x=left;x<right;++x){
   const auto v=sample(work,(x+.5-origin_x)*work.width/span_width-.5,(y+.5-origin_y)*work.height/span_height-.5,p.repeat_edges);const auto i=(std::size_t(y)*src.width+x)*4;
   for(int k=0;k<4;++k)glow.rgba[i+k]+=float(v[k]*weight);
  }}return true;}))return false;
 }
 Image out{src.width,src.height,src.left,src.top,{}};out.rgba.resize(src.rgba.size());
 // Small previews retain the original serial path: starting threads costs more
 // than this compositing pass below a quarter-million pixels on this host.
 if(!pscpu::parallel_ranges(src.height,src.rgba.size()<1048576?0:src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& check){
 for(int y=int(first);y<int(last);++y){if(check())return false;for(int x=0;x<src.width;++x){
  const auto i=(std::size_t(y)*src.width+x)*4;double g[3];
  const double coverage=total_weight>0?unit(float(glow.rgba[i+3]/total_weight)):0;
  for(int k=0;k<3;++k){
   double v=glow.rgba[i+k];
   if(deep&&(p.chromatic_shift!=0||p.chromatic_spread!=0)){
    const double sign=k==0?-1:k==2?1:0,scale=1+sign*p.chromatic_spread*.002;
    const auto q=sample(glow,(x-(src.width-1)*.5)/scale+(src.width-1)*.5+sign*p.chromatic_shift,
                        (y-(src.height-1)*.5)/scale+(src.height-1)*.5,p.repeat_edges);v=q[k];
   }
   g[k]=std::max(0.,v*p.intensity);
  }
  if((!deep&&p.colors==1)||(deep&&p.tint!=0)){
   const double strength=std::max({g[0],g[1],g[2]});
   const double mix=deep?(p.tint==1?0:1-coverage):cycle(coverage*p.loops+p.phase/360,p.looping,p.midpoint);
   for(int k=0;k<3;++k){const double target=linear(p.color_a[k]*(1-mix)+p.color_b[k]*mix)*strength;
    g[k]=deep?g[k]+(target-g[k])*p.tint_amount*.01:target;}
  }
  if(deep&&texture){const auto t=mapped(*texture,x,y,src);const double a=p.texture_opacity*.01*unit(t[3]);
   for(int k=0;k<3;++k){const double v=std::pow(std::max(0.f,sane(t[k])),1/p.texture_gamma)*std::exp2(p.texture_exposure);
    g[k]*=(1-a)+a*(v+(1-v)*coverage*p.texture_glow*.01);}
  }
  for(double& v:g){if(deep&&p.tone==1)v=v/(1+v);else if(deep&&p.tone==2)v=1-std::exp(-v);}
  if(deep){
   // The Photoshop layer stack consumes encoded RGB and coverage alpha.
   // Encode emitted (premultiplied) light BEFORE deriving that coverage.
   // Otherwise a white glow of linear energy .01 becomes RGB=1, alpha=.01,
   // losing gamma correction when composited, instead of encode(.01)=.123.
   const bool source_view=p.view==2,only_glow=p.view==1;
   if(source_view){
    const double alpha=unit(seed.rgba[i+3]);
    for(int k=0;k<3;++k){const double straight=alpha>1e-12?encode(seed.rgba[i+k]/alpha):0;out.rgba[i+k]=sane(float(p.unmult?straight:straight*alpha));}
    out.rgba[i+3]=float(alpha);continue;
   }
   double emission[3];for(int k=0;k<3;++k)emission[k]=encode(g[k]);
   const double ga=std::clamp(std::max({emission[0],emission[1],emission[2]}),0.,1.);
   const double sa=only_glow?0:unit(src.rgba[i+3])*p.source_opacity*.01;
   const double alpha=sa+ga-sa*ga;
   for(int k=0;k<3;++k){
    const double original=std::max(0.f,sane(src.rgba[i+k])),source=original*sa,e=emission[k];double value;
    if(only_glow)value=e;
    else if(p.blend==Blend::add)value=source+e;
    else if(p.blend==Blend::screen)value=source+e-std::min(1.,source)*std::min(1.,e);
    else if(p.blend==Blend::normal)value=e+source*(1-ga);
    else{const double glow_color=ga>0?e/ga:0,mixed=p.blend==Blend::lighten?std::max(original,glow_color):original*glow_color;
     value=source*(1-ga)+e*(1-sa)+mixed*sa*ga;}
    out.rgba[i+k]=sane(float(p.unmult?(alpha>1e-12?value/alpha:0):value));
   }
   out.rgba[i+3]=float(alpha);continue;
  }
  const double ga=std::clamp(std::max({g[0],g[1],g[2]}),0.,1.);
  double sa=unit(src.rgba[i+3])*(deep?p.source_opacity*.01:1);
  const bool only_glow=deep?p.view==1:p.composite==2;if(only_glow)sa=0;
  const bool source_view=deep&&p.view==2;
  double alpha=source_view?unit(seed.rgba[i+3]):sa+ga-sa*ga;
  if(!deep&&p.intensity==0&&p.composite!=2)alpha=unit(src.rgba[i+3]);
  for(int k=0;k<3;++k){
   const double source=linear(sane(src.rgba[i+k]))*sa;double v=0;
   if(source_view)v=seed.rgba[i+k];
   else if(only_glow)v=g[k];
   else if(p.blend==Blend::add)v=source+g[k];
   else if(p.blend==Blend::screen)v=source+g[k]-std::min(1.,source)*std::min(1.,g[k]);
   else{
    const double s=sa>0?source/sa:0,b=ga>0?g[k]/ga:0;
    if(p.blend==Blend::normal)v=(!deep&&p.composite==1)?source+g[k]*(1-sa):g[k]+source*(1-ga);
    else {const double mixed=p.blend==Blend::lighten?std::max(s,b):s*b;v=source*(1-ga)+g[k]*(1-sa)+mixed*sa*ga;}
   }
   // Alpha is computed in the encoded output domain; RGB remains unassociated.
   const double straight=(!deep||p.unmult)?(alpha>1e-12?v/alpha:0):v;
   out.rgba[i+k]=sane(float(encode(std::max(0.,straight))));
  }
  out.rgba[i+3]=float(alpha);
 }}
 return true;}))return false;
 if(stopped(cancel))return false;output=std::move(out);return true;
}
}
