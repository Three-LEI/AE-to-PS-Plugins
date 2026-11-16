// Public controls inspired the separation of core and halo; all fields below
// are independently implemented, with no original shader or preset data.
namespace energy::detail {
bool textured_electric(const Image& src,const Parameters& p,const Path& input,Image& out,const Cancel& cancel){
 const int w=src.width,h=src.height;std::vector<float> base(std::size_t(w)*h),core(base.size());Path path;
 if(p.input==3){
  std::vector<float> alpha(base.size());for(std::size_t n=0;n<alpha.size();n++)alpha[n]=float(clamp(std::isfinite(src.rgba[n*4+3])?src.rgba[n*4+3]:0));
  if(!pscpu::parallel_ranges(h,base.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
   if(stop())return false;for(int x=0;x<w;x++){
    const double bx=x-p.offset_x,by=double(y)-p.offset_y;
    base[y*w+x]=sample(alpha,w,h,bx,by);
    const double u=(src.left+x+.5-p.start_x-p.offset_x)/p.wavelength,v=(src.top+double(y)+.5-p.start_y-p.offset_y)/p.wavelength,phase=p.evolution/90;
    core[y*w+x]=p.distortion>0?sample(alpha,w,h,bx+fbm(u+phase,v,p,971)*p.distortion,by+fbm(u,v-phase,p,1297)*p.distortion):base[y*w+x];
   }}return true;}))return false;
 }else{
  if(p.input==2){if(!trace_alpha(src,p.alpha_threshold,path,cancel))return false;}else path=p.input?input:line_path(p);
  std::vector<Segment> flat;if(!flatten(path,.2,flat,cancel)||!rasterize(src,p,flat,base,cancel))return false;
  if(p.distortion==0&&p.branches==0)core=base;
  else{
   std::vector<Segment> wire,branches;if(!electric(path,p,wire,branches,cancel)||!rasterize(src,p,wire,core,cancel))return false;
   if(!branches.empty()){auto q=p;q.width=std::max(.1,p.width*.55);q.trim_start=0;q.trim_end=100;q.taper_start=0;q.taper_end=100;if(!rasterize(src,q,branches,core,cancel))return false;}
  }
 }
 if(p.style==4){std::vector<float> flame,heat;if(!burning_fields(src,p,base,flame,heat,cancel))return false;return layered_composite(src,p,core,out,cancel,&flame,&heat,&base);}
 return layered_composite(src,p,core,out,cancel,nullptr,nullptr,&base);
}
bool electric_halo(const Image& src,const Parameters& p,const std::vector<float>& seed,std::vector<float>& light,const Cancel& cancel){
 if(p.glow_distortion==0&&p.glow_texture==0)return true;
 const int w=src.width,h=src.height;
 // A bounded flow grid avoids evaluating fractal noise for every filament and
 // every output pixel. Coordinates are anchored to saved effect coordinates.
 const double step=std::clamp(p.glow_noise_scale/16,1.,4.);
 const double anchor_x=p.start_x+p.offset_x,anchor_y=p.start_y+p.offset_y;
 const double gx0=anchor_x-src.left-.5+std::floor((src.left+.5-anchor_x)/step)*step,gy0=anchor_y-src.top-.5+std::floor((src.top+.5-anchor_y)/step)*step;
 const int gw=int(std::ceil((w-1-gx0)/step))+2,gh=int(std::ceil((h-1-gy0)/step))+2;
 std::array<std::vector<float>,4> flow;for(auto& plane:flow)plane.resize(std::size_t(gw)*gh);
 auto noise_params=p;noise_params.seed=p.glow_seed;noise_params.detail=p.glow_detail;
 const double phase=(p.evolution+p.glow_evolution)/90;
 if(!pscpu::parallel_ranges(gh,std::size_t(gw)*gh,cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<gw;x++){
   const double u=(src.left+gx0+x*step+.5-anchor_x)/p.glow_noise_scale,v=(src.top+gy0+y*step+.5-anchor_y)/p.glow_noise_scale;
   const auto n=y*gw+x;
   const double dx=fbm(u+phase*.37,v-phase*.21,noise_params,311),dy=fbm(u-phase*.17,v+phase*.43,noise_params,719);
   flow[0][n]=float(dx);flow[1][n]=float(dy);
   flow[2][n]=float(fbm(u*3+dx,v*3+dy-phase*.6,noise_params,1291));
   flow[3][n]=float(fbm(u*7+dy,v*7-dx+phase*.3,noise_params,1733));
  }}return true;}))return false;
 std::vector<float> cloud,thin;
 if(p.glow_texture>0&&p.glow_width>0){cloud=seed;if(!light_blur(cloud,w,h,p.glow_width,cancel))return false;
  if(p.filament_strength>0&&p.filament_count>0){thin=seed;if(!light_blur(thin,w,h,std::max(.25,p.glow_width*.1),cancel))return false;}
 }
 std::array<double,6> cosine,sine;for(int i=0;i<6;i++){cosine[i]=std::cos(i*2.399963229728653);sine[i]=std::sin(i*2.399963229728653);}
 std::vector<float> warped_light(light.size());
 if(!pscpu::parallel_ranges(h,seed.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<w;x++){
   const auto n=y*w+x;const double xx=(x-gx0)/step,yy=(double(y)-gy0)/step;
   const double dx=sample(flow[0],gw,gh,xx,yy)*p.glow_distortion,dy=sample(flow[1],gw,gh,xx,yy)*p.glow_distortion;
   double value=sample(light,w,h,x+dx,double(y)+dy);
   if(!cloud.empty()){
    const double grain=sample(flow[2],gw,gh,xx,yy),fine=sample(flow[3],gw,gh,xx,yy);
    const double density=clamp(.55+grain*.7+fine*.35);
    double texture=sample(cloud,w,h,x+dx,double(y)+dy)*(.25+density*1.5)*std::min(32.,p.glow_width/(p.input==3?1:std::max(.25,p.width)));
    double ribbons=0;
    for(int i=0;i<p.filament_count&&!thin.empty();i++){
     const double cs=cosine[i],sn=sine[i],scale=.7+i*.29;
     const double rx=(dx*cs-dy*sn)*scale,ry=(dx*sn+dy*cs)*scale;
     ribbons=std::max(ribbons,double(sample(thin,w,h,x+rx,double(y)+ry))*clamp(.25+grain*.8+fine*.6));
    }
    texture+=ribbons*p.filament_strength*.9;
    value+=texture*p.glow_texture;
   }
   warped_light[n]=float(value);
  }}return true;}))return false;
 light.swap(warped_light);return true;
}
}
