#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "surface_host.hpp"
#include "script_parameters.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/async_preview.hpp"
#include "psui/preview_background.hpp"
#include <cmath>
namespace surface {namespace {
constexpr const char* properties[]={"红色 / Red","绿色 / Green","蓝色 / Blue","Alpha / Alpha","亮度 / Luminance","明度 / Lightness"};
constexpr const char* light_types[]={"远光 / Distant Light","点光 / Point Light"};
constexpr const char* current="当前图层 / Current Layer";
constexpr const char* map_label=effect_kind==Kind::blob?"胶状图层 / Blob Layer":"凹凸贴图 / Bump Map";
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;Image map,last;MapBinding binding;int selected=-1;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;ULONGLONG due=0;
 psui::AsyncPreview<Image> worker;
 Context(Parameters values,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& ls,const MapBinding* saved):p(values),host(f),source(src),layers(ls){if(saved)binding=*saved;}
 bool read_map(){worker.stop();map={};selected=-1;if(!p.map_key){binding.layer_id=0;binding.name.clear();return true;}const auto* layer=script::resolve(host,layers,p,binding);if(!layer)return false;selected=int(layer-layers.data());psui::LayerPixels pixels;const VRect area{source.top,source.left,source.top+source.height,source.left+source.width};if(psui::host_read_layer(host,*layer,area,0,pixels))return false;map=from_host(std::move(pixels));return true;}
};
#include "preview_update.inc"
bool map_control(psui::LibraryPanel& ui,Context& c){
 bool changed=ui.parameter_label(map_label,c.p.map_key);const char* selected=c.selected>=0?c.layers[c.selected].name.c_str():current;
 ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##map",ui.text(selected).c_str())){
  if(ImGui::Selectable(ui.text(current).c_str(),c.p.map_key==0)){c.p.map_key=0;c.binding.layer_id=0;c.binding.name.clear();changed=true;}
  for(std::size_t i=0;i<c.layers.size();++i){ImGui::PushID(int(i));const auto label=std::to_string(i+1)+". "+c.layers[i].name;if(ImGui::Selectable(label.c_str(),c.selected==int(i))){c.p.map_key=c.layers[i].name_key;c.binding.layer_id=0;script::capture(c.host,c.layers[i],c.binding);changed=true;}ui.mark(("map.item/"+std::to_string(i)).c_str());ImGui::PopID();}ImGui::EndCombo();
 }ui.mark("map.layer");if(changed)c.error=c.read_map()?noErr:filterBadParameters;return changed;
}
void draw(psui::LibraryPanel& ui,Context& c){
 const auto document=c.host->documentInfo->bounds;const auto factory=defaults(effect_kind,document.right-document.left,document.bottom-document.top,document.left,document.top);ui.parameter_defaults(c.p,factory);auto& p=c.p;const float s=ui.scale();bool changed=false;
 ImGui::BeginChild("preview-side",{220*s,0},0,ImGuiWindowFlags_NoScrollbar);const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),220*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-150*s,110*s,220*s));update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["surface.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");if(p.light_type==1&&ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){const auto mouse=ImGui::GetMousePos();p.light_x=c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01);p.light_y=c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01);changed=true;}
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.dirty||c.worker.pending())ImGui::TextDisabled("%s",ui.text("正在计算… / Rendering preview…").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text("读取或计算失败，请重新选择贴图、全部重置或取消。 / Failed to read or render. Reselect the map, reset all, or cancel.").c_str());
 ImGui::TextWrapped("%s",ui.text("点光模式可右键预览定位灯光。 / In Point Light mode, right-click the preview to position the light.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.map={};c.binding={};c.selected=-1;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 auto number=[&](const char* label,double& value,double lo,double hi,double slo,double shi,bool enabled=true){changed|=ui.number(label,value,lo,hi,slo,shi,enabled);};
 if(ui.section(effect_kind==Kind::blob?"胶状 / Blobbiness":"表面 / Surface",true)){
  changed|=map_control(ui,c);ImGui::BeginDisabled(!p.map_key);if(ui.button("refresh-map","重新读取贴图 / Refresh Map",-1)){c.error=c.read_map()?noErr:filterBadParameters;c.binding.revision=(c.binding.revision+1)%1000000000;changed=true;}ImGui::EndDisabled();changed|=ui.choice("属性 / Property",p.property,properties,6);number("柔和度 / Softness",p.softness,0,500,0,100);
  if(effect_kind==Kind::blob)number("切除（%） / Cut Away (%)",p.cut,0,100,0,100);
  else {number("高度 / Height",p.height,-100,100,-100,100);number("置换 / Displacement",p.displacement,-500,500,-200,200);}
 }
 if(ui.section("灯光 / Light",true)){
  ImGui::TextDisabled("%s",ui.text("效果灯光 / Effect Light").c_str());
  number("灯光强度（%） / Light Intensity (%)",p.intensity,0,1000,0,200);
  changed|=psui::color_control(ui,c.host,"灯光颜色 / Light Color",p.light_color.data());
  changed|=ui.choice("灯光类型 / Light Type",p.light_type,light_types,2);number("灯光高度 / Light Height",p.light_height,-100,100,-100,100);
  changed|=psui::point_control(ui,"灯光位置 / Light Position",p.light_x,p.light_y,p.light_type==1);
  number("灯光方向（度） / Light Direction (deg)",p.direction,-360000,360000,-180,180,p.light_type==0);
 }
 if(ui.section("材质 / Shading",true)){
  number("环境光（%） / Ambient (%)",p.ambient,0,200,0,200);number("漫反射（%） / Diffuse (%)",p.diffuse,0,100,0,100);number("镜面反射（%） / Specular (%)",p.specular,0,100,0,100);
  number("粗糙度 / Roughness",p.roughness,.001,.5,.001,.5);number("金属（%） / Metal (%)",p.metal,0,100,0,100);
 }
 ImGui::TextWrapped("%s",ui.text("独立重建；参考 AE 参数和同源输出，边缘与细节仍可能不同。 / Independent reconstruction from AE parameters and same-source references; edges and fine detail can differ.").c_str());
 ImGui::EndChild();if(changed){c.worker.stop();c.dirty=true;c.due=GetTickCount64()+35;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& map,MapBinding* binding){
 Context c(p,host,src,layers,binding);if(!c.read_map())c.error=filterBadParameters;
 const auto answer=psui::run_library_panel(m,owner,host,title,900,760,[&](psui::LibraryPanel& ui){draw(ui,c);},
 "胶状化 玻璃 胶状 表面 胶状图层 凹凸贴图 重新读取贴图 属性 柔和度 切除 高度 置换 灯光 效果灯光 灯光强度 灯光颜色 灯光类型 远光 点光 灯光高度 灯光位置 灯光方向 度 材质 环境光 漫反射 镜面反射 粗糙度 金属 当前图层 红色 绿色 蓝色 亮度 明度 正在计算 读取或计算失败，请重新选择贴图、全部重置或取消。点光模式可右键预览定位灯光。独立重建；参考参数和同源输出，边缘与细节仍可能不同。",&plugin_info);
 c.worker.stop();if(answer==IDOK&&!c.error){p=c.p;map=std::move(c.map);if(binding)*binding=c.binding;}return answer;
}
}
