#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace pscore {
enum class Blend:int {none,normal,add,multiply,screen,overlay,soft_light,hard_light,darken,lighten,difference,exclusion,subtract,divide,color_burn,color_dodge,linear_burn,linear_light,vivid_light,pin_light,hard_mix,hue,saturation,color,luminosity,behind,stencil_alpha,stencil_luma,silhouette_alpha,silhouette_luma,dissolve,count};
using RGBA=std::array<float,4>;
using RGB=std::array<double,3>;
inline double luminosity(const RGB& c){return .3*c[0]+.59*c[1]+.11*c[2];}
inline double saturation(const RGB& c){return *std::max_element(c.begin(),c.end())-*std::min_element(c.begin(),c.end());}
inline RGB set_lum(RGB c,double l){
 const double d=l-luminosity(c);for(auto& v:c)v+=d;
 const double lo=*std::min_element(c.begin(),c.end()),hi=*std::max_element(c.begin(),c.end());
 if(lo<0)for(auto& v:c)v=l+(v-l)*l/(l-lo);
 if(hi>1)for(auto& v:c)v=l+(v-l)*(1-l)/(hi-l);
 return c;
}
inline RGB set_sat(RGB c,double s){
 int lo=0,hi=0;for(int i=1;i<3;++i){if(c[i]<c[lo])lo=i;if(c[i]>c[hi])hi=i;}
 if(lo==hi)return {};const int mid=3-lo-hi;
 c[mid]=(c[mid]-c[lo])*s/(c[hi]-c[lo]);c[hi]=s;c[lo]=0;return c;
}
inline double burn(double b,double s){return s<=0?0:1-std::min(1.,(1-b)/s);}
inline double dodge(double b,double s){return s>=1?1:std::min(1.,b/(1-s));}
inline double channel(double b,double s,Blend mode){
 switch(mode){
 case Blend::add:return b+s;case Blend::multiply:return b*s;case Blend::screen:return b+s-b*s;
 case Blend::overlay:return b<=.5?2*b*s:1-2*(1-b)*(1-s);
 case Blend::hard_light:return s<=.5?2*b*s:1-2*(1-b)*(1-s);
 case Blend::soft_light:{const double d=b<=.25?((16*b-12)*b+4)*b:std::sqrt(std::max(0.,b));return s<=.5?b-(1-2*s)*b*(1-b):b+(2*s-1)*(d-b);}
 case Blend::darken:return std::min(b,s);case Blend::lighten:return std::max(b,s);
 case Blend::difference:return std::abs(b-s);case Blend::exclusion:return b+s-2*b*s;
 case Blend::subtract:return b-s;case Blend::divide:return s==0?1:b/s;
 case Blend::color_burn:return burn(b,s);case Blend::color_dodge:return dodge(b,s);
 case Blend::linear_burn:return std::max(0.,b+s-1);case Blend::linear_light:return b+2*s-1;
 case Blend::vivid_light:return s<=.5?burn(b,2*s):dodge(b,2*s-1);
 case Blend::pin_light:return s<=.5?std::min(b,2*s):std::max(b,2*s-1);
 case Blend::hard_mix:return (s<=.5?burn(b,2*s):dodge(b,2*s-1))<.5?0:1;
 default:return s;
 }
}
inline RGBA composite(RGBA base,RGBA foreground,Blend mode,std::uint32_t random=0){
 double a=std::clamp(double(foreground[3]),0.,1.),b=std::clamp(double(base[3]),0.,1.);
 if(mode==Blend::none){if(a==0)return {};foreground[3]=float(a);return foreground;}
 if(mode==Blend::stencil_alpha||mode==Blend::stencil_luma||mode==Blend::silhouette_alpha||mode==Blend::silhouette_luma){
  double mask=a;if(mode==Blend::stencil_luma||mode==Blend::silhouette_luma)mask*=std::clamp(luminosity({foreground[0],foreground[1],foreground[2]}),0.,1.);
  base[3]=float(b*((mode==Blend::silhouette_alpha||mode==Blend::silhouette_luma)?1-mask:mask));if(base[3]==0)base[0]=base[1]=base[2]=0;return base;
 }
 if(mode==Blend::dissolve){a=double(random)/4294967296.<a?1:0;mode=Blend::normal;}
 if(a==0)return base;
 RGB cb{base[0],base[1],base[2]},cs{foreground[0],foreground[1],foreground[2]},mixed;
 if(b==0)cb={};
 if(mode==Blend::hue)mixed=set_lum(set_sat(cs,saturation(cb)),luminosity(cb));
 else if(mode==Blend::saturation)mixed=set_lum(set_sat(cb,saturation(cs)),luminosity(cb));
 else if(mode==Blend::color)mixed=set_lum(cs,luminosity(cb));
 else if(mode==Blend::luminosity)mixed=set_lum(cb,luminosity(cs));
 else for(int k=0;k<3;++k)mixed[k]=channel(cb[k],cs[k],mode);
 const double opacity=a+b-a*b;RGBA out{};out[3]=float(opacity);
 for(int k=0;k<3;++k){const double premul=mode==Blend::behind?cb[k]*b+cs[k]*a*(1-b):cs[k]*a*(1-b)+mixed[k]*a*b+cb[k]*b*(1-a);out[k]=std::isfinite(premul)?float(premul/opacity):0;}
 return out;
}
inline RGBA composite_locked(RGBA base,RGBA foreground,Blend mode,std::uint32_t random=0){
 const float alpha=base[3];base[3]=1;auto result=composite(base,foreground,mode,random);
 if(mode==Blend::none||mode==Blend::stencil_alpha||mode==Blend::stencil_luma||mode==Blend::silhouette_alpha||mode==Blend::silhouette_luma)for(int k=0;k<3;++k)result[k]*=result[3];
 result[3]=alpha;return result;
}
}
