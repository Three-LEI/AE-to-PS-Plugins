#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "energy_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/adaptive_preview.hpp"
#include <cmath>
namespace energy {
namespace {
constexpr const char* modes[]={"两点光束 / Two Points","钢笔路径 / Pen Path","Alpha 轮廓 / Alpha Contour","Alpha 笔画 / Alpha Strokes"};
constexpr const char* styles[]={"基础光束 / Neon","经典电流 / Classic Electric","经典火焰 / Classic Flame","电流光晕 / Electric Wisps","燃烧 / Burning"};
constexpr const char* glow_models[]={"经典（0.1–0.2） / Classic (0.1–0.2)","分层发光 / Layered Glow"};
constexpr const char* glow_labels[]={"发光模型 / Glow Model","近光强度 / Halo Intensity","近光半径（像素） / Halo Radius (px)","发光预伽马 / Glow Pre Gamma","近光偏重 / Near Glow Bias","发光分层设置 / Glow Bands","旧 PSD 保留经典模型；可手动切换到分层发光。 / Older PSDs retain Classic; switch to Layered Glow to try the new light model."};
constexpr const char* band_sizes[]={"远光范围比 / Far Glow Size","中光范围比 / Mid Glow Size","近光范围比 / Near Glow Size","细光范围比 / Fine Glow Size"};
constexpr const char* band_gains[]={"远光强度 / Far Glow Gain","中光强度 / Mid Glow Gain","近光强度比 / Near Glow Gain","细光强度 / Fine Glow Gain"};
constexpr const char* presets[]={"自定义 / Custom","蓝色光束 / Blue Neon","蓝色电流（经典） / Classic Blue Electric","橙色火焰（经典） / Classic Orange Flame","电流光晕 / Electric Wisps","燃烧 / Burning"};
constexpr const auto& common_presets=unified_preset_labels;
constexpr const char* noise_types[]={"平滑 / Smooth","分形 / Fractal","尖锐 / Sharp"};
constexpr const char* common_labels[]={"预设 / Preset","光核噪声类型 / Core Noise Type","外围噪声类型 / Outer Noise Type","外围畸变与纹理 / Outer Distortion and Texture","外围畸变（像素） / Outer Distortion (px)","外围尺度（像素） / Outer Noise Scale (px)","外围细节 / Outer Detail","外围种子 / Outer Seed","外围演化偏移 / Outer Evolution Offset","纹理厚度（像素） / Texture Width (px)","云纹强度 / Cloud Intensity","细丝强度 / Filament Intensity","细丝数量 / Filament Count","风 / Wind","风力（像素） / Wind Amount (px)","风向 / Wind Direction","色彩过渡 / Color Transition","色彩过渡强度 / Color Transition Amount","外围颜色 / Outer Color","旧版兼容：保留已保存外观。转换后请检查预览，取消可保留原参数。 / Legacy compatibility preserves the saved look. Check the preview after conversion; Cancel retains the original settings.","转换为公共参数 / Convert to Common Controls","整体演化 / Overall Evolution","光核畸变（像素） / Core Distortion (px)","光核尺度（像素） / Core Noise Scale (px)","光核细节 / Core Detail","光核种子 / Core Seed"};
constexpr const char* burn_labels[]={"燃烧结构 / Burning Structure","焰体厚度（像素） / Flame Width (px)","卷曲幅度（像素） / Flame Turbulence (px)","火纹尺度（像素） / Fire Texture Scale (px)","焰体强度 / Flame Intensity","焰丝强度 / Flame Filaments","燃烧细节 / Burning Detail","燃烧种子 / Burning Seed","燃烧演化偏移 / Burning Evolution Offset","风 / Wind","风力（像素） / Wind Amount (px)","风向 / Wind Direction","无风时焰体包裹路径；风仅推动周围焰体。 / With no wind, flames wrap the path. Wind moves only the surrounding flame."};
constexpr const char* electric_labels[]={"光核畸变 / Core Distortion","外层电流光晕 / Outer Electric Glow","光晕畸变幅度（像素） / Glow Distortion (px)","光晕纹理尺度（像素） / Glow Noise Scale (px)","光晕厚度（像素） / Cloud Width (px)","云纹强度 / Cloud Intensity","细丝强度 / Filament Intensity","细丝数量 / Filament Count","光晕细节 / Glow Detail","光晕随机种子 / Glow Seed","光晕演化偏移 / Glow Evolution Offset","整体演化 / Overall Evolution","电流光晕使用分层发光；光核亮边跟随光核，外层纹理独立畸变。 / Electric Wisps uses Layered Glow: the bright rim follows the core while the outer texture distorts independently."};
constexpr const char* extension_text[]={"材质预设 / Material Preset","样式 / Style","Alpha 阈值（%） / Alpha Threshold (%)","轮廓包括透明孔洞；完全不透明的图层会描画布边框。 / Contours include holes. An opaque layer outlines the canvas.","笔画模式直接使用原始透明度，宽度由源笔画决定。 / Stroke mode uses source alpha and its original width.","电流形态 / Electric Shape","扰动幅度（像素） / Distortion (px)","纹理尺度（像素） / Noise Scale (px)","细节 / Detail","分叉数量 / Branches","分叉长度（像素） / Branch Length (px)","火焰形态 / Flame Shape","火焰高度（像素） / Flame Height (px)","火焰飘动（像素） / Flame Spread (px)","火焰方向 / Flame Direction","外焰颜色 / Outer Flame Color","随机与演化 / Noise and Evolution","随机种子 / Random Seed","演化 / Evolution","笔画输入不使用弧长修剪、渐细和路径分叉。 / Stroke input does not use path trimming, taper or branches.","支持两点、钢笔路径和 Alpha 输入；材质为独立实现。 / Supports points, pen paths and alpha inputs with independently implemented materials."};
constexpr const char* composites[]={"叠加原图 / Over Source","透明光束 / Transparent","黑底光束 / Black Background"};
constexpr const char* labels[]={"输入 / Input","核心 / Core","发光 / Glow","路径范围 / Path Range","输出 / Output","起点 X / Start X","起点 Y / Start Y","终点 X / End X","终点 Y / End Y","核心宽度（像素） / Core Width (px)","亮度 / Brightness","核心白度（%） / Core Whiteness (%)","光晕半径（像素） / Glow Radius (px)","光晕强度 / Glow Intensity","光束颜色 / Beam Color","开始（%） / Start (%)","结束（%） / End (%)","起点渐细（%） / Start Taper (%)","终点渐细（%） / End Taper (%)","偏移 X / Offset X","偏移 Y / Offset Y","合成 / Composite","更新路径 / Update Path","文档路径 / Document Path","未读取路径 / No Path Snapshot","已保存路径快照 / Saved Path Snapshot","无可用路径 / No Available Paths","左键拖动预览中的起点或终点。 / Drag either endpoint in the preview.","修改钢笔路径后点击更新路径。路径快照随 PSD 保存。 / Click Update Path after editing. The snapshot is saved in the PSD.","第一阶段：两点和钢笔路径；不包含原版全部材质预设。 / Stage 1: points and pen paths; full original material presets are not included.","效果受图层、画布和选区范围限制。智能对象请在内容文档中建立路径以保持定位。 / Layer, canvas and selection bounds apply. Create paths inside smart-object contents for reliable alignment.","预览失败，请重置或取消。 / Preview failed. Reset or cancel.","请选择路径并更新快照。 / Select a path and update its snapshot.","预览 / Preview","确定 / OK","取消 / Cancel","全部重置 / Reset All"};
struct Context {
 Parameters p,factory;Inputs inputs;FilterRecord* host;const Image& source;std::vector<HostPath> paths;int path_index=0,path_token=0,preset=0;
 Path alpha_path;double alpha_level=-1;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;std::string import_error;
 Image last;psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;std::vector<float> selection;std::vector<std::uint8_t> display;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters values,Inputs snapshot,FilterRecord* f,const Image& src):p(values),factory(values.engine?unified_defaults(src.width,src.height,src.left,src.top):calibrated_defaults(src.width,src.height,src.left,src.top)),inputs(std::move(snapshot)),host(f),source(src),paths(host_paths(f)){
  for(std::size_t i=0;i<paths.size();i++)if(paths[i].name==inputs.name){path_index=int(i);break;}
  for(int i=0;i<(p.engine?unified_preset_count:5);i++)if((p.engine?unified_preset(i,p):material_preset(i,p))==p)preset=i+1;
 }
 bool update_path(){
  if(paths.empty()){import_error="No available Photoshop paths";return false;}Inputs next;std::string why;
  if(!import_path(host,paths[path_index].index,next,why)){import_error=why;return false;}
  worker.stop();inputs=std::move(next);import_error.clear();error=noErr;dirty=true;return true;
 }
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(!(p.input==1&&c.inputs.path.curves.empty())&&c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass draft,const std::function<bool()>& cancel){
   psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,draft);const auto& s=result.space;const auto& src=c.cache.get(c.source,s,cancel);if(cancel())return result;
   auto q=s.step>1?draft_parameters(p,s.left,s.top,s.step):p;auto path=s.step>1?draft_path(c.inputs.path,s.left,s.top,s.step):c.inputs.path;
   if(p.input==2){
    if(!draft&&c.alpha_level!=p.alpha_threshold){Path traced;if(!trace_alpha(c.source,p.alpha_threshold,traced,cancel))return result;c.alpha_path=std::move(traced);c.alpha_level=p.alpha_threshold;}
    if(c.alpha_level==p.alpha_threshold&&!c.alpha_path.curves.empty()){path=s.step>1?draft_path(c.alpha_path,s.left,s.top,s.step):c.alpha_path;q.input=1;}
   }
   render(src,q,path,result.image,cancel);return result;
  })){
   if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;c.error=noErr;}else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
