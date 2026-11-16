// A static, path-wrapped emission field, independent of the legacy upward
// heat propagation. No original plugin code or private assets are used.
namespace energy::detail {
bool burning_fields(const Image& src,const Parameters& p,const std::vector<float>& base,std::vector<float>& flame,std::vector<float>& heat,const Cancel& cancel){
 const int w=src.width,h=src.height;flame.assign(base.size(),0);heat.assign(base.size(),0);
 if(p.burn_width==0||p.burn_density==0||std::none_of(base.begin(),base.end(),[](float value){return value>0;}))return true;
 auto envelope=base;std::vector<float> thread;
 if(!light_blur(envelope,w,h,p.burn_width,cancel))return false;
 if(p.burn_filaments>0){thread=base;if(!light_blur(thread,w,h,std::max(.25,p.burn_width*.12),cancel))return false;}
 const double step=std::clamp(p.burn_scale/16,1.,4.),ax=p.start_x+p.offset_x,ay=p.start_y+p.offset_y;
 const double gx0=ax-src.left-.5+std::floor((src.left+.5-ax)/step)*step,gy0=ay-src.top-.5+std::floor((src.top+.5-ay)/step)*step;
 const int gw=int(std::ceil((w-1-gx0)/step))+2,gh=int(std::ceil((h-1-gy0)/step))+2;
 std::array<std::vector<float>,4> flow;for(auto& field:flow)field.resize(std::size_t(gw)*gh);
 auto noise_params=p;noise_params.detail=p.burn_detail;noise_params.seed=p.burn_seed;
 const double phase=(p.evolution+p.burn_evolution)/90;
 if(!pscpu::parallel_ranges(gh,std::size_t(gw)*gh,cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<gw;x++){
   const double u=(src.left+gx0+x*step+.5-ax)/p.burn_scale,v=(src.top+gy0+y*step+.5-ay)/p.burn_scale;
   const double a=fbm(u+phase*.31,v-phase*.23,noise_params,2309),b=fbm(u-phase*.19,v+phase*.37,noise_params,2917);
   const auto n=y*gw+x;flow[0][n]=float(a);flow[1][n]=float(b);
   flow[2][n]=float(fbm(u*2.7+a,v*2.7+b-phase*.51,noise_params,3253));
   flow[3][n]=float(fbm(u*6.1+b,v*6.1-a+phase*.43,noise_params,3767));
  }}return true;}))return false;
 const double angle=std::remainder(p.fire_direction,360.)*pi/180,wind_x=std::cos(angle)*p.burn_wind,wind_y=std::sin(angle)*p.burn_wind;
 const double normalize=std::min(32.,p.burn_width/(p.input==3?std::max(1.,p.burn_width*.5):std::max(.35,p.width)));
 if(!pscpu::parallel_ranges(h,base.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<w;x++){
   const double xx=(x-gx0)/step,yy=(double(y)-gy0)/step;
   const double a=sample(flow[0],gw,gh,xx,yy),b=sample(flow[1],gw,gh,xx,yy),grain=sample(flow[2],gw,gh,xx,yy),fine=sample(flow[3],gw,gh,xx,yy);
   const double dx=(a+grain*.45)*p.burn_turbulence,dy=(b-grain*.45)*p.burn_turbulence;
   const double root=sample(envelope,w,h,x,double(y))*normalize,lift=clamp(1-root*1.5);
   const double bx=x-wind_x*lift,by=double(y)-wind_y*lift;
   const double density=clamp(.5+grain*.95+fine*.45);
   const double body=sample(envelope,w,h,bx+dx,by+dy)*normalize*density*std::sqrt(density);
   double ribbons=0;
   if(p.burn_filaments>0){
    const double shifts[3][2]={{dx,dy},{-.7*dy,.7*dx},{1.25*(dx+dy*.4),1.25*(dy-dx*.4)}};
    for(const auto& shift:shifts)ribbons=std::max(ribbons,double(sample(thread,w,h,bx+shift[0],by+shift[1])));
    ribbons*=clamp(.4+grain*.8+fine*.65)*p.burn_filaments;
   }
   const auto n=y*w+x;flame[n]=float((body*.9+ribbons*.8)*p.burn_density);
   heat[n]=float(clamp(root*.75+density*.55));
  }}return true;}))return false;
 return true;
}
}
