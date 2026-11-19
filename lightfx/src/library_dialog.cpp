#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "lightfx_host.hpp"
#include "lightfx_render_cache.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/coalesced_preview.hpp"
#include "psui/adaptive_preview.hpp"
#include "psui/preview_point.hpp"
#include "psui/preview_background.hpp"
#include <chrono>
#include <cstdio>
namespace lightfx {namespace {
constexpr const char* inputs[]={"明度 / Lightness","亮度 / Luminance","红色 / Red","绿色 / Green","蓝色 / Blue","透明度 / Alpha"};
constexpr const char* modes[]={"单色 / One Color","三色渐变 / 3 Colors","五色渐变 / 5 Colors"};
constexpr const char* blends[]={"相加 / Add","滤色 / Screen","合成 / Composite"};
constexpr const char* quality[]={"标准（32） / Standard (32)","精细（64） / Fine (64)","最高（128） / Highest (128)"};
constexpr const char* map_names[]={"色谱 A / Map A","色谱 B / Map B","色谱 C / Map C"};
constexpr const char* directions[]={"右 / Right","右下 / Down Right","下 / Down","左下 / Down Left","左 / Left","左上 / Up Left","上 / Up","右上 / Up Right"};
constexpr const char* stars[]={"八角白星 / Eight-Point White","十字星 / Cross","横向拉丝 / Horizontal","彩虹星芒 / Rainbow","金色星芒 / Gold"};
constexpr const char* shines[]={"白色光束 / White Rays","暖色光束 / Warm Rays","冷色光束 / Cool Rays","原色透光 / Source Color","纹理光束 / Textured Rays"};
struct Frame{Image image;double milliseconds=0;int side=512;};
struct Cache {
 const Image& source;Image small;std::shared_ptr<const Image> emission,small_emission;Parameters extraction;bool extracted=false;int small_side=0,seed_side=0;
 RenderCache rays;
 explicit Cache(const Image& src):source(src){}
 Frame draw(const Parameters& p,bool draft,int side,const Cancel& cancel){
  const auto start=std::chrono::steady_clock::now();
  if(!extracted||!same_extraction(p,extraction)){
   auto next=prepare(source,p,cancel);if(!next)return {};
   rays.clear();emission=std::move(next);small_emission.reset();extraction=p;extracted=true;
  }
  if(draft&&(!small.valid()||small_side!=side)){rays.clear();small=downsample(source,side,cancel);if(!small.valid())return {};small_side=side;}
  if(draft&&(!small_emission||seed_side!=side)){auto reduced=downsample(*emission,side,cancel,true);if(!reduced.valid())return {};rays.clear();small_emission=std::make_shared<Image>(std::move(reduced));seed_side=side;}
  Frame result;result.side=side;const auto& src=draft?small:source;const auto& seed=draft?*small_emission:*emission;
  if(!rays.render(src,seed,p,result.image,cancel))return {};
  result.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();return result;
 }
};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;psui::PreviewView view;
 psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool display_dirty=true,preview=true,fast=true,last_draft=true,need_request=true,need_final=true;
 OSErr error=noErr;ULONGLONG settle=0;std::uint64_t revision=0;double milliseconds=0;int frames=0,full_frames=0,preset_index=0;
 std::shared_ptr<Cache> cache;psui::CoalescedPreview<Frame> worker;psui::DraftResolution resolution;
 Context(Parameters values,FilterRecord* f,const Image& src):p(values),host(f),source(src),cache(std::make_shared<Cache>(src)){}
};
void request(Context& c,bool draft){
 const auto p=c.p;const auto cache=c.cache;const int side=c.resolution.side();c.revision=c.worker.submit(draft,[p,cache,draft,side](const Cancel& cancel){return cache->draw(p,draft,side,cancel);});c.need_request=false;c.need_final=draft;
}
void update(psui::LibraryPanel& ui,Context& c){
 try{Frame frame;bool draft=false;std::uint64_t revision=0;if(c.worker.take(frame,draft,revision)&&frame.image.valid()){
  if(draft)c.resolution.observe(frame.milliseconds,frame.side);c.last=std::move(frame.image);c.last_draft=draft;c.milliseconds=frame.milliseconds;c.display_dirty=true;++c.frames;if(!draft)++c.full_frames;
 }}catch(...){c.error=memFullErr;}
 if(c.need_request){request(c,c.fast);c.settle=GetTickCount64()+180;}
 else if(c.need_final&&GetTickCount64()>=c.settle){request(c,false);}
 if(!c.display_dirty&&!c.view.dirty)return;
 const auto roi=c.view.region();const int step=std::max(1,int(std::ceil(double(std::max(roi.right-roi.left,roi.bottom-roi.top))/c.view.proxy_side())));
 const int w=std::max(1,(roi.right-roi.left+step-1)/step),h=std::max(1,(roi.bottom-roi.top+step-1)/step);
 c.proxy.width=w;c.proxy.height=h;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 if(c.host->haveMask&&(!c.selection.size()||c.selection_geometry.width!=w||c.selection_geometry.height!=h||c.selection_geometry.origin_x!=roi.left||c.selection_geometry.origin_y!=roi.top||c.selection_geometry.step!=step)){
  auto* channel=c.host->documentInfo?c.host->documentInfo->selection:nullptr;
  if(channel&&!psui::host_read_preview_channel(c.host,channel,c.proxy,c.selection))c.selection_geometry=c.proxy;else c.selection.clear();
 }
 c.background.prepare(c.host,c.proxy);c.display.resize(std::size_t(w)*h*3);const auto& result=c.preview&&c.last.valid()?c.last:c.source;
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  const double docx=roi.left+(x+.5)*step,docy=roi.top+(y+.5)*step;
  auto before=psui::preview_pixel(c.source,docx-.5,docy-.5,psui::PreviewSpace{double(c.source.left),double(c.source.top),double(c.source.step)});
  auto after=psui::preview_pixel(result,docx-.5,docy-.5,psui::PreviewSpace{double(result.left),double(result.top),double(result.step)},c.host->filterCase!=filterCaseProtectedTransparencyNoSelection&&c.host->filterCase!=filterCaseProtectedTransparencyWithSelection);
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  if(!c.host->outTransparencyMask){for(int k=0;k<3;k++){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto color=c.background.pixel(x,y,before,after,c.selection.empty()?1:c.selection[std::size_t(y)*w+x]);for(int k=0;k<3;k++)c.display[(std::size_t(y)*w+x)*3+k]=color[k];
 }
 if(!ui.preview(c.host,c.display.data(),w,h))c.error=memFullErr;c.display_dirty=c.view.dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 const auto b=c.host->documentInfo->bounds;const auto factory=defaults(effect_kind,b.right-b.left,b.bottom-b.top,b.left,b.top);ui.parameter_defaults(c.p,factory);auto& p=c.p;const float s=ui.scale();bool changed=false;
 ImGui::BeginChild("preview-side",{300*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);c.view.configure(psui::filter_document_bounds(c.host),300*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-190*s,140*s,350*s));
 update(ui,c);ui.image_view(c.view,c.proxy);if(effect_kind==Kind::shine)changed|=psui::preview_point(ui,c.view,p.cx,p.cy);else if(p.use_mask)changed|=psui::preview_point(ui,c.view,p.mask_cx,p.mask_cy);
 ui.items["lightfx.state"]={float(c.frames),float(c.full_frames),c.last_draft?1.f:0.f,float(c.error)};ui.items["lightfx.timing"]={float(c.milliseconds),float(c.last.step),c.worker.pending()?1.f:0.f,0};
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;
 if(ui.check("交互快速预览 / Fast Interactive Preview",c.fast)){c.need_request=true;c.display_dirty=true;}
 if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ImGui::TextDisabled("%s",ui.text(c.last_draft?"交互预览；停止后补全质量 / Draft; full quality after interaction":"完整质量预览 / Full Quality Preview").c_str());
 if(c.last.valid())ImGui::TextDisabled("%.1f ms",c.milliseconds);

 if(effect_kind==Kind::shine||p.use_mask)ImGui::TextWrapped("%s",ui.text("按住右键拖动中心；左键平移。 / Right-drag the center; left-drag to pan.").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text("计算失败，请全部重置或取消。 / Render failed. Reset all or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.invalidate();p=factory;c.error=noErr;c.preset_index=0;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 auto number=[&](const char* label,double& value,double lo,double hi,double slo,double shi){changed|=ui.number(label,value,lo,hi,slo,shi);};
 if(ui.choice("预设 / Preset",c.preset_index,effect_kind==Kind::starglow?stars:shines,5)){p=preset(p,c.preset_index);changed=true;}
 if(ui.section("高光提取 / Highlight Extraction",true)){
  changed|=ui.choice("输入通道 / Input Channel",p.input,inputs,6);number("阈值 / Threshold",p.threshold,0,16,0,1);number("阈值柔和 / Threshold Softness",p.softness,0,16,0,1);
  changed|=ui.check("圆形范围 / Circular Mask",p.use_mask);if(p.use_mask){changed|=psui::point_control(ui,"范围中心 / Mask Center",p.mask_cx,p.mask_cy);number("范围半径 / Mask Radius",p.mask_radius,0,1e7,0,1000);number("范围羽化 / Mask Feather",p.mask_feather,0,1e7,0,500);}
 }
 if(ui.section(effect_kind==Kind::starglow?"星芒 / Starglow":"放射光束 / Shine",true)){
  if(effect_kind==Kind::shine)changed|=psui::point_control(ui,"光源中心 / Source Point",p.cx,p.cy);
  number(effect_kind==Kind::starglow?"星芒长度（像素） / Streak Length (px)":"光束长度 / Ray Length",p.length,0,effect_kind==Kind::starglow?10000:10,0,effect_kind==Kind::starglow?300:3);
  number("光照增强 / Boost Light",p.boost,0,100,0,10);number("衰减 / Falloff",p.falloff,.05,20,.05,5);
  if(effect_kind==Kind::starglow)number("旋转（度） / Rotation (deg)",p.rotation,-360000,360000,-180,180);
  else{int q=p.samples==32?0:p.samples==64?1:2;ui.choice_default("采样质量 / Quality",1);if(ui.choice("采样质量 / Quality",q,quality,3)){p.samples=32<<q;changed=true;}}
 }
 if(effect_kind==Kind::starglow&&ui.section("各方向长度 / Individual Lengths",false)){ImGui::PushID("direction-lengths");for(int i=0;i<8;i++)number(directions[i],p.lengths[i],0,1000,0,200);ImGui::PopID();}
 if(ui.section("着色 / Colorize",false)){
  changed|=ui.check("保留源颜色 / Use Source Color",p.source_color);
  if(!p.source_color){changed|=ui.choice("色谱类型 / Color Map Type",p.map_mode,modes,3);
   for(int m=0;m<(effect_kind==Kind::starglow?3:1);m++){ImGui::PushID(m);if(ui.section(map_names[m],m==0)){
    constexpr const char* labels[]={"阴影 / Shadows","中低调 / Mid Low","中间调 / Midtones","中高调 / Mid High","高光 / Highlights"};for(int j=0;j<5;j++)if(p.map_mode==2||(p.map_mode==1&&j%2==0)||(p.map_mode==0&&j==4))changed|=psui::color_control(ui,c.host,labels[j],p.colors[m][j].data());
   }ImGui::PopID();}
   if(effect_kind==Kind::starglow&&ui.section("各方向色谱 / Individual Colors",false)){ImGui::PushID("direction-colors");for(int i=0;i<8;i++)changed|=ui.choice(directions[i],p.maps[i],map_names,3);ImGui::PopID();}
  }
 }
 if(ui.section("闪烁纹理 / Shimmer",false)){
  number("纹理数量（%） / Shimmer Amount (%)",p.shimmer,0,100,0,100);number("纹理细节 / Detail",p.detail,.1,1000,.1,200);number("变化相位（度） / Phase (deg)",p.phase,-1e9,1e9,0,720);
  changed|=ui.check("循环变化 / Loop",p.cycle);number("循环转数 / Revolutions",p.loops,1,100,1,10);
  double seed=p.seed;ui.inherit_number_default("随机种子 / Random Seed",p.seed);if(ui.number("随机种子 / Random Seed",seed,0,4294967295.,0,1000,true,true)){p.seed=std::uint32_t(seed);changed=true;}
  changed|=psui::point_control(ui,"纹理偏移 / Noise Offset",p.noise_x,p.noise_y);
 }
 if(ui.section("输出 / Output",true)){
  changed|=ui.choice("混合模式 / Blend Mode",p.blend,blends,3);number("源不透明度（%） / Source Opacity (%)",p.source_opacity,0,100,0,100);number("光效不透明度（%） / Effect Opacity (%)",p.effect_opacity,0,100,0,100);number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);changed|=ui.check("保留源透明度 / Preserve Source Alpha",p.preserve_alpha);
 }
 ImGui::TextWrapped("%s",ui.text("独立实现；快速预览不改变最终输出质量。 / Independent implementation; fast preview does not change final quality.").c_str());
 ImGui::EndChild();if(changed){c.need_request=true;c.settle=GetTickCount64()+180;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const auto result=psui::run_library_panel(m,owner,host,title,1080,820,[&](psui::LibraryPanel& ui){draw(ui,c);},
 "星芒 放射光束 高光提取 输入通道 明度 亮度 红色 绿色 蓝色 透明度 阈值 柔和 圆形范围 范围中心 半径 羽化 光源中心 长度 像素 光照增强 衰减 旋转 度 采样质量 标准 精细 最高 各方向长度 右 右下 下 左下 左 左上 上 右上 着色 保留源颜色 色谱类型 单色 三色渐变 五色渐变 色谱 阴影 中低调 中间调 中高调 高光 各方向色谱 闪烁纹理 数量 细节 变化相位 循环变化 循环转数 随机种子 纹理偏移 输出 混合模式 相加 滤色 合成 源不透明度 光效不透明度 效果混合 保留源透明度 预设 八角白星 十字星 横向拉丝 彩虹 金色 白色 暖色 冷色 原色透光 交互快速预览 停止后补全质量 完整质量预览 正在计算 按住右键拖动中心 左键平移 独立实现 快速预览不改变最终输出质量 计算失败请全部重置或取消",&plugin_info);
 if(result==IDOK&&!c.error)p=c.p;return result;
}
} // namespace lightfx
