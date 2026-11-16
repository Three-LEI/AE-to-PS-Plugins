// Common emitter -> core noise -> outer noise/texture -> light/color stages.
// Presets only supply parameter values; this pipeline never reads p.style.
namespace energy::detail {
namespace {
double u_basis(double value,int kind){return kind==2?std::copysign(std::sqrt(std::abs(value)),value):value;}
double u_fbm(double x,double y,int kind,int detail,std::uint32_t seed){
 double sum=0,norm=0,weight=1;for(int i=0;i<detail;i++){sum+=u_basis(noise2(x,y,seed+std::uint32_t(i)*0x9e3779b9u),kind)*weight;norm+=weight;x*=kind==0?1.8:2.03;y*=kind==0?1.8:2.03;weight*=kind==0?.3:kind==1?.5:.55;}return sum/norm;
}
bool u_wire(const Path& path,const Parameters& p,std::vector<Segment>& out,std::vector<Segment>& branches,const Cancel& cancel){
 std::vector<ElectricCurve> curves;std::size_t total_points=0;double total_length=0;
 for(std::size_t index=0;index<path.curves.size();index++){
  if(cancel&&cancel())return false;const auto& original=path.curves[index];std::vector<Segment> flat;if(!flatten(Path{{original}},.2,flat,cancel))return false;
  double size=0;std::vector<double> ends;for(const auto& s:flat){size+=length(s.a,s.b);ends.push_back(size);}if(size<=1e-10){out.insert(out.end(),flat.begin(),flat.end());continue;}
  const double desired=std::ceil(size/std::max(.25,std::min(5.,p.wavelength/12)));if(desired>262144)return false;const int count=std::max(2,int(desired));if(total_points+count+1>262144)return false;total_points+=count+1;
  std::vector<Point> base;std::size_t at=0;for(int j=0;j<=count;j++){const double d=size*j/count;while(at+1<flat.size()&&ends[at]<d)at++;const double begin=at?ends[at-1]:0,span=ends[at]-begin;base.push_back(interpolate(flat[at].a,flat[at].b,span?(d-begin)/span:0));}
  ElectricCurve curve;curve.closed=original.closed;curve.length=size;curve.points=base;
  for(int j=0;j<=count;j++){
   const auto before=base[j?j-1:(original.closed?count-1:0)],after=base[j<count?j+1:(original.closed?1:count)];const double dx=after.x-before.x,dy=after.y-before.y,n=std::hypot(dx,dy),u=double(j)/count;double displacement=0,weight=1,norm=0;
   for(int octave=0;octave<p.detail;octave++){const int period=original.closed?std::max(1,int(std::lround(size/p.wavelength)))*(1<<octave):0;const double position=period?u*period:u*size/p.wavelength*(1<<octave);displacement+=u_basis(noise(position+p.evolution/90,p.seed+std::uint32_t(index)*0x85ebca6bu+std::uint32_t(octave)*0x9e3779b9u,period),p.core_noise)*weight;norm+=weight;weight*=p.core_noise==0?.3:p.core_noise==1?.5:.55;}
   const double envelope=original.closed?1:(j==0||j==count?0:std::pow(std::sin(pi*u),.75));displacement=displacement/norm*p.distortion*envelope;if(n>0){curve.points[j].x-=dy/n*displacement;curve.points[j].y+=dx/n*displacement;}
  }
  if(original.closed)curve.points.back()=curve.points.front();for(int j=0;j<count;j++)out.push_back({curve.points[j],curve.points[j+1],double(j)/count,double(j+1)/count});total_length+=size;curves.push_back(std::move(curve));
 }
 for(int b=0;b<p.branches&&total_length>0&&p.branch_length>0;b++){
  if(cancel&&cancel())return false;double pick=random(p.seed+1337,std::uint32_t(b))*total_length;const ElectricCurve* selected=&curves.back();for(const auto& c:curves){if(pick<=c.length){selected=&c;break;}pick-=c.length;}
  const double u=clamp(pick/selected->length);if(u<p.trim_start*.01||u>p.trim_end*.01)continue;const double f=u*(selected->points.size()-1);const auto at=std::min(selected->points.size()-2,std::size_t(f));const auto root=interpolate(selected->points[at],selected->points[at+1],f-at);const auto a=selected->points[at],z=selected->points[at+1];const double angle=std::atan2(z.y-a.y,z.x-a.x)+(random(p.seed+233,std::uint32_t(b))>.5?1:-1)*(pi*.25+random(p.seed+811,std::uint32_t(b))*pi*.35);
  const double size=p.branch_length*(.5+random(p.seed+993,std::uint32_t(b))*.5);const int steps=std::max(2,int(std::ceil(size/2)));Point previous=root;
  for(int j=1;j<=steps;j++){const double t=double(j)/steps,shift=u_basis(noise(t*4+p.evolution/90,p.seed+std::uint32_t(b)*97),p.core_noise)*p.distortion*.3*std::sin(pi*t);Point next{root.x+std::cos(angle)*size*t-std::sin(angle)*shift,root.y+std::sin(angle)*size*t+std::cos(angle)*shift};branches.push_back({previous,next,double(j-1)/steps,t});previous=next;}
 }
 return true;
}
bool u_emitter(const Image& src,const Parameters& p,const Path& input,std::vector<float>& base,std::vector<float>& core,const Cancel& cancel){
 const int w=src.width,h=src.height;base.assign(std::size_t(w)*h,0);core.assign(base.size(),0);
 if(p.input==3){
  std::vector<float> alpha(base.size());for(std::size_t n=0;n<alpha.size();n++){if((n&16383)==0&&cancel&&cancel())return false;alpha[n]=float(clamp(std::isfinite(src.rgba[n*4+3])?src.rgba[n*4+3]:0));}
  return pscpu::parallel_ranges(h,base.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
   if(stop())return false;for(int x=0;x<w;x++){
    const double bx=x-p.offset_x,by=double(y)-p.offset_y;base[y*w+x]=sample(alpha,w,h,bx,by);
    if(p.distortion==0){core[y*w+x]=base[y*w+x];continue;}
    const double u=(src.left+x+.5-p.start_x-p.offset_x)/p.wavelength,v=(src.top+double(y)+.5-p.start_y-p.offset_y)/p.wavelength,phase=p.evolution/90;
    core[y*w+x]=sample(alpha,w,h,bx+u_fbm(u+phase,v,p.core_noise,p.detail,p.seed+971)*p.distortion,by+u_fbm(u,v-phase,p.core_noise,p.detail,p.seed+1297)*p.distortion);
   }}return true;});
 }
 Path path;if(p.input==2){if(!trace_alpha(src,p.alpha_threshold,path,cancel))return false;}else path=p.input?input:line_path(p);
 std::vector<Segment> flat;if(!flatten(path,.2,flat,cancel)||!rasterize(src,p,flat,base,cancel))return false;
 if(p.distortion==0&&p.branches==0){core=base;return true;}
 std::vector<Segment> wire,branches;if(!u_wire(path,p,wire,branches,cancel)||!rasterize(src,p,wire,core,cancel))return false;
 if(!branches.empty()){auto q=p;q.width=std::max(.1,p.width*.55);q.trim_start=0;q.trim_end=100;q.taper_start=0;q.taper_end=100;if(!rasterize(src,q,branches,core,cancel))return false;}
 return true;
}
struct UOuter {
 int w=0,h=0,gw=0,gh=0;double step=1,gx0=0,gy0=0,normalize=1,wind_x=0,wind_y=0;
 std::array<std::vector<float>,4> flow;std::vector<float> envelope,body,heat;
 std::array<double,2> shift(int x,int y,const Parameters& p)const{
  if(flow[0].empty())return {};
  const double xx=(x-gx0)/step,yy=(y-gy0)/step,a=sample(flow[0],gw,gh,xx,yy),b=sample(flow[1],gw,gh,xx,yy),grain=sample(flow[2],gw,gh,xx,yy);
  const double root=envelope[std::size_t(y)*w+x]*normalize,lift=clamp(1-root*1.5);
  return {(a+grain*.45)*p.glow_distortion-wind_x*lift,(b-grain*.45)*p.glow_distortion-wind_y*lift};
 }
};
bool u_outer(const Image& src,const Parameters& p,const std::vector<float>& base,UOuter& out,const Cancel& cancel){
 const bool cloud=p.glow_texture>0&&p.glow_width>0,filaments=p.filament_strength>0&&p.filament_count>0;
 if((!cloud&&!filaments&&p.glow_distortion==0&&p.burn_wind==0)||std::none_of(base.begin(),base.end(),[](float v){return v>0;}))return true;
 out.w=src.width;out.h=src.height;const int w=src.width,h=src.height;out.step=std::clamp(p.glow_noise_scale/16,1.,4.);const double ax=p.start_x+p.offset_x,ay=p.start_y+p.offset_y;
 out.gx0=ax-src.left-.5+std::floor((src.left+.5-ax)/out.step)*out.step;out.gy0=ay-src.top-.5+std::floor((src.top+.5-ay)/out.step)*out.step;
 out.gw=int(std::ceil((w-1-out.gx0)/out.step))+2;out.gh=int(std::ceil((h-1-out.gy0)/out.step))+2;
 for(auto& f:out.flow)f.resize(std::size_t(out.gw)*out.gh);
 const double phase=(p.evolution+p.glow_evolution)/90;
 if(!pscpu::parallel_ranges(out.gh,std::size_t(out.gw)*out.gh,cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<out.gw;x++){
   const double u=(src.left+out.gx0+x*out.step+.5-ax)/p.glow_noise_scale,v=(src.top+out.gy0+y*out.step+.5-ay)/p.glow_noise_scale;
   const double a=u_fbm(u+phase*.31,v-phase*.23,p.glow_noise,p.glow_detail,p.glow_seed+2309),b=u_fbm(u-phase*.19,v+phase*.37,p.glow_noise,p.glow_detail,p.glow_seed+2917);const auto n=y*out.gw+x;
   out.flow[0][n]=float(a);out.flow[1][n]=float(b);out.flow[2][n]=float(u_fbm(u*2.7+a,v*2.7+b-phase*.51,p.glow_noise,p.glow_detail,p.glow_seed+3253));out.flow[3][n]=float(u_fbm(u*6.1+b,v*6.1-a+phase*.43,p.glow_noise,p.glow_detail,p.glow_seed+3767));
  }}return true;}))return false;
 out.envelope=base;if(p.glow_width>0&&!light_blur(out.envelope,w,h,p.glow_width,cancel))return false;
 out.normalize=std::min(32.,p.glow_width/(p.input==3?std::max(1.,p.glow_width*.5):std::max(.35,p.width)));
 const double angle=std::remainder(p.fire_direction,360.)*pi/180;out.wind_x=std::cos(angle)*p.burn_wind;out.wind_y=std::sin(angle)*p.burn_wind;
 if(!cloud&&!filaments)return true;
 std::vector<float> thin;if(filaments){thin=base;if(!light_blur(thin,w,h,std::max(.25,p.glow_width*.12),cancel))return false;}
 std::array<double,6> cosine,sine;for(int i=0;i<6;i++){cosine[i]=std::cos(i*2.399963229728653);sine[i]=std::sin(i*2.399963229728653);}
 out.body.resize(base.size());out.heat.resize(base.size());
 return pscpu::parallel_ranges(h,base.size(),cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<w;x++){
   const auto n=y*w+x;const double xx=(x-out.gx0)/out.step,yy=(double(y)-out.gy0)/out.step,grain=sample(out.flow[2],out.gw,out.gh,xx,yy),fine=sample(out.flow[3],out.gw,out.gh,xx,yy);
   const auto shift=out.shift(x,int(y),p);const double density=clamp(.5+grain*.95+fine*.45);
   double body=cloud?sample(out.envelope,w,h,x+shift[0],double(y)+shift[1])*out.normalize*density*std::sqrt(density)*p.glow_texture*.9:0;
   if(filaments){double ribbons=0;for(int i=0;i<p.filament_count;i++){const double cs=cosine[i],sn=sine[i],scale=.7+i*.29,dx=(shift[0]*cs-shift[1]*sn)*scale,dy=(shift[0]*sn+shift[1]*cs)*scale;ribbons=std::max(ribbons,double(sample(thin,w,h,x+dx,double(y)+dy)));}body+=ribbons*clamp(.4+grain*.8+fine*.65)*p.filament_strength*.8;}
   out.body[n]=float(body);out.heat[n]=float(clamp(out.envelope[n]*out.normalize*.75+density*.55));
  }}return true;});
}
}
bool unified_render(const Image& src,const Parameters& p,const Path& path,Image& out,const Cancel& cancel){
 std::vector<float> base,core;if(!u_emitter(src,p,path,base,core,cancel))return false;
 UOuter outer;if(p.brightness>0&&!u_outer(src,p,base,outer,cancel))return false;
 const int w=src.width,h=src.height;const auto size=base.size();std::vector<float> seed(size),light(size),halo;
 bool active=false;for(std::size_t n=0;n<size;n++){if((n&16383)==0&&cancel&&cancel())return false;seed[n]=float(std::pow(std::max(0.,double(base[n])+(outer.body.empty()?0:outer.body[n]*.6)),1/p.glow_gamma));active|=seed[n]>0||core[n]>0;}
 if(active&&p.brightness>0&&p.glow_radius>0&&p.glow_intensity>0)for(int band=0;band<4;band++){
  if(p.glow_gains[band]==0||p.glow_sizes[band]==0)continue;auto field=seed;if(!light_blur(field,w,h,p.glow_radius*p.glow_sizes[band],cancel))return false;const double gain=14*p.glow_gains[band]*std::pow(std::max(.001,p.glow_sizes[band]),.5-p.glow_bias);
  for(std::size_t n=0;n<size;n++){if((n&16383)==0&&cancel&&cancel())return false;light[n]+=float(field[n]*gain);}
 }
 if(active&&p.brightness>0&&p.halo_radius>0&&p.halo_intensity>0){halo=core;for(auto& v:halo)v=float(std::pow(std::max(0.,double(v)),1/p.glow_gamma));if(!light_blur(halo,w,h,p.halo_radius,cancel))return false;}
 std::array<double,3> core_color,halo_color;for(int k=0;k<3;k++){core_color[k]=(p.color[k]*(1-p.core_white*.01)+p.core_white*.01)*1.2;halo_color[k]=1-std::pow(1-p.color[k],3.5);}
 Image next{w,h,src.left,src.top,std::vector<float>(size*4)};
 if(!pscpu::parallel_ranges(h,size,cancel,[&](std::size_t first,std::size_t last,const auto& stop){for(auto y=first;y<last;y++){
  if(stop())return false;for(int x=0;x<w;x++){
   const auto n=y*w+x,i=n*4;const auto shift=outer.shift(x,int(y),p);const double glow=outer.flow[0].empty()?light[n]:sample(light,w,h,x+shift[0],double(y)+shift[1]);const double heat=outer.heat.empty()?clamp(core[n]):outer.heat[n];
   const double sa=p.composite==0?std::clamp(double(src.rgba[i+3]),0.,1.):0;double emission[3],maximum=0;
   for(int k=0;k<3;k++){const double far_color=std::lerp(p.color[k],p.tip_color[k],p.color_mix*(1-heat)*.65),body_color=std::lerp(p.color[k],p.tip_color[k],p.color_mix*(1-heat));emission[k]=p.brightness*(core[n]*core_color[k]+p.glow_intensity*(glow*far_color+(outer.body.empty()?0:outer.body[n]*body_color*2))+(halo.empty()?0:halo[n]*p.halo_intensity*halo_color[k]));maximum=std::max(maximum,emission[k]);}
   if(maximum==0&&p.composite==0){std::copy_n(src.rgba.data()+i,4,next.rgba.data()+i);continue;}
   const double ea=std::clamp(maximum,0.,1.),alpha=p.composite==2?1:sa+(1-sa)*ea;next.rgba[i+3]=float(alpha);
   for(int k=0;k<3;k++){double before=p.composite==0?double(src.rgba[i+k]):0;if(!std::isfinite(before))before=0;next.rgba[i+k]=alpha?float((before*sa+emission[k])/alpha):0;}
  }}return true;}))return false;
 out=std::move(next);return true;
}
}
