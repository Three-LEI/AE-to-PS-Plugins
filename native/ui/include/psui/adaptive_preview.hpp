#pragma once
#include "area_sample.hpp"
#include "coalesced_preview.hpp"
#include "library_panel.hpp"
#include "preview_background.hpp"
#include "../../../common/parallel_ranges.hpp"
#include <chrono>
#include <deque>
#include <memory>
#include <algorithm>
#include <cmath>
namespace psui {
// Shared view-only policy. Effect parameters and host output never use this state.
struct PreviewPass {
 bool draft=false;int side=512;
 constexpr operator bool()const{return draft;}
};
class DraftResolution {
 int level=2,quick=0;
 inline static constexpr int sides[]={256,384,512,768};
public:
 int side()const{return sides[level];}
 void observe(double ms,int rendered_side){
  if(rendered_side!=side()||!std::isfinite(ms))return;
  if(ms>38&&level>0){--level;quick=0;}
  else if(ms<8&&level<3){if(++quick>=4){++level;quick=0;}}
  else quick=0;
 }
};
template<class Result>class AdaptivePreview {
 struct Timed {Result value;double milliseconds=0;int side=512;};
 std::unique_ptr<CoalescedPreview<Timed>> worker=std::make_unique<CoalescedPreview<Timed>>();
 DraftResolution resolution;double last_ms=0;
 bool refine=false,last_draft=true,fast_full=false;
 std::chrono::steady_clock::time_point due{};
 template<class Work>auto timed(Work work,bool draft,int side){
  return [work,draft,side](const auto& cancel){const auto start=std::chrono::steady_clock::now();auto value=work(PreviewPass{draft,side},cancel);return Timed{std::move(value),std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count(),side};};
 }
public:
 bool fast=true;unsigned draft_frames=0,full_frames=0;
 int draft_side()const{return resolution.side();}
 bool showing_draft()const{return last_draft;}
 double milliseconds()const{return last_ms;}
 template<class Work>bool update(bool& changed,Result& result,Work work,bool native_region=false){
  const auto now=std::chrono::steady_clock::now();
  if(worker->take_slow_interaction()&&fast&&!native_region){
   invalidate();
   changed=true;
  }
  if(changed){
   const bool precise=fast_full||native_region;
   const auto mode=!fast?PreviewRequest::Final:precise?PreviewRequest::InteractiveFull:PreviewRequest::Draft;
   const auto budget=mode==PreviewRequest::InteractiveFull&&!native_region
       ?std::chrono::milliseconds(55):std::chrono::milliseconds{};
   refine=mode==PreviewRequest::Draft;
   worker->submit(mode,timed(work,refine,resolution.side()),budget);
   changed=false;
   due=now+std::chrono::milliseconds(180);
  }else if(refine&&now>=due){
   worker->submit(PreviewRequest::Final,timed(work,false,1));
   refine=false;
  }
  bool draft=false;std::uint64_t revision=0;Timed completed;if(!worker->take(completed,draft,revision))return false;
  last_ms=completed.milliseconds;
  if(draft)resolution.observe(last_ms,completed.side);
  else if(!native_region){
   if(last_ms<=18)fast_full=true;
   else if(last_ms>=30)fast_full=false;
  }
  result=std::move(completed.value);
  last_draft=draft;if(draft)++draft_frames;else ++full_frames;return true;
 }
 void invalidate(){worker->invalidate();refine=fast_full=false;}
 void stop(){worker.reset();worker=std::make_unique<CoalescedPreview<Timed>>();refine=fast_full=false;}
 bool pending(){return refine||worker->pending();}
 bool controls(LibraryPanel& ui,bool& changed){
  const bool toggle=ui.check("交互快速预览 / Adaptive Preview",fast);
  if(toggle){invalidate();changed=true;}
  ImGui::SameLine();ImGui::TextDisabled("%s",ui.text(last_draft?"快速 / Draft":"精细 / Full").c_str());
  ui.preview_status(pending());
  ui.items["adaptive.quality"]={float(resolution.side()),float(last_ms),0,0};
  ui.items["adaptive.state"]={float(draft_frames),float(full_frames),last_draft?1.f:0.f,pending()?1.f:0.f};return toggle;
 }
};
struct PreviewSpace {
 double left=0,top=0,step=1;
 double x(double value)const{return (value-left)/step;}
 double y(double value)const{return (value-top)/step;}
 double length(double value)const{return value/step;}
};
template<class Image>struct SpatialFrame{Image image;PreviewSpace space;};
// Input snapshots are immutable while jobs run. Reference-layer changes must
// stop the worker before clearing this cache. References stay valid on insertion.
template<class Image>class DraftImageCache {
 struct Entry{const float* pixels;int width,height,left,top;PreviewSpace space;Image image;};
 std::deque<Entry> entries;
 Image cancelled_image;
public:
 void clear(){entries.clear();}
 template<class Cancel>const Image& get(const Image& source,const PreviewSpace& space,const Cancel& cancel){
  if(!source.valid()||space.step==1)return source;
  for(const auto& e:entries)if(e.pixels==source.rgba.data()&&e.width==source.width&&e.height==source.height&&e.left==source.left&&e.top==source.top&&e.space.left==space.left&&e.space.top==space.top&&e.space.step==space.step)return e.image;
  Entry entry{source.rgba.data(),source.width,source.height,source.left,source.top,space,{}};auto& out=entry.image;
  out.left=int(std::floor(space.x(source.left)));out.top=int(std::floor(space.y(source.top)));
  out.width=std::max(1,int(std::ceil(space.x(source.left+source.width)))-out.left);out.height=std::max(1,int(std::ceil(space.y(source.top+source.height)))-out.top);
  out.rgba.resize(std::size_t(out.width)*out.height*4);
  struct Span {int first,end;};
  std::vector<Span> columns(out.width);
  for(int x=0;x<out.width;++x){
   columns[x]={std::max(0,int(std::floor(space.left+(out.left+x)*space.step-source.left))),
               std::min(source.width,int(std::ceil(space.left+(out.left+x+1)*space.step-source.left)))};
  }
  const auto rows=[&](std::size_t first,std::size_t end,const auto& stopped){
   for(auto row=first;row<end;++row){
    if(stopped())return false;
    const int y=int(row);
    const int y0=std::max(0,int(std::floor(space.top+(out.top+y)*space.step-source.top)));
    const int y1=std::min(source.height,int(std::ceil(space.top+(out.top+y+1)*space.step-source.top)));
    for(int x=0;x<out.width;++x){
     const auto [x0,x1]=columns[x];
     const int count=std::max(0,y1-y0)*std::max(0,x1-x0);
     double sum[4]{};
     for(int yy=y0;yy<y1;++yy)for(int xx=x0;xx<x1;++xx){
      const auto i=(std::size_t(yy)*source.width+xx)*4;
      const double alpha=std::clamp(double(source.rgba[i+3]),0.,1.);
      for(int k=0;k<3;++k)sum[k]+=source.rgba[i+k]*alpha;
      sum[3]+=alpha;
     }
     const auto i=(row*out.width+x)*4;
     for(int k=0;k<3;++k)out.rgba[i+k]=sum[3]>0?float(sum[k]/sum[3]):0;
     out.rgba[i+3]=count?float(sum[3]/count):0;
    }
   }
   return true;
  };
  if(!pscpu::parallel_ranges(out.height,source.rgba.size(),cancel,rows)||cancel())return cancelled_image;
  entries.push_back(std::move(entry));return entries.back().image;
 }
};
template<class Image>PreviewSpace preview_space(const Image& image,PreviewPass pass){return {double(image.left),double(image.top),pass.draft?double(std::max(1,int(std::ceil(double(std::max(image.width,image.height))/std::max(1,pass.side))))):1.};}
template<class Image>PreviewSpace preview_space(const Image& image,bool draft){return preview_space(image,PreviewPass{draft,512});}
template<class Image>std::array<float,4> preview_pixel(const Image& image,double x,double y,const PreviewSpace& space,bool premultiplied=true){
 // Filter in premultiplied space at reduced-pixel area centres.
 const double fx=std::clamp(space.x(x)+.5/space.step-.5,0.,double(image.width-1)),fy=std::clamp(space.y(y)+.5/space.step-.5,0.,double(image.height-1));
 const int x0=int(fx),y0=int(fy),x1=std::min(x0+1,image.width-1),y1=std::min(y0+1,image.height-1);const double tx=fx-x0,ty=fy-y0;
 if(tx==0&&ty==0){const auto i=(std::size_t(y0)*image.width+x0)*4;return {image.rgba[i],image.rgba[i+1],image.rgba[i+2],image.rgba[i+3]};}
 std::array<double,4> sum{};
 for(int dy=0;dy<2;dy++)for(int dx=0;dx<2;dx++){const double weight=(dx?tx:1-tx)*(dy?ty:1-ty);const auto i=(std::size_t(dy?y1:y0)*image.width+(dx?x1:x0))*4;const double a=std::clamp(double(image.rgba[i+3]),0.,1.);sum[3]+=weight*a;for(int k=0;k<3;k++)sum[k]+=weight*(premultiplied?a:1)*image.rgba[i+k];}
 const double divisor=premultiplied?sum[3]:1;return {divisor>0?float(sum[0]/divisor):0,divisor>0?float(sum[1]/divisor):0,divisor>0?float(sum[2]/divisor):0,float(sum[3])};
}
template<class Image>float preview_alpha(const Image& image,double x,double y,const PreviewSpace& space){
 const double fx=std::clamp(space.x(x)+.5/space.step-.5,0.,double(image.width-1)),fy=std::clamp(space.y(y)+.5/space.step-.5,0.,double(image.height-1));
 const int x0=int(fx),y0=int(fy),x1=std::min(x0+1,image.width-1),y1=std::min(y0+1,image.height-1);const double tx=fx-x0,ty=fy-y0;
 auto at=[&](int xx,int yy){return image.rgba[(std::size_t(yy)*image.width+xx)*4+3];};
 return float((1-ty)*((1-tx)*at(x0,y0)+tx*at(x1,y0))+ty*((1-tx)*at(x0,y1)+tx*at(x1,y1)));
}
// Cache the immutable before-image samples when an effect opts into area reduction.
struct PreviewAreaCache {
    ProxyImage geometry;
    std::vector<std::array<float, 4>> pixels;

