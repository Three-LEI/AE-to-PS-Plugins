// Independent spectral-transform and aperture kernels. Existing RGB/spectrum
// rendering below this include deliberately retains its original arithmetic.
struct Term {int offset;double weight;};
struct KernelRow {int dy;std::vector<Term> terms;};
struct Aperture {std::vector<KernelRow> rows;double area=0;int padding=2;};
struct Point {double x,y;};
Aperture aperture(double rx,double ry,const Parameters& p,double scale_x,double scale_y){
 Aperture out;std::map<int,std::map<int,double>> coefficients;const int sides=p.aperture_shape?p.aperture_shape+2:0;
 const int count=sides?(p.aperture_roundness==0?sides:sides*32):128;
 const double turn=p.aperture_rotation*pi/180,cs=std::cos(turn),sn=std::sin(turn),round=p.aperture_roundness*.01;
 std::vector<Point> vertices;double miny=1e30,maxy=-1e30,maxx=0;
 for(int i=0;i<count;i++){
  const double angle=sides&&p.aperture_roundness==0?pi/sides+2*pi*i/count:2*pi*i/count;
  const double polygon=sides?std::cos(pi/sides)/std::cos(std::remainder(angle,2*pi/sides)):1;
  const double radius=sides?std::lerp(polygon,1.,round):1;
  const double x=rx*radius*std::cos(angle),y=ry*radius*std::sin(angle);const Point v{(cs*x-sn*y)*scale_x,(sn*x+cs*y)*scale_y};vertices.push_back(v);miny=std::min(miny,v.y);maxy=std::max(maxy,v.y);maxx=std::max(maxx,std::abs(v.x));
 }
 out.padding=int(std::ceil(maxx))+2;const double inner=1-p.ring_width*.01;
 auto interval=[&](double row,double scale){
  double left=1e30,right=-1e30;if(scale<=0)return std::pair{left,right};
  for(std::size_t i=0;i<vertices.size();i++){const auto a=vertices[i],b=vertices[(i+1)%vertices.size()];const double ay=a.y*scale,by=b.y*scale;
   if((ay<=row&&by>row)||(by<=row&&ay>row)){const double x=scale*(a.x+(b.x-a.x)*(row-ay)/(by-ay));left=std::min(left,x);right=std::max(right,x);}
  }return std::pair{left,right};
 };
 // Four subrows integrate vertical pixel coverage. Horizontal intervals use
 // exact fractional pixel-cell integration, including the inner aperture hole.
 for(int dy=int(std::floor(miny-.5));dy<=int(std::ceil(maxy+.5));dy++)for(int sub=0;sub<4;sub++){
  const double low=std::max(miny,dy-.5),high=std::min(maxy,dy+.5);if(high<=low)continue;
  const double row=low+(sub+.5)*(high-low)/4;
  for(int hole=0;hole<2;hole++){const auto [left,right]=interval(row,hole?inner:1.);if(right<=left)continue;const double weight=(hole?-.25:.25)*(high-low);out.area+=(right-left)*weight;
   auto endpoint=[&](double value,double coefficient){const double at=value+.5;const int index=int(std::floor(at));const double f=at-index;coefficients[dy][index]+=coefficient*(1-f);coefficients[dy][index+1]+=coefficient*f;};
   endpoint(-left,weight);endpoint(-right,-weight);}
 }
 for(const auto& [dy,terms]:coefficients){KernelRow row{dy,{}};for(const auto& [offset,weight]:terms)if(weight!=0)row.terms.push_back({offset,weight});if(!row.terms.empty())out.rows.push_back(std::move(row));}
 return out;
}
Image reduce_extended(const Image& src,int factor,const Cancel& cancel){
 if(factor==1)return src;Image out{(src.width+factor-1)/factor,(src.height+factor-1)/factor,0,0,{}};out.rgba.resize(std::size_t(out.width)*out.height*4);
 const double sx=double(src.width)/out.width,sy=double(src.height)/out.height;
 for(int y=0;y<out.height;y++){if(stop(cancel))return {};for(int x=0;x<out.width;x++){
  const double left=x*sx,right=(x+1)*sx,top=y*sy,bottom=(y+1)*sy;double sum[4]{};
  for(int yy=int(top);yy<std::min(src.height,int(std::ceil(bottom)));yy++)for(int xx=int(left);xx<std::min(src.width,int(std::ceil(right)));xx++){
   const double weight=(std::min(right,double(xx+1))-std::max(left,double(xx)))*(std::min(bottom,double(yy+1))-std::max(top,double(yy)));const auto i=(std::size_t(yy)*src.width+xx)*4;for(int k=0;k<4;k++)sum[k]+=src.rgba[i+k]*weight;
  }
  const auto i=(std::size_t(y)*out.width+x)*4;for(int k=0;k<4;k++)out.rgba[i+k]=float(sum[k]/(sx*sy));
 }}return out;
}
bool convolve_aperture(const Image& seed,const Parameters& p,int channel,double rx,double ry,double scale_x,double scale_y,Image& out,const Cancel& cancel){
 const double angle=p.aperture_rotation*pi/180;const double extent=std::max((std::abs(rx*std::cos(angle))+std::abs(ry*std::sin(angle)))*scale_x,(std::abs(rx*std::sin(angle))+std::abs(ry*std::cos(angle)))*scale_y);
 if(extent<.25){for(std::size_t i=0;i<seed.rgba.size();i+=4){out.rgba[i+channel]=seed.rgba[i+channel];out.rgba[i+3]=std::max(out.rgba[i+3],seed.rgba[i+3]);}return !stop(cancel);}
 const auto kernel=aperture(rx,ry,p,scale_x,scale_y);if(kernel.area<1e-10){for(std::size_t i=0;i<seed.rgba.size();i+=4){out.rgba[i+channel]=seed.rgba[i+channel];out.rgba[i+3]=std::max(out.rgba[i+3],seed.rgba[i+3]);}return !stop(cancel);}const int w=seed.width,h=seed.height,pad=kernel.padding;
 return pscpu::parallel_ranges(h,std::size_t(w)*h,cancel,[&](std::size_t first,std::size_t last,const auto& check){
  std::vector<double> prefix(std::size_t(w+2*pad+1)*2),sums(std::size_t(w)*2);
  for(int y=int(first);y<int(last);y++){
   if(check())return false;std::fill(sums.begin(),sums.end(),0);int previous_y=-2;
   for(std::size_t n=0;n<kernel.rows.size();n++){
    if((n&7)==0&&check())return false;const auto& row=kernel.rows[n];const int yy=border(y-row.dy,h,p.edge);if(yy<0)continue;
    if(yy!=previous_y){prefix[0]=prefix[1]=0;for(int x=-pad;x<w+pad;x++){const int xx=border(x,w,p.edge);const auto j=std::size_t(x+pad)*2;const auto i=xx<0?0:(std::size_t(yy)*w+xx)*4;prefix[j+2]=prefix[j]+(xx<0?0:seed.rgba[i+channel]);prefix[j+3]=prefix[j+1]+(xx<0?0:seed.rgba[i+3]);}previous_y=yy;}
    // Adjacent subrow endpoints sharing the same pixel cell are combined.
    // Exact fractional integration needs only a few prefix terms per row.
    for(const auto& term:row.terms){const int offset=pad+term.offset;for(int x=0;x<w;x++){const auto to=std::size_t(x)*2,from=std::size_t(x+offset)*2;sums[to]+=prefix[from]*term.weight;sums[to+1]+=prefix[from+1]*term.weight;}}
   }
   for(int x=0;x<w;x++){const auto i=(std::size_t(y)*w+x)*4;out.rgba[i+channel]=float(sums[std::size_t(x)*2]/kernel.area);out.rgba[i+3]=std::max(out.rgba[i+3],float(std::max(0.,sums[std::size_t(x)*2+1]/kernel.area)));}
  }return true;
 });
}
double extended_mask(const Image* mask,const Image& src,int x,int y,const Parameters& p){
 if(!mask)return 1;const auto q=sample(*mask,(x+.5)*mask->width/src.width-.5,(y+.5)*mask->height/src.height-.5,Edge::repeat);const double m=p.mask_mode==0?unit(q[3]):unit(.2126f*q[0]+.7152f*q[1]+.0722f*q[2])*unit(q[3]);return p.invert_mask?1-m:m;
}
void extended_pixel(const Image& src,const Parameters& p,std::size_t i,std::array<double,3> color,double alpha,double mix,Image& out){
 const double original_alpha=unit(src.rgba[i+3]),remainder=p.highlights_only?1-gate(src,i,p):0;
 const double affected_alpha=std::min(1.,original_alpha*remainder+alpha);for(int k=0;k<3;k++)color[k]+=finite(src.rgba[i+k])*original_alpha*remainder;
 double final_alpha=affected_alpha;if(p.alpha==Alpha::preserve){for(auto& c:color)c=affected_alpha>1e-12?c/affected_alpha*original_alpha:0;final_alpha=original_alpha;}else if(p.alpha==Alpha::black)final_alpha=1;
 const double a=original_alpha*(1-mix)+final_alpha*mix;
 for(int k=0;k<3;k++){const double v=finite(src.rgba[i+k])*original_alpha*(1-mix)+color[k]*mix;out.rgba[i+k]=finite(float(a>1e-12?v/a:0));}out.rgba[i+3]=unit(float(a));
}
bool render_extended(const Image& src,const Parameters& p,Image& output,const Image* mask,const Cancel& cancel){
 const bool neutral_warp=p.mode==Mode::warp&&p.softness==0&&p.alpha==Alpha::expand&&std::all_of(p.warp.begin(),p.warp.end(),[](const auto& t){return t.scale==100&&t.rotation==0&&t.x==0&&t.y==0;});
 if(neutral_warp||p.mix==0||(p.mode==Mode::warp&&p.warp_amount==0)||(p.mode==Mode::defocus&&(p.defocus_radius==0||std::max({p.defocus_radii[0],p.defocus_radii[1],p.defocus_radii[2]})==0))){output=src;return true;}
 const double gamma=p.linear_light?p.gamma:1;Image seed{src.width,src.height,src.left,src.top,{}};seed.rgba.resize(src.rgba.size());
 if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& check){
  for(int y=int(first);y<int(last);y++){if(check())return false;for(int x=0;x<src.width;x++){const auto i=(std::size_t(y)*src.width+x)*4;const double a=unit(src.rgba[i+3])*gate(src,i,p);for(int k=0;k<3;k++)seed.rgba[i+k]=float(transfer(finite(src.rgba[i+k])*a,gamma));seed.rgba[i+3]=float(std::pow(a,gamma));}}return true;
 }))return false;
 Image out{src.width,src.height,src.left,src.top,std::vector<float>(src.rgba.size())};
 if(p.mode==Mode::warp){
  if(p.softness>0&&!blur(seed,p.softness,p.edge,cancel))return false;
  const auto nodes=spectrum(p);const double cx=p.center_x*src.width-.5,cy=p.center_y*src.height-.5,diagonal=.5*std::hypot(src.width,src.height);
  struct WarpSample {double cs,sn,scale,x,y;std::array<double,3> weight;};std::vector<WarpSample> transforms;
  for(const auto& node:nodes){double t=std::pow(node.position,p.dispersion_power*.5);if(p.reverse)t=1-t;const double turn=std::lerp(p.warp[0].rotation,p.warp[1].rotation,t)*pi/180;
   transforms.push_back({std::cos(turn),std::sin(turn),std::exp(std::lerp(std::log(p.warp[0].scale*.01),std::log(p.warp[1].scale*.01),t)),std::lerp(p.warp[0].x,p.warp[1].x,t),std::lerp(p.warp[0].y,p.warp[1].y,t),node.weight});
  }
  if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& check){
   for(int y=int(first);y<int(last);y++){if(check())return false;for(int x=0;x<src.width;x++){
    if((x&63)==0&&check())return false;const auto i=(std::size_t(y)*src.width+x)*4;double strength=p.warp_amount*.01*extended_mask(mask,src,x,y,p);
    if(p.falloff){const double inner=p.protect*.01,t=std::clamp((std::hypot(x-cx,y-cy)/diagonal-inner)/(1-inner),0.,1.);double f=std::pow(t*t*(3-2*t),p.falloff_power);strength*=p.invert_falloff?1-f:f;}
    if(strength==0){std::copy_n(src.rgba.data()+i,4,out.rgba.data()+i);continue;}
    std::array<double,3> color{},alpha{};
    for(const auto& t:transforms){const double xx=x-cx-t.x,yy=y-cy-t.y,qx=cx+(xx*t.cs+yy*t.sn)/t.scale,qy=cy+(-xx*t.sn+yy*t.cs)/t.scale;
     const auto q=sample(seed,x+(qx-x)*strength,y+(qy-y)*strength,p.edge);for(int k=0;k<3;k++){color[k]+=t.weight[k]*q[k];alpha[k]+=t.weight[k]*std::max(0.f,q[3]);}
    }
    for(int k=0;k<3;k++){color[k]=transfer(color[k],1/gamma);alpha[k]=std::pow(std::max(0.,alpha[k]),1/gamma);}extended_pixel(src,p,i,color,std::max({alpha[0],alpha[1],alpha[2]}),p.mix*.01,out);
   }}return true;
  }))return false;
 }else{
  const double stretch=std::sqrt(p.aperture_aspect),largest=p.defocus_radius*std::max({p.defocus_radii[0],p.defocus_radii[1],p.defocus_radii[2]})*.01*std::max(stretch,1/stretch);const double limit=p.defocus_quality==0?8:p.defocus_quality==1?16:32;
  int factor=1;while(largest/factor>limit&&factor<4096)factor*=2;
  auto work=reduce_extended(seed,factor,cancel);if(!work.valid())return false;Image filtered{work.width,work.height,0,0,std::vector<float>(work.rgba.size())};
  for(int k=0;k<3;k++){const double radius=p.defocus_radius*p.defocus_radii[k]*.01;const double rx=radius*stretch,ry=radius/stretch;if(!convolve_aperture(work,p,k,rx,ry,double(work.width)/src.width,double(work.height)/src.height,filtered,cancel))return false;}
  if(!pscpu::parallel_ranges(src.height,src.rgba.size(),cancel,[&](std::size_t first,std::size_t last,const auto& check){
   for(int y=int(first);y<int(last);y++){if(check())return false;for(int x=0;x<src.width;x++){const auto i=(std::size_t(y)*src.width+x)*4;const double mix=p.mix*.01*extended_mask(mask,src,x,y,p);if(mix==0){std::copy_n(src.rgba.data()+i,4,out.rgba.data()+i);continue;}
    const auto q=sample(filtered,(x+.5)*work.width/src.width-.5,(y+.5)*work.height/src.height-.5,p.edge);std::array<double,3> color;for(int k=0;k<3;k++)color[k]=transfer(q[k],1/gamma);extended_pixel(src,p,i,color,std::pow(std::max(0.f,q[3]),1/gamma),mix,out);
   }}return true;
  }))return false;
 }
 if(stop(cancel))return false;output=std::move(out);return true;
}
