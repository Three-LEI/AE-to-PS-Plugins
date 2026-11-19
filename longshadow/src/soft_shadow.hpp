#pragma once
#include "../../native/common/parallel_ranges.hpp"
#include <cmath>

namespace longshadow::detail {
// Separable tent with transparent padding, using zeroth/first moments to
// integrate a triangle of any radius in constant work per sample.
template<class Read,class Write>
bool tent_pass(int width,int height,bool vertical,double radius,const Cancel& cancel,Read read,Write write){
 const int extent=vertical?height:width,lines=vertical?width:height;
 const double span=radius+1;
 const int support=int(std::floor(span));
 const double norm=1+2*(support-support*(support+1.)/(2*span));
 return pscpu::parallel_ranges(lines,std::size_t(width)*height,cancel,[&](std::size_t begin,std::size_t end,const auto& stop){
  std::vector<double> sum(extent+1),moment(extent+1);
  for(auto line=begin;line<end;line++){
   if(stop())return false;
   auto index=[&](int j){return vertical?std::size_t(j)*width+line:line*width+j;};
   for(int j=0;j<extent;j++){
    const double v=read(index(j));sum[j+1]=sum[j]+v;moment[j+1]=moment[j]+j*v;
   }
   for(int j=0;j<extent;j++){
    const int lo=std::max(0,j-support),mid=j+1,hi=std::min(extent,j+support+1);
    const double a=sum[mid]-sum[lo],b=sum[hi]-sum[mid];
    const double s=a+b+(moment[mid]-moment[lo]-j*a+j*b-moment[hi]+moment[mid])/span;
    write(index(j),s/norm);
   }
  }return true;
 });
}

// The distance field remains meaningful just outside a hard edge: use the
// age of the closest covered sample there. A horizontal search finds the
// nearest candidate in each row; the vertical lower envelope then finds the
// Euclidean nearest point in linear time. Seeds are read-only in both passes.
inline bool extend_age(int width,int height,int channels,const std::vector<float>& hard,
                       std::vector<float>& age,const Cancel& cancel){
 const auto cells=std::size_t(width)*height;
 std::vector<int> closest(cells);
 const auto covered=[&](std::size_t i){return hard[i*channels+channels-1]>1e-5f;};
 if(!pscpu::parallel_ranges(height,cells,cancel,[&](std::size_t begin,std::size_t end,const auto& stop){
  for(auto y=begin;y<end;y++){
   if(stop())return false;
   const auto row=y*width;
   int at=-1;
   for(int x=0;x<width;x++){
    if(covered(row+x))at=x;
    closest[row+x]=at;
   }
   at=-1;
   for(int x=width-1;x>=0;x--){
    if(covered(row+x))at=x;
    auto& left=closest[row+x];
    if(at>=0&&(left<0||at-x<x-left))left=at;
   }
  }return true;
 }))return false;
 return pscpu::parallel_ranges(width,cells,cancel,[&](std::size_t begin,std::size_t end,const auto& stop){
  std::vector<int> row(height);
  std::vector<double> split(height);
  for(auto col=begin;col<end;col++){
   if(stop())return false;
   const int x=int(col);
   auto index=[&](int y){return std::size_t(y)*width+x;};
   auto square=[](double d){return d*d;};
   int count=0;
   for(int y=0;y<height;y++){
    const int k=closest[index(y)];
    if(k<0)continue;
    const double cost=square(k-x)+square(y);
    double start=-1;
    while(count>0){
     const int before=row[count-1];
     start=(cost-square(closest[index(before)]-x)-square(before))/(2.*(y-before));
     if(count==1||start>split[count-1])break;
     --count;
    }
    row[count]=y;split[count]=start;++count;
   }
   if(count==0)continue;
   int at=0;
   for(int y=0;y<height;y++){
    const auto i=index(y);
    while(at+1<count&&split[at+1]<y)++at;
    if(!covered(i)){
     const int seed_row=row[at];
     age[i]=age[std::size_t(seed_row)*width+closest[index(seed_row)]];
    }
   }
  }return true;
 });
}

inline bool soften(int width,int height,int channels,const std::vector<float>& hard,std::vector<float>& age,
                   double near_radius,double far_radius,bool draft,const Cancel& cancel,std::vector<float>& soft){
 const auto cells=std::size_t(width)*height;
 const bool progressive=!age.empty();
 const int strata=progressive?(draft?5:9):1;
 const double lo=std::min(near_radius,far_radius),hi=std::max(near_radius,far_radius);
 if(progressive){
  if(!extend_age(width,height,channels,hard,age,cancel))return false;
  if(!pscpu::parallel_ranges(height,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
   for(auto y=a;y<b;y++){
    if(stop())return false;
    for(int x=0;x<width;x++){
     auto& v=age[y*width+x];
     const double t=std::sqrt(near_radius<=far_radius?v:1-v)*(strata-1);
     const double base=std::floor(t),f=t-base;
     // Smoothstep gives zero slope at a radius switch; squared spacing
     // samples the fast-changing small radii more densely.
     v=float(base+f*f*(3-2*f));
    }
   }
   return true;
  }))return false;
 }
 soft.assign(cells*channels,0);
 std::vector<float> horizontal(cells);
 for(int layer=0;layer<strata;layer++){
  const double t=progressive?double(layer)/(strata-1):0;
  const double radius=progressive?std::lerp(lo,hi,t*t):near_radius;
  // Blend complete shadows at the destination, rather than spreading
  // differently weighted source bands into one another. These convex
  // weights prevent an artificial dark ring from overlapping coverage.
  auto blend=[&](std::size_t i){return progressive?std::max(0.f,1.f-std::abs(age[i]-layer)):1.f;};
  for(int k=0;k<channels;k++){
   auto input=[&](std::size_t i){return hard[i*channels+k];};
   if(radius<=0){
    if(!pscpu::parallel_ranges(height,cells,cancel,[&](std::size_t a,std::size_t b,const auto& stop){
     for(auto y=a;y<b;y++){if(stop())return false;for(int x=0;x<width;x++){const auto i=y*width+x;soft[i*channels+k]+=input(i)*blend(i);}}
     return true;
    }))return false;
   }else{
    if(!tent_pass(width,height,false,radius,cancel,input,[&](auto i,double v){horizontal[i]=float(v);}))return false;
    if(!tent_pass(width,height,true,radius,cancel,[&](auto i){return horizontal[i];},[&](auto i,double v){soft[i*channels+k]+=float(v)*blend(i);}))return false;
   }
  }
 }
 return true;
}
}