    template<class Image> void prepare(const Image& source, const PreviewSpace& space, const ProxyImage& proxy) {
        if (!pixels.empty() && geometry.width == proxy.width && geometry.height == proxy.height &&
            geometry.origin_x == proxy.origin_x && geometry.origin_y == proxy.origin_y && geometry.step == proxy.step) return;
        std::vector<std::array<float, 4>> next(std::size_t(proxy.width) * proxy.height);
        for (int y = 0; y < proxy.height; ++y) {
            const double top = double(proxy.origin_y) + y * proxy.step;
            for (int x = 0; x < proxy.width; ++x) {
                const double left = double(proxy.origin_x) + x * proxy.step;
                next[std::size_t(y) * proxy.width + x] = sample_area(source, space.x(left), space.y(top),
                    space.x(left + proxy.step), space.y(top + proxy.step));
            }
        }
        pixels = std::move(next);
        geometry = proxy;
    }
};

// Shared native-coordinate composition for spatial-filter previews. Host reads
// and texture uploads remain on the UI thread, including selection/background.
template<class Image>bool compose_spatial_preview(LibraryPanel& ui,FilterRecord* host,PreviewView& view,ProxyImage& proxy,PreviewBackground& background,std::vector<float>& selection,ProxyImage& selection_geometry,std::vector<std::uint8_t>& display,const Image& source,const Image& last,const PreviewSpace& mapping,bool enabled,bool& display_dirty,OSErr& error,bool* approximate=nullptr,bool alpha_mask=false,PreviewAreaCache* area_cache=nullptr){
 if(!display_dirty&&!view.dirty)return true;
 const auto roi=view.region();const int step=std::max(1,int(std::ceil(double(std::max(roi.right-roi.left,roi.bottom-roi.top))/view.proxy_side())));
 const int w=std::max(1,(roi.right-roi.left+step-1)/step),h=std::max(1,(roi.bottom-roi.top+step-1)/step);
 proxy.width=w;proxy.height=h;proxy.origin_x=roi.left;proxy.origin_y=roi.top;proxy.step=step;
 if(host->haveMask&&(selection.empty()||selection_geometry.width!=w||selection_geometry.height!=h||selection_geometry.origin_x!=roi.left||selection_geometry.origin_y!=roi.top||selection_geometry.step!=step)){
  auto* channel=host->documentInfo?host->documentInfo->selection:nullptr;
  if(channel&&!host_read_preview_channel(host,channel,proxy,selection)){selection_geometry=proxy;if(approximate)*approximate=false;}else{selection.clear();if(approximate)*approximate=true;}
 }
 background.prepare(host,proxy);display.resize(std::size_t(w)*h*3);const bool effect=enabled&&last.valid();double source_step=1;if constexpr(requires{source.step;})source_step=source.step;const PreviewSpace original_space{double(source.left),double(source.top),source_step};
 const bool area_sampled=area_cache&&step>1;
 if(area_sampled)area_cache->prepare(source,original_space,proxy);
 const bool locked=host->filterCase==filterCaseProtectedTransparencyNoSelection||host->filterCase==filterCaseProtectedTransparencyWithSelection;
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  const auto at=std::size_t(y)*w+x;
  const double xx=double(roi.left)+x*step,yy=double(roi.top)+y*step;
  auto before=area_sampled?area_cache->pixels[at]:preview_pixel(source,xx,yy,original_space),after=before;
  if(effect){
   if(alpha_mask)after[3]*=preview_alpha(last,xx,yy,mapping);
   else if(area_sampled)after=sample_area(last,mapping.x(xx),mapping.y(yy),mapping.x(xx+step),mapping.y(yy+step));
   else after=preview_pixel(last,xx,yy,mapping,!locked);
  }
  if(locked)after[3]=before[3];
  if(!host->outTransparencyMask){for(int k=0;k<3;k++){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto color=background.pixel(x,y,before,after,selection.empty()?1:selection[at]);for(int k=0;k<3;k++)display[at*3+k]=color[k];
 }
 if(!ui.preview(host,display.data(),w,h)){error=memFullErr;return false;}display_dirty=view.dirty=false;return true;
}
inline ProxyImage adaptive_geometry(const PreviewView& view,PreviewPass pass){
 const auto r=view.region();const int limit=pass.draft?std::min(pass.side,view.proxy_side()):view.proxy_side();
 const int step=std::max(1,int(std::ceil(double(std::max(r.right-r.left,r.bottom-r.top))/limit)));
 ProxyImage out;out.width=std::max(1,(r.right-r.left+step-1)/step);out.height=std::max(1,(r.bottom-r.top+step-1)/step);out.origin_x=r.left;out.origin_y=r.top;out.step=step;return out;
}
inline ProxyImage adaptive_geometry(const PreviewView& view,bool draft){return adaptive_geometry(view,PreviewPass{draft,512});}
} // namespace psui
