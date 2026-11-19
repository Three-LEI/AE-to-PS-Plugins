#include "longshadow_core.hpp"
#include "../../native/common/parallel_ranges.hpp"
#include <algorithm>
#include <cmath>
#include <deque>
#include <numbers>
#include "soft_shadow.hpp"

namespace longshadow {
namespace {
double ramp(double a){a=std::clamp(a,0.,1.);return a*a*(3-2*a);}
float alpha(float a){return std::isfinite(a)?std::clamp(a,0.f,1.f):0.f;}
double mask_at(const Parameters& p,double x,double y){
 if(!p.mask)return 1;
 if(p.mask_width==0||p.mask_height==0)return p.invert_mask?1:0;
 const double dx=2*(x-p.mask_x)/p.mask_width,dy=2*(y-p.mask_y)/p.mask_height;
 const double r=p.mask==1?std::max(std::abs(dx),std::abs(dy)):std::hypot(dx,dy);
 const double m=p.mask_feather>0?ramp((1-r)/(p.mask_feather*.01)):double(r<=1);
 return p.invert_mask?1-m:m;
}
struct Sample {int pos;float value;};
}
bool valid(const Parameters& p){
 auto in=[](double x,double lo,double hi){return std::isfinite(x)&&x>=lo&&x<=hi;};
 return in(p.angle,-360,360)&&in(p.length,0,10000)&&in(p.opacity,0,100)&&in(p.expansion,0,100)&&in(p.fade,0,100)&&
        std::all_of(p.color.begin(),p.color.end(),[&](double x){return in(x,0,1);})&&
        in(p.crop_left,0,100)&&in(p.crop_right,0,100)&&p.mask>=0&&p.mask<=2&&
        in(p.mask_x,-100,200)&&in(p.mask_y,-100,200)&&in(p.mask_width,0,200)&&in(p.mask_height,0,200)&&in(p.mask_feather,0,100)&&
        p.color_mode>=0&&p.color_mode<=1&&in(p.source_brightness,0,200)&&in(p.gap,0,2000)&&
        in(p.blur_near,0,160)&&in(p.blur_far,0,160);
}
Parameters preset(int i){
 Parameters p;
 switch(i){
  case 1:p.angle=90;p.length=220;p.opacity=66;p.fade=72;p.color={.05,.08,.15};break;
  case 2:p.angle=32;p.length=55;p.opacity=100;p.color={.18,.27,.66};break;
  case 3:p.angle=135;p.length=150;p.opacity=86;p.fade=20;p.color={.56,.17,.16};break;
  case 4:p.angle=45;p.length=85;p.opacity=68;p.gap=12;p.blur_near=1.5;p.blur_far=18;p.fade=38;break;
 }
 return p;
}
bool render(const Image& src,const Parameters& p,Image& out,const Cancel& cancel,double step){
 if(!src.valid()||!valid(p)||!std::isfinite(step)||step<1||(cancel&&cancel()))return false;
 const auto cells=std::size_t(src.width)*src.height;
 if(p.length==0||p.opacity==0){
  if(p.shadow_only)out={src.width,src.height,src.left,src.top,std::vector<float>(cells*4)};
  else if(&src!=&out)out=src;
  return true;
 }

 std::vector<float> caster(cells);
 int left=0,right=src.width;
 if(p.crop){
  left=src.width;right=0;
  for(int y=0;y<src.height;y++){
   if(cancel&&cancel())return false;
   for(int x=0;x<src.width;x++)if(alpha(src.rgba[(std::size_t(y)*src.width+x)*4+3])>0){left=std::min(left,x);right=std::max(right,x+1);}
  }
 }
 const double start=left+(right-left)*p.crop_left*.01,end=left+(right-left)*p.crop_right*.01;
 if(!pscpu::parallel_ranges(src.height,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
  for(std::size_t y=a;y<b;y++){
   if(stop())return false;
   for(int x=0;x<src.width;x++){
    const auto i=y*src.width+x;
    const double gate=p.crop?std::clamp(std::min(end,x+1.)-std::max(start,double(x)),0.,1.):1.;
    caster[i]=float(gate*alpha(src.rgba[i*4+3]));
   }
  }return true;
 }))return false;

