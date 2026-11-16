// Independent layered light model. The legacy compositor is left intact.
namespace energy::detail {
namespace {
// Fractional running boxes keep radius changes continuous and cost O(pixels).
bool light_box(std::vector<float>& field,int w,int h,double radius,bool vertical,const Cancel& cancel){
 if(radius<1e-8)return true;
 const int r=int(std::floor(radius)),length=vertical?h:w,lines=vertical?w:h;
 const double fraction=radius-r,norm=1/(2*radius+1);
 std::vector<float> result(field.size());
 if(!pscpu::parallel_ranges(lines,field.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(auto line=first;line<last;line++){
   if(stop())return false;
   auto at=[&](int p)->float{return p<0||p>=length?0.f:field[vertical?std::size_t(p)*w+line:line*w+p];};
   double sum=0;for(int p=0;p<=std::min(length-1,r);p++)sum+=at(p);
   for(int p=0;p<length;p++){
    result[vertical?std::size_t(p)*w+line:line*w+p]=float(std::max(0.,sum+fraction*(at(p-r-1)+at(p+r+1)))*norm);
    sum-=at(p-r);sum+=at(p+r+1);
   }
  }return true;
 }))return false;
 field.swap(result);return true;
}
bool light_blur(std::vector<float>& field,int w,int h,double sigma,const Cancel& cancel){
 const double radius=(std::sqrt(4*sigma*sigma+1)-1)*.5;
 for(int pass=0;pass<3;pass++)if(!light_box(field,w,h,radius,false,cancel)||!light_box(field,w,h,radius,true,cancel))return false;
 return true;
}
}
bool layered_composite(const Image& src,const Parameters& p,const std::vector<float>& core,Image& out,const Cancel& cancel,const std::vector<float>* flame,const std::vector<float>* heat,const std::vector<float>* seed_override){
 const int w=src.width,h=src.height;const auto size=core.size();
 std::vector<float> seed(size),light(size),halo;bool active=false;
 for(std::size_t n=0;n<size;n++){
  if((n&16383)==0&&cancel&&cancel())return false;
  const double value=std::max(0.,double(seed_override?(*seed_override)[n]:core[n])+(flame?(*flame)[n]*.6:0));
  seed[n]=float(std::pow(value,1/p.glow_gamma));
  active|=seed[n]>0;
 }
 if(active&&p.brightness>0&&p.glow_radius>0&&p.glow_intensity>0){
  for(int band=0;band<4;band++){
   if(p.glow_gains[band]==0||p.glow_sizes[band]==0)continue;
   auto field=seed;if(!light_blur(field,w,h,p.glow_radius*p.glow_sizes[band],cancel))return false;
   const double gain=14*p.glow_gains[band]*std::pow(std::max(.001,p.glow_sizes[band]),.5-p.glow_bias);
   for(std::size_t n=0;n<size;n++){if((n&16383)==0&&cancel&&cancel())return false;light[n]+=float(field[n]*gain);}
  }
 }
 if(active&&p.brightness>0&&p.halo_radius>0&&p.halo_intensity>0){
  halo=seed;
  // The tight bright rim stays attached to the core. Only the outer light
  // field is independently distorted in the new electric style.
  if(p.style==3)for(std::size_t n=0;n<size;n++){if((n&16383)==0&&cancel&&cancel())return false;halo[n]=float(std::pow(std::max(0.,double(core[n])),1/p.glow_gamma));}
  if(p.style==4)for(std::size_t n=0;n<size;n++){if((n&16383)==0&&cancel&&cancel())return false;halo[n]=float(std::pow(std::max(0.,double(core[n])+(flame?(*flame)[n]*.25:0)),1/p.glow_gamma));}
  if(!light_blur(halo,w,h,p.halo_radius,cancel))return false;
 }
 if(active&&p.brightness>0&&p.style==3&&!electric_halo(src,p,seed,light,cancel))return false;
 std::array<double,3> core_color,halo_color;
 for(int k=0;k<3;k++){core_color[k]=(p.color[k]*(1-p.core_white*.01)+p.core_white*.01)*1.2;halo_color[k]=1-std::pow(1-p.color[k],3.5);}
 Image next{w,h,src.left,src.top,std::vector<float>(size*4)};
 if(!pscpu::parallel_ranges(h,size,cancel,[&](std::size_t first,std::size_t last,const auto& stop){
  for(auto y=first;y<last;y++){
   if(stop())return false;
   for(int x=0;x<w;x++){
    const auto n=y*w+x,i=n*4;const double sa=p.composite==0?std::clamp(double(src.rgba[i+3]),0.,1.):0;
    double emission[3],maximum=0;
    for(int k=0;k<3;k++){
     emission[k]=p.brightness*(core[n]*core_color[k]+light[n]*p.glow_intensity*p.color[k]+(halo.empty()?0:halo[n]*p.halo_intensity*halo_color[k]));
     if(flame){const double hot=std::clamp(double((*heat)[n]),0.,1.);emission[k]+=p.brightness*(*flame)[n]*(p.tip_color[k]+(p.color[k]-p.tip_color[k])*hot)*2;}
     maximum=std::max(maximum,emission[k]);
    }
    if(maximum==0&&p.composite==0){std::copy_n(src.rgba.data()+i,4,next.rgba.data()+i);continue;}
    const double ea=std::clamp(maximum,0.,1.),alpha=p.composite==2?1:sa+(1-sa)*ea;next.rgba[i+3]=float(alpha);
    for(int k=0;k<3;k++){
     double before=p.composite==0?double(src.rgba[i+k]):0;if(!std::isfinite(before))before=0;
     // Work in the document's channel values, matching a non-linearized AE
     // fixture. Straight-alpha output retains HDR energy without RGB clipping.
     next.rgba[i+k]=alpha?float((before*sa+emission[k])/alpha):0;
    }
   }
  }return true;
 }))return false;
 out=std::move(next);return true;
}
}
