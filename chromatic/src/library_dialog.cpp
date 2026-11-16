#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "chromatic_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "psui/adaptive_preview.hpp"
#include <algorithm>
#include <cmath>
namespace chromatic {
namespace {
constexpr const char* presets[]={"自定义 / Custom","镜头色差 / Lens Aberration","RGB 错位 / RGB Split","棱镜色散 / Prism Dispersion","玻璃高光 / Glass Highlights","光谱缩放 / Spectral Zoom","光谱旋转 / Spectral Spin","棱镜光环 / Prism Rings","六边形散景 / Hex Bokeh"};
constexpr const char* modes[]={"RGB 分离 / RGB Separation","连续光谱 / Continuous Spectrum","光谱变换 / Spectral Transform","棱镜散焦 / Prism Defocus"};
constexpr const char* apertures[]={"圆形 / Circle","三角形 / Triangle","四边形 / Square","五边形 / Pentagon","六边形 / Hexagon","七边形 / Heptagon","八边形 / Octagon"};
constexpr const char* defocus_qualities[]={"快速 / Fast","标准 / Standard","精细 / Fine"};
constexpr const char* geometries[]={"直线 / Directional","径向 / Radial","切向 / Tangential"};
constexpr const char* edges[]={"透明 / Transparent","延伸 / Repeat","镜像 / Reflect","循环 / Wrap"};
constexpr const char* alphas[]={"扩展色散透明度 / Expand","保留源透明度 / Preserve Source","黑底不透明 / Opaque on Black"};
constexpr const char* masks[]={"Alpha 通道 / Alpha Channel","亮度 / Luminance"};
constexpr const char* channels[]={"红色通道 / Red Channel","绿色通道 / Green Channel","蓝色通道 / Blue Channel"};
constexpr const char* labels[]={
 "预设 / Preset","基本 / Main","模式 / Mode","方向类型 / Direction Type","色差距离（像素） / Separation (px)","方向角度 / Direction Angle","反转色散 / Reverse Dispersion","效果混合（%） / Effect Mix (%)",
 "中心与衰减 / Center and Falloff","中心 X（%） / Center X (%)","中心 Y（%） / Center Y (%)","居中 / Center","启用衰减 / Enable Falloff","中心保护（%） / Center Protection (%)","衰减曲线 / Falloff Curve","反转衰减 / Invert Falloff",
 "光谱 / Spectrum","光谱采样 / Spectrum Samples","色散曲线 / Dispersion Curve","光谱色相 / Spectrum Hue","光谱饱和度（%） / Spectrum Saturation (%)","柔化（像素） / Softness (px)",
 "距离系数 / Distance Factor","偏移 X / Offset X","偏移 Y / Offset Y","缩放（%） / Scale (%)","旋转 / Rotation","模糊（像素） / Blur (px)",
 "高光 / Highlights","仅处理高光 / Highlights Only","高光阈值（%） / Highlight Threshold (%)","阈值平滑（%） / Threshold Smoothness (%)","线性光插值 / Linear-Light Interpolation","工作伽马 / Working Gamma",
 "边缘与透明度 / Edges and Alpha","边缘处理 / Edge Handling","透明度 / Alpha Handling","遮罩 / Mask","遮罩图层 / Mask Layer","遮罩模式 / Mask Mode","反转遮罩 / Invert Mask","无 / None",
 "计算预览中… / Rendering preview…","预览读取失败，请重置或取消。 / Preview failed. Reset or cancel.","显示中心 / Show Center",
 "右键缩览图设置中心。 / Right-click the preview to set the center.",
 "光谱模式使用 RGB 近似，不是完整光学光谱模拟。 / Spectrum mode is an RGB approximation, not a full optical simulation.",
 "输出受 PS 画布和选区范围限制。 / Output is limited to Photoshop canvas and selection bounds.",
 "遮罩图层名称不唯一或无法读取。 / Mask name is ambiguous or the layer is unreadable."};
enum Label {preset_label,main_section,mode_label,geometry_label,amount_label,angle_label,reverse_label,mix_label,
 center_section,center_x_label,center_y_label,center_label,falloff_label,protect_label,power_label,invert_falloff_label,
 spectrum_section,samples_label,dispersion_label,hue_label,saturation_label,softness_label,
 factor_label,x_label,y_label,scale_label,rotation_label,blur_label,
 highlights_section,highlights_label,threshold_label,knee_label,linear_label,gamma_label,
 edges_section,edge_label,alpha_label,mask_section,mask_layer_label,mask_mode_label,invert_mask_label,none_label,
 working_label,error_label,show_center_label,pick_center_label,approximation_label,bounds_label,mask_error_label};
#include "preview_adapter.hpp"
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;
 Image mask,last;psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<float> selection;bool selection_approximate=false;
 std::vector<std::uint8_t> display;bool preview=true,dirty=true,display_dirty=true,show_center=false;
 int preset=0;OSErr error=noErr;ULONGLONG update_at=0;
  psui::PreviewSpace adaptive_mapping;psui::DraftImageCache<Image> adaptive_cache;
 psui::ProxyImage adaptive_selection_geometry;std::vector<float> adaptive_selection;
 psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters values,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& ls):p(values),host(f),source(src),layers(ls){for(int i=0;i<4;++i)if(p==chromatic::preset(i))preset=i+1;}
 bool read_mask(){
  worker.stop();adaptive_cache.clear();mask={};if(!p.mask_key)return true;const psui::HostLayer* selected=nullptr;
  for(const auto& l:layers)if(l.name_key==p.mask_key){if(selected)return false;selected=&l;}
  if(!selected)return false;psui::LayerPixels pixels;
  if(psui::host_read_layer(host,*selected,host->documentInfo->bounds,0,pixels))return false;
  mask=from_host(std::move(pixels));return true;
 }
};
bool number(psui::LibraryPanel& ui,int label,double& value,double lo,double hi,double sliding_lo,double sliding_hi,bool enabled=true,bool integer=false){return ui.number(labels[label],value,lo,hi,sliding_lo,sliding_hi,enabled,integer);}
bool choice(psui::LibraryPanel& ui,int label,int& value,const char*const* values,int count){return ui.choice(labels[label],value,values,count);}
bool mask_control(psui::LibraryPanel& ui,Context& c){
 const char* selected=labels[none_label];for(const auto& l:c.layers)if(l.name_key==c.p.mask_key)selected=l.name.c_str();bool changed=false;
 changed|=ui.parameter_label(labels[mask_layer_label],c.p.mask_key);ImGui::SetNextItemWidth(-1);
 if(ImGui::BeginCombo("##mask-layer",ui.text(selected).c_str())){
  if(ImGui::Selectable(ui.text(labels[none_label]).c_str(),c.p.mask_key==0)){c.p.mask_key=0;changed=true;}
  for(std::size_t i=0;i<c.layers.size();++i){ImGui::PushID(int(i));if(ImGui::Selectable(c.layers[i].name.c_str(),c.p.mask_key==c.layers[i].name_key)){c.p.mask_key=c.layers[i].name_key;changed=true;}ImGui::PopID();}
  ImGui::EndCombo();
 }ui.mark("mask.layer");if(changed)c.error=c.read_mask()?noErr:filterBadParameters;return changed;
}
void update(psui::LibraryPanel& ui,Context& c){
 try{
  const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(!c.error&&c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass draft,const std::function<bool()>& cancel){
   psui::SpatialFrame<Image> frame;const auto space=psui::preview_space(c.source,draft);frame.space=space;
   const auto& source=c.adaptive_cache.get(c.source,space,cancel);const auto& mask=c.adaptive_cache.get(c.mask,space,cancel);
   if(cancel())return frame;const auto q=space.step>1?draft_parameters(p,space):p;
   render(source,q,frame.image,mask.valid()?&mask:nullptr,cancel);
   return frame;
  })){
   if(frame.image.valid()){c.last=std::move(frame.image);c.adaptive_mapping=frame.space; c.display_dirty=true;}else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.adaptive_selection,c.adaptive_selection_geometry,c.display,c.source,c.last,c.adaptive_mapping,c.preview,c.display_dirty,c.error,&c.selection_approximate);
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, chromatic::preset(2));
 ui.inherit_choice_default(labels[mode_label],c.p.mode);
 ui.inherit_choice_default(labels[geometry_label],c.p.geometry);
 ui.inherit_choice_default(labels[edge_label],c.p.edge);
 ui.inherit_choice_default(labels[alpha_label],c.p.alpha);
 const float s=ui.scale();bool changed=false,preset_changed=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-ImGui::GetFrameHeight()-ImGui::GetStyle().ItemSpacing.y-190*s,100*s,205*s));
 update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["chromatic.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");
 if(p.mode!=Mode::defocus&&ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
  const auto mouse=ImGui::GetMousePos();const double x=c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01),y=c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01);
  const double nx=(x-c.source.left)/c.source.width,ny=(y-c.source.top)/c.source.height;
  if(nx>=0&&nx<=1&&ny>=0&&ny<=1){p.center_x=nx;p.center_y=ny;changed=true;}
 }
 if(c.show_center&&p.mode!=Mode::defocus){
  const float x=float((rect.x+rect.z)*.5+(c.source.left+p.center_x*c.source.width-c.view.center_x())*c.view.percent*.01);
  const float y=float((rect.y+rect.w)*.5+(c.source.top+p.center_y*c.source.height-c.view.center_y())*c.view.percent*.01);
  auto* list=ImGui::GetWindowDrawList();list->PushClipRect({rect.x,rect.y},{rect.z,rect.w},true);list->AddLine({x-6*s,y},{x+6*s,y},IM_COL32(85,190,255,255),s);list->AddLine({x,y-6*s},{x,y+6*s},IM_COL32(85,190,255,255),s);list->PopClipRect();
 }
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);
 if(psui::preview_background_control(ui,c.background))c.display_dirty=true;
 ui.preview_zoom(c.view);if(p.mode!=Mode::defocus)ui.check(labels[show_center_label],c.show_center);

 if(c.selection_approximate)ImGui::TextDisabled("%s",ui.text("选区缩览为矩形近似 / Rectangular selection preview").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[error_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();c.adaptive_cache.clear();p=chromatic::preset(2);c.preset=3;c.mask={};c.error=noErr;changed=preset_changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(choice(ui,preset_label,c.preset,presets,9)&&c.preset){c.worker.stop();c.adaptive_cache.clear();p=chromatic::preset(c.preset-1);c.mask={};c.error=noErr;changed=preset_changed=true;}
 if(ui.section(labels[main_section],true)){
  int mode=int(p.mode),geometry=int(p.geometry);if(choice(ui,mode_label,mode,modes,4)){p.mode=Mode(mode);changed=true;}
  if(int(p.mode)<2){if(choice(ui,geometry_label,geometry,geometries,3)){p.geometry=Geometry(geometry);changed=true;}
  changed|=number(ui,amount_label,p.amount,-4096,4096,0,200);changed|=number(ui,angle_label,p.angle,-360000,360000,-180,180);}
  if(p.mode==Mode::warp)changed|=ui.number("变换强度（%） / Transform Amount (%)",p.warp_amount,0,200,0,100);
  if(p.mode==Mode::defocus)changed|=ui.number("散焦半径（像素） / Defocus Radius (px)",p.defocus_radius,0,1024,0,100);
  if(p.mode!=Mode::defocus)changed|=ui.check(labels[reverse_label],p.reverse);changed|=number(ui,mix_label,p.mix,0,100,0,100);
 }
 if(p.mode!=Mode::defocus&&ui.section(labels[center_section],false)){
  ui.number_default(labels[center_x_label],chromatic::preset(2).center_x*100);ui.number_default(labels[center_y_label],chromatic::preset(2).center_y*100);
  double x=p.center_x*100,y=p.center_y*100;
  if(number(ui,center_x_label,x,0,100,0,100)){p.center_x=x*.01;changed=true;}
  if(number(ui,center_y_label,y,0,100,0,100)){p.center_y=y*.01;changed=true;}
  if(ui.button("center",labels[center_label],-1)){p.center_x=p.center_y=.5;changed=true;}
  ImGui::TextDisabled("%s",ui.text(labels[pick_center_label]).c_str());
  changed|=ui.check(labels[falloff_label],p.falloff);
  if(p.falloff){changed|=number(ui,protect_label,p.protect,0,99.9,0,95);changed|=number(ui,power_label,p.falloff_power,.1,8,.1,4);changed|=ui.check(labels[invert_falloff_label],p.invert_falloff);}
 }
 if(p.mode==Mode::spectrum||p.mode==Mode::warp){if(ui.section(labels[spectrum_section],true)){
   ui.inherit_number_default(labels[samples_label],p.samples);double samples=p.samples;if(number(ui,samples_label,samples,8,64,8,64,true,true)){p.samples=int(samples);changed=true;}
   changed|=number(ui,dispersion_label,p.dispersion_power,.5,4,.5,4);changed|=number(ui,hue_label,p.hue,-360000,360000,0,360);
   changed|=number(ui,saturation_label,p.saturation,0,200,0,200);changed|=number(ui,softness_label,p.softness,0,50,0,10);
  }}else if(p.mode==Mode::rgb)for(int k=0;k<3;++k){ImGui::PushID(k);if(ui.section(channels[k],false)){
    auto& channel=p.channels[k];changed|=number(ui,factor_label,channel.amount,-4,4,-2,2);changed|=number(ui,x_label,channel.x,-4096,4096,-100,100);changed|=number(ui,y_label,channel.y,-4096,4096,-100,100);
    changed|=number(ui,scale_label,channel.scale,1,1000,90,110);changed|=number(ui,rotation_label,channel.rotation,-360000,360000,-10,10);changed|=number(ui,blur_label,channel.blur,0,50,0,10);
   }ImGui::PopID();}
 if(p.mode==Mode::warp){
  const char* sections[]={"红端变换 / Red-End Transform","蓝端变换 / Blue-End Transform"};
  for(int i=0;i<2;i++){ImGui::PushID(30+i);if(ui.section(sections[i],true)){auto& t=p.warp[i];
   changed|=number(ui,scale_label,t.scale,1,1000,50,150);changed|=number(ui,rotation_label,t.rotation,-360000,360000,-45,45);
   changed|=number(ui,x_label,t.x,-4096,4096,-100,100);changed|=number(ui,y_label,t.y,-4096,4096,-100,100);
  }ImGui::PopID();}
 }
 if(p.mode==Mode::defocus&&ui.section("光圈与色环 / Aperture and Color Rings",true)){
  ui.inherit_choice_default("光圈形状 / Aperture Shape",p.aperture_shape);changed|=ui.choice("光圈形状 / Aperture Shape",p.aperture_shape,apertures,7);
  changed|=ui.number("环宽（%） / Ring Width (%)",p.ring_width,1,100,1,100);
  changed|=ui.number("光圈旋转 / Aperture Rotation",p.aperture_rotation,-360000,360000,-180,180);
  changed|=ui.number("光圈长宽比 / Aperture Aspect",p.aperture_aspect,.1,10,.25,4);
  changed|=ui.number("光圈圆润度（%） / Aperture Roundness (%)",p.aperture_roundness,0,100,0,100,p.aperture_shape!=0);
  const char* colors[]={"红色半径（%） / Red Radius (%)","绿色半径（%） / Green Radius (%)","蓝色半径（%） / Blue Radius (%)"};
  for(int i=0;i<3;i++)changed|=ui.number(colors[i],p.defocus_radii[i],0,400,0,200);
  ui.inherit_choice_default("散焦质量 / Defocus Quality",p.defocus_quality);changed|=ui.choice("散焦质量 / Defocus Quality",p.defocus_quality,defocus_qualities,3);
 }
 if(ui.section(labels[highlights_section],false)){
  changed|=ui.check(labels[highlights_label],p.highlights_only);
  if(p.highlights_only){changed|=number(ui,threshold_label,p.threshold,0,1000,0,100);changed|=number(ui,knee_label,p.knee,0,100,0,100);}
  changed|=ui.check(labels[linear_label],p.linear_light);if(p.linear_light)changed|=number(ui,gamma_label,p.gamma,1,3,1,3);
 }
 if(ui.section(labels[edges_section],false)){
  int edge=int(p.edge),alpha=int(p.alpha);if(choice(ui,edge_label,edge,edges,4)){p.edge=Edge(edge);changed=true;}if(choice(ui,alpha_label,alpha,alphas,3)){p.alpha=Alpha(alpha);changed=true;}
 }
 if(ui.section(labels[mask_section],false)){
  changed|=mask_control(ui,c);if(p.mask_key){changed|=choice(ui,mask_mode_label,p.mask_mode,masks,2);changed|=ui.check(labels[invert_mask_label],p.invert_mask);}
 }
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[approximation_label]).c_str());ImGui::TextWrapped("%s",ui.text(labels[bounds_label]).c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[mask_error_label]).c_str());
 ImGui::EndChild();
 if(changed){if(!p.mask_key||c.mask.valid())c.error=noErr;if(!preset_changed)c.preset=0;c.dirty=true;c.update_at=GetTickCount64()+50;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,const Image& source,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& mask){
 Context c(p,host,source,layers);if(!c.read_mask())c.error=filterBadParameters;
 std::string glyphs;for(auto label:labels)glyphs+=label;
 glyphs+="变换强度 红端变换 蓝端变换 散焦半径 像素 光圈与色环 光圈形状 环宽 光圈旋转 光圈长宽比 光圈圆润度 红色半径 绿色半径 蓝色半径 散焦质量";
 auto add=[&](const auto& list){for(auto text:list)glyphs+=text;};add(apertures);add(defocus_qualities);add(presets);add(modes);add(geometries);add(edges);add(alphas);add(masks);add(channels);for(const auto& layer:layers)glyphs+=layer.name;
 const auto answer=psui::run_library_panel(module,owner,host,title,815,620,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);
 c.worker.stop();c.adaptive_cache.clear();if(answer==IDOK&&!c.error){p=c.p;mask=std::move(c.mask);}return answer;
}
}
