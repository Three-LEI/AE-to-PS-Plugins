#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "caustics_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/image_bounds.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "preview_area.hpp"
namespace caustics {namespace {
constexpr const char* presets[]={"自定义 / Custom","清澈水光 / Clear Water","深池光纹 / Deep Pool","暖阳水光 / Golden Water","水下折射 / Underwater","同心涟漪 / Circular Ripples","细碎光网 / Fine Light Mesh","仅叠加水光 / Light Overlay Only"};
constexpr const char* surfaces[]={"内置水波 / Procedural Water","当前图层 / Current Layer","参考图层 / Reference Layer"};
constexpr const char* waves[]={"自然水波 / Natural Water","交错波浪 / Crossing Waves","同心涟漪 / Circular Ripples"};
constexpr const char* channels[]={"亮度 / Luminance","红 / Red","绿 / Green","蓝 / Blue","透明度 / Alpha"};
constexpr const char* outputs[]={"与源图合成 / Composite","焦散纹理 / Caustic Texture","仅光纹（透明） / Light Only (Transparent)","水面高度图 / Surface Height"};
constexpr const char* blends[]={"滤色 / Screen","相加 / Add","水底照明 / Bottom Lighting","叠加 / Overlay"};
constexpr const char* borders[]={"延伸边缘 / Clamp","镜像 / Mirror","重复 / Repeat","透明 / Transparent"};
constexpr const char* qualities[]={"标准 / Standard","精细 / Fine","最高 / Highest"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;Image map,last;int selected=0;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;preview::SourceAreaCache before_cache;
 std::vector<std::uint8_t> display;std::vector<float> selection;bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;RenderCache transport;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters v,FilterRecord* f,const Image& s,const std::vector<psui::HostLayer>& l):p(v),host(f),source(s),layers(l){}
 bool read_map(){
  worker.stop();transport.clear();map={};if(p.surface!=2)return true;
  const psui::HostLayer* found=nullptr;for(const auto& layer:layers)if(layer.name_key==p.map_key){if(found)return false;found=&layer;}if(!found)return false;
  psui::LayerPixels pixels;if(psui::host_read_reference(host,*found,pixels))return false;map=from_host(std::move(pixels));return true;
 }
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(!c.error&&c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass pass,const Cancel& cancel){psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,pass);const auto& src=c.cache.get(c.source,result.space,cancel);if(cancel())return result;
   const Image* map=p.surface==1?&c.source:c.map.valid()?&c.map:nullptr;
   render(src,p,result.image,map,cancel,{double(c.source.left),double(c.source.top),double(c.source.width),double(c.source.height),result.space.step},&c.transport);return result;
  })){if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;}else c.error=filterBadParameters;}
 }catch(...){c.error=memFullErr;}
 preview::compose(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.before_cache,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
bool layer_control(psui::LibraryPanel& ui,Context& c){
 bool changed=ui.parameter_label("高度参考图层 / Height Reference",&c.p.map_key,sizeof(c.p.map_key));
 std::string current=ui.text("请选择图层 / Select Layer");for(const auto& layer:c.layers)if(layer.name_key==c.p.map_key)current=layer.name;
 if(c.p.map_key&&current==ui.text("请选择图层 / Select Layer"))current=ui.text("参考图层缺失 / Missing Layer");
 ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##height-layer",current.c_str())){
  for(std::size_t i=0;i<c.layers.size();i++){ImGui::PushID(int(i));if(ImGui::Selectable((std::to_string(i+1)+". "+c.layers[i].name).c_str(),c.p.map_key==c.layers[i].name_key)){c.p.map_key=c.layers[i].name_key;changed=true;}ImGui::PopID();}ImGui::EndCombo();
 }return changed;
}
void draw(psui::LibraryPanel& ui,Context& c){
 const Parameters factory;ui.parameter_defaults(c.p,factory);bool changed=false,applied=false,reload=false;auto& p=c.p;
 const float footer = ui.filter_footer_height();
 ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);update(ui,c);
 const auto bounds = c.view.bounds;
 const auto offset = p.surface == 0 && p.wave == 2 ? psui::PreviewPointSpace{} :
     psui::PreviewPointSpace::offset(.5 * (double(bounds.left) + bounds.right), .5 * (double(bounds.top) + bounds.bottom));
 psui::PreviewAdjustments handles;
 handles.point("水波偏移 / Wave Shift", p.shift_x, p.shift_y, offset);
 changed |= ui.image_view(c.view,c.proxy,handles);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["caustics.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.scale),float(p.refraction)};
 const auto stats=c.transport.stats();ui.items["caustics.cache"]={float(stats.hits),float(stats.builds),float(stats.bytes),0};
 if(c.error)ImGui::TextWrapped("%s",ui.text("计算失败或参考层缺失，请重新选择、重置或取消。 / Render failed or input missing. Reselect, reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();c.transport.clear();p=factory;c.map={};c.selected=0;c.error=noErr;changed=true;}ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 auto number=[&](const char* label,double& v,double lo,double hi,double a,double b){changed|=ui.number(label,v,lo,hi,a,b);};
 if(ui.choice("预设 / Preset",c.selected,presets,8)&&c.selected){
  c.worker.stop();const auto key=p.map_key;const auto surf=p.surface;const auto channel=p.channel;const auto x=p.shift_x,y=p.shift_y;
  const bool center=p.wave!=2&&x==0&&y==0&&c.selected==5;
  p=preset(c.selected-1);p.map_key=key;p.surface=surf;p.channel=channel;p.shift_x=center?c.source.left+c.source.width*.5:x;p.shift_y=center?c.source.top+c.source.height*.5:y;changed=applied=true;
 }
 changed|=ui.choice("输出 / Output",p.output,outputs,4);
 if(ui.section("水波来源 / Water Surface",true)){
  if(ui.choice("水波来源 / Surface Source",p.surface,surfaces,3)){reload=true;if(p.surface!=2)p.map_key=0;}
  if(p.surface==2)reload|=layer_control(ui,c);
  if(p.surface==0){changed|=ui.choice("波形 / Wave Pattern",p.wave,waves,3);number("水波尺寸（像素） / Wave Scale (px)",p.scale,8,4000,20,600);number("复杂度 / Complexity",p.complexity,1,5,1,5);number("演化（度） / Evolution (deg)",p.evolution,-1e9,1e9,-360,360);}
  else{changed|=ui.choice("高度通道 / Height Channel",p.channel,channels,5);number("高度图平滑（像素） / Height Smooth (px)",p.map_softness,0,200,0,50);}
  number("波高（像素） / Wave Height (px)",p.height,0,500,0,80);
 }
 if(ui.section("焦散光纹 / Caustic Light",true)){
  number("光纹强度（%） / Light Intensity (%)",p.intensity,0,1000,0,300);number("聚焦倍率 / Ray Focus",p.focus,0,8,0,4);number("光纹柔化（像素） / Ray Blur (px)",p.softness,0,100,0,12);number("光纹阈值 / Light Threshold",p.threshold,0,5,0,2);
  changed|=psui::color_control(ui,c.host,"光纹颜色 / Light Color",p.light.data());
 }
 if(ui.section("水下折射 / Underwater Refraction",true)){
  ImGui::BeginDisabled(p.output!=0);
  number("折射扭曲（%） / Refraction Amount (%)",p.refraction,0,200,0,100);
  ImGui::EndDisabled();
  number("水深（像素） / Water Depth (px)",p.depth,0,2000,0,400);number("折射率 / Refractive Index",p.refractive_index,1,3,1,2);
 }
 if(ui.section("位置与随机 / Position and Random",false)){
  changed|=psui::point_control(ui,"水波偏移 / Wave Shift",p.shift_x,p.shift_y);
  double seed=p.seed;ui.inherit_number_default("随机种子 / Random Seed",p.seed);if(ui.number("随机种子 / Random Seed",seed,0,4294967295.,0,1000,true,true)){p.seed=std::uint32_t(seed);changed=true;}
 }
 if(ui.section("质量与输出 / Quality and Output",false)){
  int quality=p.quality-1;const int default_quality=factory.quality-1;ui.parameter_defaults(quality,default_quality);if(ui.choice("采样质量 / Sampling Quality",quality,qualities,3)){p.quality=quality+1;changed=true;}ui.parameter_defaults(c.p,factory);
  changed|=ui.choice("边缘处理 / Border",p.border,borders,4);
  ImGui::BeginDisabled(p.output!=0);changed|=ui.choice("混合模式 / Blend Mode",p.blend,blends,4);number("源图亮度（%） / Background (%)",p.background,0,200,0,150);ImGui::EndDisabled();
  if(p.output==1)changed|=psui::color_control(ui,c.host,"暗部颜色 / Dark Color",p.dark.data());
  changed|=ui.check("保留源透明度 / Preserve Source Alpha",p.preserve_alpha);number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 }
 if(p.surface==2)ImGui::TextWrapped("%s",ui.text("修改参考图层后，重新进入效果刷新。 / Reopen the effect after editing its reference layer.").c_str());
 ImGui::EndChild();if(reload){c.error=c.read_map()?noErr:filterBadParameters;changed=true;}
 if(changed){if(!applied)c.selected=0;c.dirty=true;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& map){
 Context c(p,host,src,layers);if(!c.read_map())c.error=filterBadParameters;
 const char* glyphs="焦散 自定义 清澈水光 深池光纹 暖阳水光 水下折射 同心涟漪 细碎光网 内置水波 当前图层 参考图层 自然水波 交错波浪 亮度 红 绿 蓝 透明度 与源图合成 焦散纹理 仅光纹透明 水面高度图 滤色 相加 水底照明 叠加 延伸边缘 镜像 重复 透明 标准 精细 最高 请选择图层 高度参考图层 参考图层缺失 计算失败或参考层缺失请重新选择重置或取消 水波来源 波形 水波尺寸像素 复杂度 演化度 高度通道 高度图平滑 波高 焦散光纹 光纹强度 聚焦倍率 光纹柔化 光纹阈值 光纹颜色 折射扭曲 水深 折射率 位置与随机 水波偏移 随机种子 质量与输出 采样质量 边缘处理 输出 混合模式 源图亮度 暗部颜色 保留源透明度 效果混合 修改参考图层后重新进入效果刷新";
 const auto answer=psui::run_filter_panel(m,owner,host,title,1080,900,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&plugin_info);c.worker.stop();
 if(answer==IDOK&&!c.error){p=c.p;map=std::move(c.map);}return answer;
}
}
