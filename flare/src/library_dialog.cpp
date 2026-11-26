#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "flare_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/image_bounds.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "preset_files.hpp"
#include "thumbnail_renderer.hpp"
#include <cstdio>
namespace flare {namespace {
constexpr const char* presets[]={"自定义 / Custom","电影蓝光 / Cinematic Blue","暖阳 / Warm Sun","复古镜头 / Vintage Lens","彩虹光环 / Rainbow Halo","宽银幕蓝 / Anamorphic Blue","柔和自然光 / Soft Natural","洋红霓光 / Magenta Neon","棱镜光圈 / Prism Aperture"};
constexpr const char* blends[]={"滤色 / Screen","相加 / Add","仅耀斑（透明） / Flare Only (Transparent)"};
constexpr const char* masks[]={"关闭 / None","透明度 / Alpha","亮度 / Luminance"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;Image mask,last;int selected=0;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Design design;bool editor=false,history_pending=false;int element=0,new_type=0,solo=-1,editor_page=0,scroll_to_element=-1;char preset_name[97]="My Flare";std::string message;
 struct Snapshot{Parameters p;Design design;};std::vector<Snapshot> undo,redo;Snapshot pending;
 ThumbnailRenderer thumbnails;std::vector<std::filesystem::path> saved_presets;bool refreshed=false;
 Context(Parameters v,FilterRecord* f,const Image& s,const std::vector<psui::HostLayer>& l):p(v),host(f),source(s),layers(l){}
 bool read_mask(){worker.stop();mask={};if(!p.occlusion||!p.map_key)return true;
  const psui::HostLayer* found=nullptr;for(const auto& layer:layers)if(layer.name_key==p.map_key){if(found)return false;found=&layer;}if(!found)return false;
  psui::LayerPixels pixels;if(psui::host_read_reference(host,*found,pixels))return false;mask=from_host(std::move(pixels));return true;
 }
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;auto design=c.design;if(c.editor&&c.solo>=0)for(std::size_t i=0;i<design.elements.size();i++)design.elements[i].enabled=design.elements[i].enabled&&int(i)==c.solo;psui::SpatialFrame<Image> frame;
  if(!c.error&&c.worker.update(c.dirty,frame,[&c,p,design](psui::PreviewPass pass,const Cancel& cancel){psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,pass);const auto& src=c.cache.get(c.source,result.space,cancel);if(cancel())return result;
   const RenderView view{double(c.source.left),double(c.source.top),double(c.source.width),double(c.source.height),result.space.step};
   // Sample occlusion in native coordinates, keeping visibility stable in draft frames.
   const Image* mask=c.mask.valid()?&c.mask:&c.source;render(src,p,result.image,mask,cancel,view,&design);return result;
  })){if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;}else c.error=filterBadParameters;}
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
bool layer_control(psui::LibraryPanel& ui,Context& c){
 bool changed=ui.parameter_label("遮挡参考图层 / Occlusion Layer",&c.p.map_key,sizeof(c.p.map_key));
 std::string current=ui.text("当前图层 / Current Layer");for(const auto& l:c.layers)if(l.name_key==c.p.map_key)current=l.name;
 if(c.p.map_key&&current==ui.text("当前图层 / Current Layer"))current=ui.text("参考图层缺失 / Missing Layer");
 ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##mask-layer",current.c_str())){
  if(ImGui::Selectable(ui.text("当前图层 / Current Layer").c_str(),c.p.map_key==0)){c.p.map_key=0;changed=true;}
  for(std::size_t i=0;i<c.layers.size();i++){ImGui::PushID(int(i));if(ImGui::Selectable((std::to_string(i+1)+". "+c.layers[i].name).c_str(),c.p.map_key==c.layers[i].name_key)){c.p.map_key=c.layers[i].name_key;changed=true;}ImGui::PopID();}ImGui::EndCombo();
 }return changed;
}
#include "designer.inc"
void draw(psui::LibraryPanel& ui,Context& c){
 const auto factory=defaults(c.source.width,c.source.height,c.source.left,c.source.top);ui.parameter_defaults(c.p,factory);bool changed=false,applied=false;auto& p=c.p;
 const float footer = ui.filter_footer_height();
 ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);update(ui,c);
 psui::PreviewAdjustments handles;
 handles.point("光源位置 / Hotspot", p.hx, p.hy).point("光学中心 / Pivot", p.px, p.py);
 changed |= ui.image_view(c.view,c.proxy,handles);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["flare.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.brightness),float(p.hx)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("计算失败或参考层缺失，请重新选择、重置或取消。 / Render failed or input missing. Reselect, reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.design={};c.editor=false;c.mask={};c.undo.clear();c.redo.clear();c.history_pending=false;c.element=0;c.solo=-1;c.selected=0;c.error=noErr;changed=true;}ImGui::EndChild();ImGui::SameLine();ui.begin_filter_parameters();
 if(c.editor){changed|=draw_designer(ui,c);ui.end_filter_parameters();if(changed){c.selected=0;c.dirty=true;}return;}
 auto number=[&](const char* label,double& value,double lo,double hi,double sl,double sh){changed|=ui.number(label,value,lo,hi,sl,sh);};
 auto integer=[&](const char* label,int& value,int lo,int hi){double n=value;ui.inherit_number_default(label,value);if(ui.number(label,n,lo,hi,lo,hi,true,true)){value=int(n);changed=true;}};
 if(ui.choice("预设 / Preset",c.selected,presets,9)&&c.selected){const double hx=p.hx,hy=p.hy,px=p.px,py=p.py;p=preset(c.selected-1,c.source.width,c.source.height,c.source.left,c.source.top);p.hx=hx;p.hy=hy;p.px=px;p.py=py;c.worker.stop();c.mask={};changed=applied=true;}
 if(applied&&c.design.enabled)c.design=design_from_legacy(p);
 if(ui.button("designer.open","耀斑设计器 / Flare Designer",-1)){if(!c.design.enabled){if(c.design.elements.empty())c.design=design_from_legacy(p);c.design.enabled=true;}c.editor=true;c.refreshed=false;c.solo=-1;changed=true;}
 if(!c.design.elements.empty()){const bool default_stack=false;ui.parameter_defaults(c.design.enabled,default_stack);changed|=ui.check("使用元素堆栈 / Use Element Stack",c.design.enabled);ui.parameter_defaults(c.p,factory);}
 if(ui.section("主要 / Main",true)){
  changed|=psui::point_control(ui,"光源位置 / Hotspot",p.hx,p.hy);changed|=psui::point_control(ui,"光学中心 / Pivot",p.px,p.py);
  number("亮度 / Brightness",p.brightness,0,20,0,3);number("整体尺寸（%） / Scale (%)",p.scale,1,1000,10,300);number("高宽比 / Relative Height",p.aspect,.1,10,.2,3);changed|=psui::color_control(ui,c.host,"整体颜色 / Overall Color",p.color.data());
 }
 if(!c.design.enabled&&ui.section("光源与光芒 / Hotspot and Rays",true)){
  number("光源亮度 / Hotspot Brightness",p.hot_bright,0,20,0,3);number("光源尺寸 / Hotspot Size",p.hot_size,.01,10,.05,3);number("柔光亮度 / Glow Brightness",p.glow,0,20,0,3);changed|=psui::color_control(ui,c.host,"光源颜色 / Hotspot Color",p.hot_color.data());
  number("射线亮度 / Rays Brightness",p.ray_bright,0,20,0,3);integer("射线数量 / Ray Count",p.rays,0,64);number("射线长度 / Rays Length",p.ray_length,.01,10,.1,4);number("射线粗细 / Rays Thickness",p.ray_thickness,.01,10,.1,4);number("旋转（度） / Rotation (deg)",p.rotation,-1e9,1e9,-180,180);
 }
 if(!c.design.enabled&&ui.section("镜头鬼影 / Lens Ghosts",false)){
  number("鬼影亮度 / Ghost Brightness",p.ghost_bright,0,20,0,3);integer("鬼影数量 / Ghost Count",p.ghosts,0,24);integer("光圈叶片 / Aperture Blades",p.blades,3,16);
  number("鬼影尺寸 / Ghost Size",p.ghost_size,.01,10,.1,3);number("鬼影间距 / Ghost Spacing",p.ghost_spacing,.01,10,.1,3);number("鬼影柔和 / Ghost Softness",p.ghost_softness,0,1,0,1);changed|=psui::color_control(ui,c.host,"鬼影颜色 / Ghost Color",p.ghost_color.data());
 }
 if(!c.design.enabled&&ui.section("光环与拉丝 / Halo and Streak",false)){
  number("光环亮度 / Halo Brightness",p.ring_bright,0,20,0,3);number("光环尺寸 / Halo Size",p.ring_size,.01,10,.1,3);number("光谱色彩（%） / Spectral Color (%)",p.chromatic,0,100,0,100);
  number("拉丝亮度 / Streak Brightness",p.streak_bright,0,20,0,3);number("拉丝长度 / Streak Length",p.streak_length,.01,10,.1,4);number("拉丝宽度 / Streak Width",p.streak_width,.01,10,.1,4);
 }
 if(ui.section("着色与气氛 / Color and Atmosphere",false)){
  number("伽马 / Gamma",p.gamma,.1,5,.1,3);number("饱和度（%） / Saturation (%)",p.saturation,0,300,0,200);number("色相偏移（度） / Hue Shift (deg)",p.hue,-1e9,1e9,-180,180);
  number("气氛强度（%） / Atmosphere (%)",p.atmosphere,0,100,0,100);number("气氛纹理尺寸 / Noise Scale",p.noise_scale,1,10000,10,500);
  double seed=p.seed;ui.inherit_number_default("随机种子 / Random Seed",p.seed);if(ui.number("随机种子 / Random Seed",seed,0,4294967295.,0,1000,true,true)){p.seed=std::uint32_t(seed);changed=true;}
 }
 if(ui.section("位置触发 / Position Triggers",false)){
  number("边缘范围（%） / Edge Width (%)",p.edge_width,0,100,0,50);number("边缘亮度倍率 / Edge Brightness",p.edge_bright,0,5,0,3);number("中心范围（%） / Center Radius (%)",p.center_radius,0,100,0,50);number("中心亮度倍率 / Center Brightness",p.center_bright,0,5,0,3);
 }
 if(ui.section("遮挡 / Occlusion",false)){
  bool reload=ui.choice("遮挡通道 / Occlusion From",p.occlusion,masks,3);reload|=layer_control(ui,c);if(reload){c.error=c.read_mask()?noErr:filterBadParameters;changed=true;}
  changed|=ui.check("反转遮挡 / Invert Occlusion",p.invert_occlusion);number("遮挡柔和（像素） / Occlusion Softness (px)",p.occlusion_softness,0,2000,0,100);
  ImGui::TextWrapped("%s",ui.text("白色或不透明区域遮挡光源；修改外部层后需重新进入效果刷新。 / White or opaque regions block the light. Reopen the effect after editing an external layer.").c_str());
 }
 if(ui.section("输出 / Output",true)){
  changed|=ui.choice("混合模式 / Blend Mode",p.blend,blends,3);number("背景亮度（%） / Background (%)",p.background,0,100,0,100);number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);ImGui::BeginDisabled(p.blend==2);changed|=ui.check("保留源透明度 / Preserve Source Alpha",p.preserve_alpha);ImGui::EndDisabled();
 }
 ImGui::TextWrapped("%s",ui.text("独立实现的二维镜头耀斑。透明输出建议使用普通图层或智能对象，并预留画布边距。 / Independent 2D lens flare. For transparent output use a regular layer or smart object with canvas margins.").c_str());
 ui.end_filter_parameters();if(changed){if(!applied)c.selected=0;c.dirty=true;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& mask,Design* design){
 Context c(p,host,src,layers);if(design)c.design=*design;if(!c.read_mask())c.error=filterBadParameters;
 const char* glyphs="镜头耀斑 电影蓝光 暖阳 复古镜头 彩虹光环 宽银幕蓝 柔和自然光 洋红霓光 棱镜光圈 自定义 主要 光源位置 光学中心 亮度 整体尺寸 高宽比 整体颜色 光源与光芒 光源亮度 光源尺寸 柔光亮度 光源颜色 射线亮度 射线数量 射线长度 射线粗细 旋转 度 镜头鬼影 鬼影亮度 鬼影数量 光圈叶片 鬼影尺寸 鬼影间距 鬼影柔和 鬼影颜色 光环与拉丝 光环亮度 光环尺寸 光谱色彩 拉丝亮度 拉丝长度 拉丝宽度 着色与气氛 伽马 饱和度 色相偏移 气氛强度 气氛纹理尺寸 随机种子 位置触发 边缘范围 边缘亮度倍率 中心范围 中心亮度倍率 遮挡通道 遮挡参考图层 当前图层 参考图层缺失 关闭 透明度 亮度 反转遮挡 遮挡柔和 像素 白色或不透明区域遮挡光源 修改外部层后需重新进入效果刷新 输出 混合模式 滤色 相加 仅耀斑 透明 背景亮度 效果混合 保留源透明度 独立实现的二维镜头耀斑 透明输出建议使用普通图层或智能对象 并预留画布边距 计算失败或参考层缺失 请重新选择重置或取消";
 std::string all_glyphs=std::string(glyphs)+designer_glyphs;for(const auto& e:c.design.elements)all_glyphs+=" "+e.name;for(const auto& f:files::list())all_glyphs+=" "+files::utf8_name(f);
 const auto answer=psui::run_filter_panel(m,owner,host,title,1180,880,[&](psui::LibraryPanel& ui){draw(ui,c);},all_glyphs.c_str(),&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error){p=c.p;mask=std::move(c.mask);if(design)*design=std::move(c.design);}return answer;
}
}
