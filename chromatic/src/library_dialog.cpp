#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "chromatic_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "psui/async_preview.hpp"
#include <algorithm>
#include <cmath>
namespace chromatic {
namespace {
constexpr const char* presets[]={"自定义 / Custom","镜头色差 / Lens Aberration","RGB 错位 / RGB Split","棱镜色散 / Prism Dispersion","玻璃高光 / Glass Highlights"};
constexpr const char* modes[]={"RGB 分离 / RGB Separation","连续光谱 / Continuous Spectrum"};
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
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;
 Image mask,last;psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<float> selection;bool selection_approximate=false;
 std::vector<std::uint8_t> display;bool preview=true,dirty=true,display_dirty=true,show_center=false;
 int preset=0;OSErr error=noErr;ULONGLONG update_at=0;
 psui::AsyncPreview<Image> worker;
 Context(Parameters values,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& ls):p(values),host(f),source(src),layers(ls){for(int i=0;i<4;++i)if(p==chromatic::preset(i))preset=i+1;}
 bool read_mask(){
  worker.stop();mask={};if(!p.mask_key)return true;const psui::HostLayer* selected=nullptr;
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
 Image completed;try{if(c.worker.take(completed)){if(completed.valid()){c.last=std::move(completed);c.display_dirty=true;}else c.error=filterBadParameters;}}catch(...){c.error=memFullErr;}
 if(c.dirty&&GetTickCount64()>=c.update_at&&!c.error){
  const auto p=c.p;c.worker.start([&c,p](std::stop_token token){Image out;render(c.source,p,out,c.mask.valid()?&c.mask:nullptr,[token]{return token.stop_requested();});return out;});c.dirty=false;
 }
 if(!c.display_dirty&&!c.view.dirty)return;
 const auto roi=c.view.region();const int w=roi.right-roi.left,h=roi.bottom-roi.top;
 const int step=std::max(1,int(std::ceil(double(std::max(w,h))/c.view.proxy_side()))),pw=std::max(1,(w+step-1)/step),ph=std::max(1,(h+step-1)/step);
 c.proxy.width=pw;c.proxy.height=ph;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 if(c.host->haveMask){
  auto* channel=c.host->documentInfo?c.host->documentInfo->selection:nullptr;
  if(!channel){c.selection.clear();c.selection_approximate=true;}
  else if(c.selection_geometry.width!=pw||c.selection_geometry.height!=ph||c.selection_geometry.origin_x!=roi.left||c.selection_geometry.origin_y!=roi.top||c.selection_geometry.step!=step){
   const auto error=psui::host_read_preview_channel(c.host,channel,c.proxy,c.selection);
   if(error){c.selection.clear();c.selection_approximate=true;}else{c.selection_geometry=c.proxy;c.selection_approximate=false;}
  }
 }
 c.background.prepare(c.host,c.proxy);c.display.resize(std::size_t(pw)*ph*3);
 const auto& pixels=c.preview&&c.last.valid()?c.last:c.source;
 for(int y=0;y<ph;++y)for(int x=0;x<pw;++x){
  const int xx=std::clamp(roi.left+x*step-pixels.left,0,pixels.width-1),yy=std::clamp(roi.top+y*step-pixels.top,0,pixels.height-1);const auto i=(std::size_t(yy)*pixels.width+xx)*4;
  std::array<float,4> before{},after{};for(int k=0;k<4;++k){before[k]=c.source.rgba[i+k];after[k]=pixels.rgba[i+k];}
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  // RGB-only documents show the same black flattening as the native adapter.
  if(!c.host->outTransparencyMask){for(int k=0;k<3;++k){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const float selected=c.selection.empty()?1:std::clamp(c.selection[std::size_t(y)*pw+x],0.f,1.f);
  const auto color=c.background.pixel(x,y,before,after,selected);for(int k=0;k<3;++k)c.display[(std::size_t(y)*pw+x)*3+k]=color[k];
 }
 if(!ui.preview(c.host,c.display.data(),pw,ph))c.error=memFullErr;
 c.view.dirty=c.display_dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, chromatic::preset(2));
 ui.inherit_choice_default(labels[mode_label],c.p.mode);
 ui.inherit_choice_default(labels[geometry_label],c.p.geometry);
 ui.inherit_choice_default(labels[edge_label],c.p.edge);
 ui.inherit_choice_default(labels[alpha_label],c.p.alpha);
 const float s=ui.scale();bool changed=false,preset_changed=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-190*s,100*s,205*s));
 update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["chromatic.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");
 if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
  const auto mouse=ImGui::GetMousePos();const double x=c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01),y=c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01);
  const double nx=(x-c.source.left)/c.source.width,ny=(y-c.source.top)/c.source.height;
  if(nx>=0&&nx<=1&&ny>=0&&ny<=1){p.center_x=nx;p.center_y=ny;changed=true;}
 }
 if(c.show_center){
  const float x=float((rect.x+rect.z)*.5+(c.source.left+p.center_x*c.source.width-c.view.center_x())*c.view.percent*.01);
  const float y=float((rect.y+rect.w)*.5+(c.source.top+p.center_y*c.source.height-c.view.center_y())*c.view.percent*.01);
  auto* list=ImGui::GetWindowDrawList();list->PushClipRect({rect.x,rect.y},{rect.z,rect.w},true);list->AddLine({x-6*s,y},{x+6*s,y},IM_COL32(85,190,255,255),s);list->AddLine({x,y-6*s},{x,y+6*s},IM_COL32(85,190,255,255),s);list->PopClipRect();
 }
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;
 if(psui::preview_background_control(ui,c.background))c.display_dirty=true;
 ui.preview_zoom(c.view);ui.check(labels[show_center_label],c.show_center);
 if(c.worker.pending()||c.dirty)ImGui::TextDisabled("%s",ui.text(labels[working_label]).c_str());
 if(c.selection_approximate)ImGui::TextDisabled("%s",ui.text("选区缩览为矩形近似 / Rectangular selection preview").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[error_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=chromatic::preset(2);c.preset=3;c.mask={};c.error=noErr;changed=preset_changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(choice(ui,preset_label,c.preset,presets,5)&&c.preset){c.worker.stop();p=chromatic::preset(c.preset-1);c.mask={};c.error=noErr;changed=preset_changed=true;}
 if(ui.section(labels[main_section],true)){
  int mode=int(p.mode),geometry=int(p.geometry);if(choice(ui,mode_label,mode,modes,2)){p.mode=Mode(mode);changed=true;}
  if(choice(ui,geometry_label,geometry,geometries,3)){p.geometry=Geometry(geometry);changed=true;}
  changed|=number(ui,amount_label,p.amount,-4096,4096,0,200);changed|=number(ui,angle_label,p.angle,-360000,360000,-180,180);
  changed|=ui.check(labels[reverse_label],p.reverse);changed|=number(ui,mix_label,p.mix,0,100,0,100);
 }
 if(ui.section(labels[center_section],false)){
  ui.number_default(labels[center_x_label],chromatic::preset(2).center_x*100);ui.number_default(labels[center_y_label],chromatic::preset(2).center_y*100);
  double x=p.center_x*100,y=p.center_y*100;
  if(number(ui,center_x_label,x,0,100,0,100)){p.center_x=x*.01;changed=true;}
  if(number(ui,center_y_label,y,0,100,0,100)){p.center_y=y*.01;changed=true;}
  if(ui.button("center",labels[center_label],-1)){p.center_x=p.center_y=.5;changed=true;}
  ImGui::TextDisabled("%s",ui.text(labels[pick_center_label]).c_str());
  changed|=ui.check(labels[falloff_label],p.falloff);
  if(p.falloff){changed|=number(ui,protect_label,p.protect,0,99.9,0,95);changed|=number(ui,power_label,p.falloff_power,.1,8,.1,4);changed|=ui.check(labels[invert_falloff_label],p.invert_falloff);}
 }
 if(p.mode==Mode::spectrum){if(ui.section(labels[spectrum_section],true)){
   ui.inherit_number_default(labels[samples_label],p.samples);double samples=p.samples;if(number(ui,samples_label,samples,8,64,8,64,true,true)){p.samples=int(samples);changed=true;}
   changed|=number(ui,dispersion_label,p.dispersion_power,.5,4,.5,4);changed|=number(ui,hue_label,p.hue,-360000,360000,0,360);
   changed|=number(ui,saturation_label,p.saturation,0,200,0,200);changed|=number(ui,softness_label,p.softness,0,50,0,10);
  }}else for(int k=0;k<3;++k){ImGui::PushID(k);if(ui.section(channels[k],false)){
    auto& channel=p.channels[k];changed|=number(ui,factor_label,channel.amount,-4,4,-2,2);changed|=number(ui,x_label,channel.x,-4096,4096,-100,100);changed|=number(ui,y_label,channel.y,-4096,4096,-100,100);
    changed|=number(ui,scale_label,channel.scale,1,1000,90,110);changed|=number(ui,rotation_label,channel.rotation,-360000,360000,-10,10);changed|=number(ui,blur_label,channel.blur,0,50,0,10);
   }ImGui::PopID();}
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
 if(changed){c.worker.stop();if(!p.mask_key||c.mask.valid())c.error=noErr;if(!preset_changed)c.preset=0;c.dirty=true;c.update_at=GetTickCount64()+50;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,const Image& source,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& mask){
 Context c(p,host,source,layers);if(!c.read_mask())c.error=filterBadParameters;
 std::string glyphs;for(auto label:labels)glyphs+=label;
 auto add=[&](const auto& list){for(auto text:list)glyphs+=text;};add(presets);add(modes);add(geometries);add(edges);add(alphas);add(masks);add(channels);for(const auto& layer:layers)glyphs+=layer.name;
 const auto answer=psui::run_library_panel(module,owner,host,title,815,620,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);
 c.worker.stop();if(answer==IDOK&&!c.error){p=c.p;mask=std::move(c.mask);}return answer;
}
}