 // Widen the caster with a separable max filter; interpolated radii keep small
 // expansion changes continuous, including on reduced previews.
 const bool stretch=p.color_mode==1;
 const bool soft=p.blur_near>0||p.blur_far>0,progressive=p.blur_near!=p.blur_far;
 const double expansion=p.expansion/step;
 std::vector<std::uint32_t> owner;
 if(expansion>0){
  std::vector<float> intermediate(cells);
  std::vector<std::uint32_t> intermediate_owner(stretch?cells:0);
  if(stretch)owner.resize(cells);
  auto expand=[&](bool vertical,const std::vector<float>& input,std::vector<float>& output,
                  const std::vector<std::uint32_t>* origin,std::vector<std::uint32_t>& expanded){
   const int n=vertical?src.height:src.width,lines=vertical?src.width:src.height;
   const int radius=int(expansion);const float fraction=float(expansion-radius);
   return pscpu::parallel_ranges(lines,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
    std::deque<Sample> q;
    for(auto line=a;line<b;line++){
     if(stop())return false;
     q.clear();int next=0;
     const auto index=[&](int j){return vertical?std::size_t(j)*src.width+line:line*src.width+j;};
     for(int j=0;j<n;j++){
      if((j&4095)==0&&stop())return false;
      while(!q.empty()&&q.front().pos<j-radius)q.pop_front();
      for(;next<n&&next<=j+radius;next++){
       const float v=input[index(next)];
       while(!q.empty()&&q.back().value<=v)q.pop_back();
       q.push_back({next,v});
      }
      const float max=q.front().value;
      const int lo=j-radius-1,hi=j+radius+1;
      const float a=lo>=0?input[index(lo)]:0.f,b=hi<n?input[index(hi)]:0.f;
      const float neighbour=std::max(a,b);
      const auto dest=index(j);
      output[dest]=std::lerp(max,std::max(max,neighbour),fraction);
      if(stretch&&output[dest]>0){
       // Retain the original color in the undilated interior. At a softened
       // expansion edge use the source of the dominant alpha contribution.
       const auto from=index(neighbour>max&&fraction*neighbour>(1-fraction)*max?(a>=b?lo:hi):
                             input[dest]==max?j:q.front().pos);
       expanded[dest]=origin?(*origin)[from]:std::uint32_t(from);
      }
     }
    }return true;
   });
  };
  if(!expand(false,caster,intermediate,nullptr,intermediate_owner)||!expand(true,intermediate,caster,&intermediate_owner,owner))return false;
 }

 const double angle=p.angle*std::numbers::pi/180,dx=std::cos(angle),dy=std::sin(angle);
 const bool vertical=std::abs(dy)>std::abs(dx);
 const double direction=vertical?dy:dx,dominant=std::abs(direction);
 const int major=vertical?src.height:src.width,minor=vertical?src.width:src.height;
 double slope=(vertical?dx:dy)/dominant;
 if(std::abs(slope)<1e-12)slope=0;
 if(std::abs(std::abs(slope)-1)<1e-12)slope=std::copysign(1.,slope);
 const double reach=p.length*dominant/step;
 const double offset=p.gap*dominant/step;
 const int whole=int(reach);const float fraction=float(reach-whole);
 std::vector<float> weight(std::min(major,whole+2));
 weight[0]=1;
 for(std::size_t i=1;i<weight.size();i++)weight[i]=float(std::exp2(-.06*p.fade*std::min(1.,double(i)/reach)));
 const int stride=minor+1;
 std::vector<float> swept(std::size_t(major)*stride);
 // Color mode stores only the winning position for each ray sample, so
 // changing shadow length still costs a single scan and RGB is read once.
 std::vector<int> contributor(stretch||progressive?swept.size():0);
 std::vector<int> shift(major);
 std::vector<float> phase(major);
 for(int m=0;m<major;m++){const double s=slope*m;shift[m]=int(std::floor(s));phase[m]=float(s-shift[m]);}
 const auto index=[&](int m,int n){
  const int u=direction<0?major-1-m:m;
  return vertical?std::size_t(u)*src.width+n:std::size_t(n)*src.width+u;
 };
 const auto at=[&](int m,int n){return n>=0&&n<minor?caster[index(m,n)]:0.f;};
 const auto tap=[&](int m,int ray){
  const int n=ray+shift[m];
  return std::lerp(at(m,n),at(m,n+1),phase[m]);
 };
 const int min_ray=-1-std::max(0,shift.back()),max_ray=minor-1-std::min(0,shift.back());
 // Each ray uses a sliding maximum. Exponential tail weights are separable
 // by sample age, so a stronger newer sample can discard an older one.
 if(!pscpu::parallel_ranges(max_ray-min_ray+1,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
  std::deque<Sample> queue;
  for(auto i=a;i<b;i++){
   if(stop())return false;
   const int ray=min_ray+int(i);int begin=0,end=major;
   if(slope!=0){
    const double s=(-1.-ray)/slope,t=(minor-double(ray))/slope;
    begin=int(std::clamp(std::floor(std::min(s,t))-1,0.,double(major)));
    end=int(std::clamp(std::ceil(std::max(s,t))+1,0.,double(major)));
   }
   queue.clear();
   for(int m=begin;m<end;m++){
    if((m&4095)==0&&stop())return false;
    const int n=ray+shift[m];
    if(n< -1||n>=minor)continue;
    while(!queue.empty()&&m-queue.front().pos>whole)queue.pop_front();
    const float value=tap(m,ray);
    if(value>0){
     while(!queue.empty()&&queue.back().value*weight[m-queue.back().pos]<=value)queue.pop_back();
     queue.push_back({m,value});
    }
    int from=queue.empty()?m:queue.front().pos;
    float coverage=queue.empty()?0.f:queue.front().value*weight[m-from];
    if(fraction>0&&m>whole){
     const float last=tap(m-whole-1,ray)*weight[whole+1]*fraction;
     if(last>coverage){coverage=last;from=m-whole-1;}
    }
    const auto dest=std::size_t(m)*stride+n+1;
    swept[dest]=coverage;
    if(!contributor.empty())contributor[dest]=from;
   }
  }return true;
 }))return false;

 // Reproject along the same ray, delaying its start by the requested gap.
 // Interpolate alpha and associated color so clear source pixels cannot tint
 // the edge and adjacent rays meet without introducing dark fringes.
 const auto project=[&](int m,int n,std::array<double,3>& color,float& age){
  const int ray=n-shift[m];
  const double z=m-offset;
  if(z<=-1)return 0.f;
  const int a=int(std::floor(z));
  const float t=float(z-a),u=phase[m];
  std::array<double,3> rgb{};double sum=0,distance=0;
  auto read=[&](int column,int line,float blend){
   if(column<0||column>=major)return 0.f;
   const int y=line+shift[column];
   if(y< -1||y>=minor)return 0.f;
   const auto index_in_sweep=std::size_t(column)*stride+y+1;
   const float v=swept[index_in_sweep];
   if(!contributor.empty()&&v>0&&blend>0){
    const int source_m=contributor[index_in_sweep],source_n=line+shift[source_m];
    const double scale=double(v)*blend;
    sum+=scale;
    if(progressive&&reach>0)distance+=scale*std::min(1.,(column-source_m)/reach);
    if(stretch){
     const double a0=at(source_m,source_n)*(1.-phase[source_m]),a1=at(source_m,source_n+1)*phase[source_m];
     auto add=[&](int source_row,double w){
      if(w<=0)return;
      const auto pixel=index(source_m,source_row);
      const auto sample=(owner.empty()?pixel:owner[pixel])*4;
      for(int k=0;k<3;k++)if(std::isfinite(src.rgba[sample+k]))rgb[k]+=w*src.rgba[sample+k];
     };
     if(a0+a1>0){
      const double factor=scale/(a0+a1);
      add(source_n,a0*factor);add(source_n+1,a1*factor);
     }
    }
   }
   return v;
  };
  auto cross=[&](int column,float blend){
   const float same=read(column,ray,blend*(1-u));
   const float previous=read(column,ray-1,blend*u);
   return std::lerp(same,previous,u);
  };
  const float first=cross(a,1-t);
  const float coverage=t>0?std::lerp(first,cross(a+1,t),t):first;
  if(sum>0){
   if(stretch){const double scale=p.source_brightness*.01/sum;for(int k=0;k<3;k++)color[k]=rgb[k]*scale;}
   if(progressive)age=float(std::clamp(distance/sum,0.,1.));
  }
  return coverage;
 };
 const int channels=stretch?4:1;
 std::vector<float> hard(soft?cells*channels:0),age(progressive?cells:0);
 Image next;
 if(!soft)next=p.shadow_only?Image{src.width,src.height,src.left,src.top,std::vector<float>(cells*4)}:src;
 const auto composite=[&](int x,int y,std::size_t i,double coverage,const std::array<double,3>& color){
  const double cast=coverage*p.opacity*.01*mask_at(p,(x+.5)*100/src.width,(y+.5)*100/src.height);
  if(cast<=0)return;
  if(p.shadow_only){
   for(int k=0;k<3;k++)next.rgba[i+k]=float(color[k]);
   next.rgba[i+3]=float(cast);
  }else{
   const double source=alpha(src.rgba[i+3]),visible=cast*(1-source);
   if(visible==0)return;
   const double total=source+visible;
   for(int k=0;k<3;k++)next.rgba[i+k]=float((source*src.rgba[i+k]+visible*color[k])/total);
   next.rgba[i+3]=float(total);
  }
 };
 if(!pscpu::parallel_ranges(src.height,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
  for(auto y=a;y<b;y++){
   if(stop())return false;
   for(int x=0;x<src.width;x++){
    const int u=vertical?int(y):x,n=vertical?x:int(y),m=direction<0?major-1-u:u;
    const auto w=std::size_t(m)*stride+n,i=(y*src.width+x)*4;
    auto color=p.color;
    float travel=0;
    const float coverage=stretch||offset>0||progressive?project(m,n,color,travel):std::lerp(swept[w+1],swept[w],phase[m]);
    if(!soft){composite(x,int(y),i,coverage,color);continue;}
    const auto k=(i/4)*channels;
    if(stretch)for(int c=0;c<3;c++)hard[k+c]=float(color[c]*coverage);
    hard[k+channels-1]=coverage;
    if(progressive)age[i/4]=travel;
   }
  }return true;
 }))return false;
 if(soft){
  std::vector<float>().swap(caster);std::vector<float>().swap(swept);
  std::vector<int>().swap(contributor);std::vector<std::uint32_t>().swap(owner);
  std::vector<float> blurred;
  if(!detail::soften(src.width,src.height,channels,hard,age,p.blur_near/step,p.blur_far/step,step>1,cancel,blurred))return false;
  std::vector<float>().swap(hard);std::vector<float>().swap(age);
  next=p.shadow_only?Image{src.width,src.height,src.left,src.top,std::vector<float>(cells*4)}:src;
  if(!pscpu::parallel_ranges(src.height,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
   for(auto y=a;y<b;y++){
    if(stop())return false;
    for(int x=0;x<src.width;x++){
     const auto i=y*src.width+x,k=i*channels;const float raw=blurred[k+channels-1];
     if(raw<1e-7f)continue;
     auto color=p.color;
     if(stretch)for(int c=0;c<3;c++)color[c]=blurred[k+c]/raw;
     composite(x,int(y),i*4,std::min(raw,1.f),color);
    }
   }return true;
  }))return false;
 }
 out=std::move(next);return true;
}
}
