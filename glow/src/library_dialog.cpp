#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "glow_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/async_preview.hpp"
#include "psui/preview_background.hpp"
#include <algorithm>
#include <cmath>
namespace glow {
namespace {
constexpr const char* blends[]={"相加 / Add","滤色 / Screen","正常 / Normal","变亮 / Lighten","正片叠底 / Multiply"};
constexpr const char* based[]={"颜色通道 / Color Channels","Alpha 通道 / Alpha Channel"};
constexpr const char* composites[]={"后面 / Behind","前面 / On Top","无（仅发光） / None (Glow Only)"};
constexpr const char* colors[]={"原始颜色 / Original Colors","颜色 A 和 B / A & B Colors"};
constexpr const char* loops[]={"三角形 A-B-A / Triangle A-B-A","锯齿 A-B / Sawtooth A-B","锯齿 B-A / Sawtooth B-A"};
constexpr const char* dimensions[]={"水平和垂直 / Horizontal and Vertical","水平 / Horizontal","垂直 / Vertical"};
constexpr const char* falloffs[]={"指数 / Exponential","宽尾 / Wide Tail"};
constexpr const char* thresholds[]={"亮度 / Luminance","最大颜色通道 / Max Color Channel","Alpha 通道 / Alpha Channel"};
constexpr const char* tints[]={"关闭 / Off","单色 / Single Color","内外双色 / Inner and Outer"};
constexpr const char* tones[]={"无 / None","柔和压缩 / Reinhard","曝光压缩 / Exponential"};
constexpr const char* qualities[]={"草稿 / Draft","中 / Medium","高 / High"};
constexpr const char* views[]={"最终结果 / Final Result","仅发光 / Glow Only","发光源 / Glow Source"};
constexpr const char* masks[]={"Alpha 通道 / Alpha Channel","亮度 / Luminance"};
// One source of labels also builds the compact Chinese font subset.
constexpr const char* labels[]={
 "主要 / Main","输入 / Input","风格 / Style","着色 / Tint","镜头污垢 / Lens Dirt","质量与输出 / Quality and Output",
 "发光基于 / Glow Based On","发光阈值 / Glow Threshold","发光半径 / Glow Radius","发光强度 / Glow Intensity",
 "合成原始项目 / Composite Original","发光操作 / Glow Operation","发光颜色 / Glow Colors","颜色循环 / Color Looping",
 "循环次数 / Color Loops","色彩相位 / Color Phase","A 和 B 中点 / A & B Midpoint","颜色 A / Color A","颜色 B / Color B","发光维度 / Glow Dimensions",
 "辉光模式 / Glow Mode","混合模式 / Blend Mode","半径 / Radius","曝光 / Exposure","阈值模式 / Threshold Mode","阈值 / Threshold",
 "阈值平滑度 / Threshold Smoothness","饱和度偏差 / Saturation Bias","遮罩图层 / Mask Layer","遮罩模式 / Mask Mode","反转遮罩 / Invert Mask",
 "伽马校正 / Gamma Correction","工作伽马 / Working Gamma","长宽比 / Aspect Ratio","角度 / Angle","像素色差 / Pixel Aberration","通道半径差 / Channel Spread",
 "着色模式 / Tint Mode","内部颜色 / Inner Color","外部颜色 / Outer Color","着色强度 / Tint Amount","色调映射 / Tone Mapping",
 "纹理图层 / Texture Layer","纹理透明度 / Texture Opacity","继承发光 / Inherit Glow","纹理曝光 / Texture Exposure","纹理伽马 / Texture Gamma",
 "质量预设 / Quality Preset","视图 / View","源透明度 / Source Opacity","取消预乘 / Unmult","扩展边缘 / Repeat Edges",
 "颜色与合成 / Color and Composite","无 / None","预览计算中… / Rendering preview…","预览失败，请调整参数或取消。 / Preview failed. Change parameters or cancel.",
 "独立重建，非原版 Deep Glow。 / Independent reconstruction; not the original Deep Glow.",
 "输出限于 PS 滤镜作用范围；透明图层建议预留画布边距。 / Output is limited to Photoshop filter bounds. Leave canvas space around transparent artwork.",
 "输入图层读取失败或名称不唯一，请重新选择。 / Input layer read failed or name is ambiguous. Select again.","交换颜色 / Swap Colors"};
enum Label {main_section,input_section,style_section,tint_section,dirt_section,quality_section,
 based_label,threshold_label,radius_label,intensity_label,composite_label,operation_label,colors_label,looping_label,
 loops_label,phase_label,midpoint_label,a_label,b_label,dimensions_label,falloff_label,blend_label,deep_radius_label,exposure_label,
 threshold_mode_label,deep_threshold_label,smooth_label,saturation_label,mask_layer_label,mask_mode_label,invert_label,
 gamma_amount_label,gamma_label,aspect_label,angle_label,shift_label,spread_label,tint_label,inner_label,outer_label,tint_amount_label,tone_label,
 texture_label,texture_opacity_label,texture_glow_label,texture_exposure_label,texture_gamma_label,quality_label,view_label,source_label,unmult_label,edge_label,
 color_section,none_label,rendering_label,failed_label,independent_label,bounds_label,input_error_label,swap_label};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;
 Image mask,texture,last;psui::PreviewView view;psui::ProxyImage proxy;
 psui::PreviewBackground background;
 std::vector<std::uint8_t> display;
 bool preview=true,dirty=true,display_dirty=true;OSErr error=noErr;ULONGLONG update_at=0;
 psui::AsyncPreview<Image> worker; // destroyed first, before referenced images
 Context(Parameters values,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& ls):p(values),host(f),source(src),layers(ls){}
 bool input(std::uint64_t key,Image& image){
  worker.stop();image={};if(!key)return true;
  const psui::HostLayer* selected=nullptr;
  for(const auto& l:layers)if(l.name_key==key){if(selected)return false;selected=&l;}
  if(!selected)return false;
  psui::LayerPixels pixels;if(psui::host_read_layer(host,*selected,host->documentInfo->bounds,0,pixels))return false;
  image=from_host(std::move(pixels));return true;
 }
};
bool choice(psui::LibraryPanel& ui,int label,int& value,const char* const* options,int count){return ui.choice(labels[label],value,options,count);}
bool number(psui::LibraryPanel& ui,int label,double& value,double lo,double hi,double sliding_lo,double sliding_hi){return ui.number(labels[label],value,lo,hi,sliding_lo,sliding_hi);}
bool color(psui::LibraryPanel& ui,int label,std::array<double,3>& v,FilterRecord* host){
 ImGui::PushID(labels[label]);bool changed=ui.parameter_label(labels[label],v);
 changed|=ui.color_swatch(labels[label],host,v.data(),{54*ui.scale(),20*ui.scale()});
 ImGui::PopID();return changed;
}
bool layer(psui::LibraryPanel& ui,Context& c,int label,std::uint64_t& key,Image& image){
 bool changed=false;const char* selected=labels[none_label];for(const auto& l:c.layers)if(l.name_key==key)selected=l.name.c_str();
 ImGui::PushID(labels[label]);changed|=ui.parameter_label(labels[label],key);ImGui::SetNextItemWidth(-1);
 if(ImGui::BeginCombo("##layer",ui.text(selected).c_str())){
  if(ImGui::Selectable(ui.text(labels[none_label]).c_str(),key==0)){key=0;changed=true;}
  for(std::size_t i=0;i<c.layers.size();++i){ImGui::PushID(int(i));if(ImGui::Selectable(c.layers[i].name.c_str(),key==c.layers[i].name_key)){key=c.layers[i].name_key;changed=true;}ImGui::PopID();}
  ImGui::EndCombo();
 }ui.mark(labels[label]);ImGui::PopID();
 if(changed)c.error=c.input(key,image)?noErr:filterBadParameters;
 return changed;
}
void update(psui::LibraryPanel& ui,Context& c){
 Image completed;
 try{if(c.worker.take(completed)){if(completed.valid()){c.last=std::move(completed);c.display_dirty=true;}else c.error=filterBadParameters;}}
 catch(...){c.error=memFullErr;}
 if(c.dirty&&GetTickCount64()>=c.update_at&&c.error==noErr){
  const auto p=c.p;c.worker.start([&c,p](std::stop_token token){Image out;render(c.source,p,out,c.mask.valid()?&c.mask:nullptr,c.texture.valid()?&c.texture:nullptr,[token]{return token.stop_requested();});return out;});c.dirty=false;
 }
 if(!c.display_dirty&&!c.view.dirty)return;
 const auto roi=c.view.region();const int w=roi.right-roi.left,h=roi.bottom-roi.top;
 const int step=std::max(1,int(std::ceil(double(std::max(w,h))/c.view.proxy_side())));
 const int pw=std::max(1,(w+step-1)/step),ph=std::max(1,(h+step-1)/step);
 c.proxy.width=pw;c.proxy.height=ph;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 c.background.prepare(c.host,c.proxy);
 c.display.resize(std::size_t(pw)*ph*3);
 const auto& pixels=c.preview&&c.last.valid()?c.last:c.source;
 const bool alpha=c.host->outTransparencyMask!=0;
 for(int y=0;y<ph;++y)for(int x=0;x<pw;++x){
  const int xx=std::clamp(roi.left+x*step-pixels.left,0,pixels.width-1),yy=std::clamp(roi.top+y*step-pixels.top,0,pixels.height-1);
  const auto i=(std::size_t(yy)*pixels.width+xx)*4;const float a=alpha?std::clamp(pixels.rgba[i+3],0.f,1.f):1;
  const int bg=((x/8+y/8)&1)?156:200;
  if(c.background.enabled){
   std::array<float,4> original{},processed{};for(int k=0;k<4;++k){original[k]=c.source.rgba[i+k];processed[k]=pixels.rgba[i+k];}
   if(!alpha)original[3]=processed[3]=1;
   const auto color=c.background.pixel(x,y,original,processed);
   for(int k=0;k<3;++k)c.display[(std::size_t(y)*pw+x)*3+k]=color[k];
   continue;
  }
  for(int k=0;k<3;++k)c.display[(std::size_t(y)*pw+x)*3+k]=std::uint8_t(std::lround(std::clamp(pixels.rgba[i+k],0.f,1.f)*255*a+bg*(1-a)));
 }
 if(!ui.preview(c.host,c.display.data(),pw,ph))c.error=memFullErr;
 c.display_dirty=c.view.dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, defaults(c.p.kind));
 ui.inherit_choice_default(labels[blend_label],c.p.blend);
 ui.inherit_choice_default(labels[operation_label],c.p.blend);
 const float s=ui.scale();bool changed=false;auto& p=c.p;const bool deep=p.kind==Kind::deep;
 ImGui::BeginChild("preview-side",{200*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),200*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-157*s,100*s,200*s));
 update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["glow.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;
 if(psui::preview_background_control(ui,c.background))c.display_dirty=true;
 ui.preview_zoom(c.view);
 if(c.worker.pending()||c.dirty)ImGui::TextDisabled("%s",ui.text(labels[rendering_label]).c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[failed_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=defaults(p.kind);c.mask={};c.texture={};c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(deep){
  if(ui.section(labels[main_section],true)){
   changed|=choice(ui,falloff_label,p.falloff,falloffs,2);
   int blend=int(p.blend);if(choice(ui,blend_label,blend,blends,5)){p.blend=Blend(blend);changed=true;}
   changed|=number(ui,deep_radius_label,p.radius,0,4096,0,500);changed|=number(ui,exposure_label,p.intensity,0,100,0,5);
  }
  if(ui.section(labels[input_section],true)){
   changed|=choice(ui,threshold_mode_label,p.threshold_mode,thresholds,3);
   changed|=number(ui,deep_threshold_label,p.threshold,0,1000,0,100);changed|=number(ui,smooth_label,p.smoothness,0,100,0,100);
   changed|=number(ui,saturation_label,p.saturation_bias,0,100,0,100);
   changed|=layer(ui,c,mask_layer_label,p.mask_key,c.mask);
   if(p.mask_key){changed|=choice(ui,mask_mode_label,p.mask_mode,masks,2);changed|=ui.check(labels[invert_label],p.invert_mask);}
  }
  if(ui.section(labels[style_section],false)){
   changed|=number(ui,gamma_amount_label,p.gamma_amount,0,100,0,100);changed|=number(ui,gamma_label,p.gamma,.1,5,1,3);
   changed|=number(ui,aspect_label,p.aspect,.05,20,.1,5);changed|=number(ui,angle_label,p.angle,-360000,360000,-180,180);
   changed|=number(ui,shift_label,p.chromatic_shift,-100,100,-10,10);changed|=number(ui,spread_label,p.chromatic_spread,0,100,0,50);
   changed|=choice(ui,tone_label,p.tone,tones,3);
  }
  if(ui.section(labels[tint_section],false)){
   changed|=choice(ui,tint_label,p.tint,tints,3);
   if(p.tint){changed|=color(ui,inner_label,p.color_a,c.host);if(p.tint==2)changed|=color(ui,outer_label,p.color_b,c.host);
    changed|=number(ui,tint_amount_label,p.tint_amount,0,100,0,100);
    if(ui.button("swap",labels[swap_label],-1)){std::swap(p.color_a,p.color_b);changed=true;}}
  }
  if(ui.section(labels[dirt_section],false)){
   changed|=layer(ui,c,texture_label,p.texture_key,c.texture);
   if(p.texture_key){changed|=number(ui,texture_opacity_label,p.texture_opacity,0,100,0,100);changed|=number(ui,texture_glow_label,p.texture_glow,0,100,0,100);
    changed|=number(ui,texture_exposure_label,p.texture_exposure,-10,10,-3,3);changed|=number(ui,texture_gamma_label,p.texture_gamma,.1,5,.1,3);}
  }
  if(ui.section(labels[quality_section],false)){
   changed|=choice(ui,quality_label,p.quality,qualities,3);changed|=choice(ui,view_label,p.view,views,3);
   changed|=number(ui,source_label,p.source_opacity,0,100,0,100);changed|=ui.check(labels[unmult_label],p.unmult);changed|=ui.check(labels[edge_label],p.repeat_edges);
  }
  ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[independent_label]).c_str());
 }else{
  changed|=choice(ui,based_label,p.based_on,based,2);changed|=number(ui,threshold_label,p.threshold,0,100,0,100);
  changed|=number(ui,radius_label,p.radius,0,4096,0,200);changed|=number(ui,intensity_label,p.intensity,0,100,0,5);
  if(ui.section(labels[color_section],true)){
   changed|=choice(ui,composite_label,p.composite,composites,3);
   int blend=int(p.blend);if(choice(ui,operation_label,blend,blends,5)){p.blend=Blend(blend);changed=true;}
   changed|=choice(ui,colors_label,p.colors,colors,2);
   if(p.colors){changed|=choice(ui,looping_label,p.looping,loops,3);changed|=number(ui,loops_label,p.loops,0,100,0,10);
    changed|=number(ui,phase_label,p.phase,-360000,360000,0,360);changed|=number(ui,midpoint_label,p.midpoint,0,100,0,100);
    changed|=color(ui,a_label,p.color_a,c.host);changed|=color(ui,b_label,p.color_b,c.host);
    if(ui.button("swap",labels[swap_label],-1)){std::swap(p.color_a,p.color_b);changed=true;}}
  }
  changed|=choice(ui,dimensions_label,p.dimensions,dimensions,3);
 }
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[bounds_label]).c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[input_error_label]).c_str());
 ImGui::EndChild();
 if(changed){c.worker.stop();if((!p.mask_key||c.mask.valid())&&(!p.texture_key||c.texture.valid()))c.error=noErr;c.dirty=true;c.update_at=GetTickCount64()+50;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,const Image& source,
 const std::vector<psui::HostLayer>& layers,Parameters& p,Image& mask,Image& texture){
 Context c(p,host,source,layers);
 if(!c.input(p.mask_key,c.mask)||!c.input(p.texture_key,c.texture))c.error=filterBadParameters;
 std::string glyphs;for(auto text:labels)glyphs+=text;
 for(auto list:{std::vector<const char*>(std::begin(blends),std::end(blends)),{based[0],based[1]}})for(auto text:list)glyphs+=text;
 auto add=[&](auto& options){for(auto text:options)glyphs+=text;};
 add(composites);add(colors);add(loops);add(dimensions);add(falloffs);add(thresholds);add(tints);add(tones);add(qualities);add(views);add(masks);
 for(const auto& l:layers)glyphs+=l.name;
 const auto result=psui::run_library_panel(module,owner,host,title,p.kind==Kind::deep?790:740,p.kind==Kind::deep?580:440,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);
 c.worker.stop();if(result==IDOK&&!c.error){p=c.p;mask=std::move(c.mask);texture=std::move(c.texture);}
 return result;
}
}