bool common_controls(psui::LibraryPanel& ui,Context& c){
 auto& p=c.p;bool changed=false;
 auto integer=[&](const char* label,auto& value,int lo,int hi){ui.inherit_number_default(label,value);double n=value;if(ui.number(label,n,lo,hi,lo,hi,true,true)){value=static_cast<std::remove_reference_t<decltype(value)>>(n);return true;}return false;};
 ui.inherit_choice_default(common_labels[1],p.core_noise);ui.inherit_choice_default(common_labels[2],p.glow_noise);
 if(ui.section("核心 / Core",false)){changed|=ui.number(labels[9],p.width,.1,4096,.1,40,p.input!=3);changed|=ui.number(labels[10],p.brightness,0,100,0,5);changed|=ui.number(labels[11],p.core_white,0,100,0,100);changed|=ui.parameter_label(labels[14],p.color);changed|=ui.color_swatch("beam.color",c.host,p.color.data());}
 if(ui.section("发光 / Glow",false)){
  changed|=ui.number(labels[12],p.glow_radius,0,4096,0,300);changed|=ui.number(labels[13],p.glow_intensity,0,100,0,5);changed|=ui.number(glow_labels[1],p.halo_intensity,0,20,0,4);changed|=ui.number(glow_labels[2],p.halo_radius,0,4096,0,60);changed|=ui.number(glow_labels[3],p.glow_gamma,.1,10,.1,3);changed|=ui.number(glow_labels[4],p.glow_bias,0,2,0,2);
  if(ui.section(glow_labels[5],false))for(int band=0;band<4;band++){changed|=ui.number(band_sizes[band],p.glow_sizes[band],0,4,0,2);changed|=ui.number(band_gains[band],p.glow_gains[band],0,20,0,3);}
 }
 if(ui.section(electric_labels[0],false)){
  changed|=ui.choice(common_labels[1],p.core_noise,noise_types,3);changed|=ui.number(common_labels[22],p.distortion,0,4096,0,60);changed|=ui.number(common_labels[23],p.wavelength,.05,8192,4,256);changed|=integer(common_labels[24],p.detail,1,6);changed|=integer(common_labels[25],p.seed,0,2147483647);changed|=ui.number(common_labels[21],p.evolution,-360000,360000,0,360);
  if(p.input!=3){changed|=integer(extension_text[9],p.branches,0,16);changed|=ui.number(extension_text[10],p.branch_length,0,4096,0,200);}
 }
 if(ui.section(common_labels[3],true)){
  changed|=ui.choice(common_labels[2],p.glow_noise,noise_types,3);changed|=ui.number(common_labels[4],p.glow_distortion,0,4096,0,100);changed|=ui.number(common_labels[5],p.glow_noise_scale,.05,8192,4,256);changed|=integer(common_labels[6],p.glow_detail,1,6);changed|=integer(common_labels[7],p.glow_seed,0,2147483647);changed|=ui.number(common_labels[8],p.glow_evolution,-360000,360000,0,360);changed|=ui.number(common_labels[9],p.glow_width,0,4096,0,40);changed|=ui.number(common_labels[10],p.glow_texture,0,10,0,3);changed|=ui.number(common_labels[11],p.filament_strength,0,10,0,3);changed|=integer(common_labels[12],p.filament_count,0,6);
 }
 if(ui.section(common_labels[13],false)){changed|=ui.number(common_labels[14],p.burn_wind,0,4096,0,100);changed|=ui.number(common_labels[15],p.fire_direction,-360000,360000,-180,180);}
 if(ui.section(common_labels[16],false)){changed|=ui.number(common_labels[17],p.color_mix,0,1,0,1);changed|=ui.parameter_label(common_labels[18],p.tip_color);changed|=ui.color_swatch("outer.color",c.host,p.tip_color.data());}
 return changed;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p,c.factory);ui.inherit_choice_default("输入 / Input",c.p.input);ui.inherit_choice_default("合成 / Composite",c.p.composite);
 ui.inherit_choice_default("样式 / Style",c.p.style);ui.choice_default("材质预设 / Material Preset",1);
 ui.choice_default(common_labels[0],1);
 ui.inherit_choice_default(glow_labels[0],c.p.glow_model);
 auto& p=c.p;bool changed=false,preset_applied=false;
 const float footer = ui.filter_footer_height();
 ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);update(ui,c);
 psui::PreviewAdjustments handles;
 if (p.input == 0) {
  const auto space = psui::PreviewPointSpace::offset(p.offset_x, p.offset_y);
  handles.point("起点 / Start", p.start_x, p.start_y, space).point("终点 / End", p.end_x, p.end_y, space)
         .line(space.x(p.start_x), space.y(p.start_y), space.x(p.end_x), space.y(p.end_y));
 }
 changed |= ui.image_view(c.view,c.proxy,handles);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["energy.state"]={c.last.valid()?1.f:0.f,float(c.error),float(c.inputs.path.curves.size()),float(p.input)};
 ui.items["energy.material"]={float(p.style),float(p.seed),float(p.alpha_threshold),float(p.evolution)};
 ui.items["energy.glow"]={float(p.glow_model),float(p.halo_intensity),float(p.glow_gamma),float(p.glow_bias)};
 ui.items["energy.unified"]={float(p.engine),float(p.core_noise),float(p.glow_noise),float(p.color_mix)};
 ui.items["energy.outer"]={float(p.glow_texture),float(p.filament_strength),float(p.burn_wind),float(p.glow_distortion)};
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[31]).c_str());
 const bool missing=p.input==1&&c.inputs.path.curves.empty();if(missing)ImGui::TextWrapped("%s",ui.text(labels[32]).c_str());
 if(!c.import_error.empty())ImGui::TextWrapped("%s",c.import_error.c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr||missing);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=c.factory;c.inputs={};c.preset=1;c.import_error.clear();c.error=noErr;changed=preset_applied=true;}
 ImGui::EndChild();ImGui::SameLine();ui.begin_filter_parameters();
 if(!p.engine){
  ImGui::TextWrapped("%s",ui.text(common_labels[19]).c_str());
  if(ui.button("common.convert",common_labels[20],-1)){c.worker.stop();p=to_unified(p);c.factory=unified_defaults(c.source.width,c.source.height,c.source.left,c.source.top);c.preset=0;changed=preset_applied=true;}
 }
 if(p.engine){if(ui.choice(common_labels[0],c.preset,common_presets,unified_preset_count+1)&&c.preset){p=unified_preset(c.preset-1,p);changed=preset_applied=true;}}
 else{
  if(ui.choice("材质预设 / Material Preset",c.preset,presets,6)&&c.preset){p=material_preset(c.preset-1,p);changed=preset_applied=true;}
  if(ui.choice("样式 / Style",p.style,styles,5)){changed=true;if(p.style>=3)p.glow_model=1;}
 }
 if(ui.choice("输入 / Input",p.input,modes,4)){changed=true;c.error=noErr;if(p.input==1&&c.inputs.path.curves.empty()&&!c.paths.empty())c.update_path();}
 if(p.input==0){
  changed|=ui.number(labels[5],p.start_x,-1e7,1e7,c.source.left,c.source.left+c.source.width);changed|=ui.number(labels[6],p.start_y,-1e7,1e7,c.source.top,c.source.top+c.source.height);changed|=ui.number(labels[7],p.end_x,-1e7,1e7,c.source.left,c.source.left+c.source.width);changed|=ui.number(labels[8],p.end_y,-1e7,1e7,c.source.top,c.source.top+c.source.height);ImGui::TextWrapped("%s",ui.text(labels[27]).c_str());
 }else if(p.input==1){
  c.path_token=c.inputs.path.curves.empty()?0:1;ui.choice_default("文档路径 / Document Path",0);
  if(ui.parameter_label("文档路径 / Document Path",c.path_token)){c.worker.stop();c.inputs={};c.last={};c.display_dirty=true;c.import_error.clear();changed=true;}
  ImGui::SetNextItemWidth(-1);
  const std::string selected_text=c.paths.empty()?ui.text("无可用路径 / No Available Paths"):c.paths[c.path_index].name;
  if(ImGui::BeginCombo("##document-path",selected_text.c_str())){for(std::size_t i=0;i<c.paths.size();i++){ImGui::PushID(int(i));if(ImGui::Selectable(c.paths[i].name.c_str(),c.path_index==int(i))){c.path_index=int(i);if(c.update_path())changed=true;}ImGui::PopID();}ImGui::EndCombo();}ui.mark("path.selector");
  if(ui.button("path.update","更新路径 / Update Path",-1))changed|=c.update_path();
  ImGui::TextWrapped("%s: %s",ui.text(c.inputs.path.curves.empty()?"未读取路径 / No Path Snapshot":"已保存路径快照 / Saved Path Snapshot").c_str(),c.inputs.name.c_str());ImGui::TextWrapped("%s",ui.text(labels[28]).c_str());
 }else if(p.input==2){changed|=ui.number(extension_text[2],p.alpha_threshold,.1,99.9,.1,99.9);ImGui::TextWrapped("%s",ui.text(extension_text[3]).c_str());
 }else{ImGui::TextWrapped("%s",ui.text(extension_text[4]).c_str());
 }
 if(p.engine)changed|=common_controls(ui,c);
 else{
 if(ui.section("核心 / Core",p.style<3)){changed|=ui.number(labels[9],p.width,.1,4096,.1,40,p.input!=3);changed|=ui.number(labels[10],p.brightness,0,100,0,5);changed|=ui.number(labels[11],p.core_white,0,100,0,100);changed|=ui.parameter_label(labels[14],p.color);changed|=ui.color_swatch("beam.color",c.host,p.color.data());}
 if(ui.section("发光 / Glow",p.style<3)){
  ImGui::BeginDisabled(p.style>=3);changed|=ui.choice(glow_labels[0],p.glow_model,glow_models,2);ImGui::EndDisabled();
  changed|=ui.number(labels[12],p.glow_radius,0,4096,0,300);changed|=ui.number(labels[13],p.glow_intensity,0,100,0,5);
  if(p.glow_model==1){
   changed|=ui.number(glow_labels[1],p.halo_intensity,0,20,0,4);changed|=ui.number(glow_labels[2],p.halo_radius,0,4096,0,60);
   changed|=ui.number(glow_labels[3],p.glow_gamma,.1,10,.1,3);changed|=ui.number(glow_labels[4],p.glow_bias,0,2,0,2);
   if(ui.section(glow_labels[5],false))for(int band=0;band<4;band++){changed|=ui.number(band_sizes[band],p.glow_sizes[band],0,4,0,2);changed|=ui.number(band_gains[band],p.glow_gains[band],0,20,0,3);}
  }
  ImGui::TextWrapped("%s",ui.text(glow_labels[6]).c_str());
 }
 auto integer=[&](const char* label,auto& value,int lo,int hi){ui.inherit_number_default(label,value);double n=value;if(ui.number(label,n,lo,hi,lo,hi,true,true)){value=static_cast<std::remove_reference_t<decltype(value)>>(n);return true;}return false;};
 if(p.style>=3&&ui.section(electric_labels[0],false)){changed|=ui.number(extension_text[6],p.distortion,0,4096,0,60);changed|=ui.number(extension_text[7],p.wavelength,.05,8192,4,256);changed|=integer(extension_text[8],p.detail,1,6);if(p.input!=3){changed|=integer(extension_text[9],p.branches,0,16);changed|=ui.number(extension_text[10],p.branch_length,0,4096,0,200);}}
 if(p.style==3){
  if(ui.section(electric_labels[1],true)){
   changed|=ui.number(electric_labels[2],p.glow_distortion,0,4096,0,100);changed|=ui.number(electric_labels[3],p.glow_noise_scale,.05,8192,4,256);changed|=ui.number(electric_labels[4],p.glow_width,0,4096,0,40);changed|=ui.number(electric_labels[5],p.glow_texture,0,10,0,3);changed|=ui.number(electric_labels[6],p.filament_strength,0,10,0,3);changed|=integer(electric_labels[7],p.filament_count,0,6);changed|=integer(electric_labels[8],p.glow_detail,1,6);changed|=integer(electric_labels[9],p.glow_seed,0,2147483647);changed|=ui.number(electric_labels[10],p.glow_evolution,-360000,360000,0,360);
  }
  ImGui::TextWrapped("%s",ui.text(electric_labels[12]).c_str());
 }
 if(p.style==4){
  if(ui.section(burn_labels[0],true)){
   changed|=ui.number(burn_labels[1],p.burn_width,0,4096,0,50);changed|=ui.number(burn_labels[2],p.burn_turbulence,0,4096,0,100);changed|=ui.number(burn_labels[3],p.burn_scale,.05,8192,4,256);changed|=ui.number(burn_labels[4],p.burn_density,0,10,0,3);changed|=ui.number(burn_labels[5],p.burn_filaments,0,10,0,3);changed|=integer(burn_labels[6],p.burn_detail,1,6);changed|=integer(burn_labels[7],p.burn_seed,0,2147483647);changed|=ui.number(burn_labels[8],p.burn_evolution,-360000,360000,0,360);changed|=ui.parameter_label(extension_text[15],p.tip_color);changed|=ui.color_swatch("burn.color",c.host,p.tip_color.data());
  }
  if(ui.section(burn_labels[9],false)){changed|=ui.number(burn_labels[10],p.burn_wind,0,4096,0,100);changed|=ui.number(burn_labels[11],p.fire_direction,-360000,360000,-180,180);}
  ImGui::TextWrapped("%s",ui.text(burn_labels[12]).c_str());
 }
 if(p.style==1&&ui.section(extension_text[5],true)){changed|=ui.number(extension_text[6],p.distortion,0,4096,0,100);changed|=ui.number(extension_text[7],p.wavelength,.05,8192,4,256);changed|=integer(extension_text[8],p.detail,1,6);if(p.input!=3){changed|=integer(extension_text[9],p.branches,0,16);changed|=ui.number(extension_text[10],p.branch_length,0,4096,0,200);}}
 if(p.style==2&&ui.section(extension_text[11],true)){changed|=ui.number(extension_text[12],p.fire_height,0,4096,0,300);changed|=ui.number(extension_text[13],p.fire_spread,0,4096,0,100);changed|=ui.number(extension_text[14],p.fire_direction,-360000,360000,-180,180);changed|=ui.number(extension_text[7],p.wavelength,.05,8192,4,256);changed|=integer(extension_text[8],p.detail,1,6);changed|=ui.parameter_label(extension_text[15],p.tip_color);changed|=ui.color_swatch("flame.color",c.host,p.tip_color.data());}
 if(p.style!=0&&ui.section(extension_text[16],p.style<3)){changed|=integer(extension_text[17],p.seed,0,2147483647);changed|=ui.number(p.style>=3?electric_labels[11]:extension_text[18],p.evolution,-360000,360000,0,360);}
 }
 if(ui.section("路径范围 / Path Range",false)){if(p.input!=3){changed|=ui.number(labels[15],p.trim_start,0,100,0,100);changed|=ui.number(labels[16],p.trim_end,0,100,0,100);changed|=ui.number(labels[17],p.taper_start,0,100,0,100);changed|=ui.number(labels[18],p.taper_end,0,100,0,100);}else ImGui::TextWrapped("%s",ui.text(extension_text[19]).c_str());changed|=ui.number(labels[19],p.offset_x,-1e7,1e7,-500,500);changed|=ui.number(labels[20],p.offset_y,-1e7,1e7,-500,500);}
 if(ui.section("输出 / Output",true))changed|=ui.choice("合成 / Composite",p.composite,composites,3);
 ImGui::TextWrapped("%s",ui.text(extension_text[20]).c_str());ImGui::TextWrapped("%s",ui.text(labels[30]).c_str());ui.end_filter_parameters();if(changed){if(!preset_applied)c.preset=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,Parameters& p,Inputs& inputs){
 Context c(p,inputs,f,src);std::string glyphs;for(auto label:labels)glyphs+=label;for(auto label:extension_text)glyphs+=label;for(auto mode:modes)glyphs+=mode;for(auto mode:styles)glyphs+=mode;for(auto mode:presets)glyphs+=mode;for(auto mode:composites)glyphs+=mode;for(const auto& path:c.paths)glyphs+=path.name;glyphs+=inputs.name;
 for(auto text:glow_models)glyphs+=text;for(auto text:glow_labels)glyphs+=text;for(auto text:band_sizes)glyphs+=text;for(auto text:band_gains)glyphs+=text;
 for(auto text:electric_labels)glyphs+=text;
 for(auto text:burn_labels)glyphs+=text;
 for(auto text:common_labels)glyphs+=text;for(auto text:common_presets)glyphs+=text;for(auto text:noise_types)glyphs+=text;
 const auto result=psui::run_filter_panel(m,owner,f,title,860,650,[&](auto& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);c.worker.stop();if(result==IDOK&&!c.error){p=c.p;inputs=std::move(c.inputs);}return result;
}
}
