#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "vector_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include "psui/async_preview.hpp"
#include <algorithm>
#include <cmath>
namespace vectorblur {
namespace {
constexpr const char* presets[]={"自定义 / Custom","默认 / Default","流动 / Flow","轮廓流线 / Contour Flow","单向风 / Wind","色相丝带 / Hue Ribbons"};
constexpr const char* types[]={"自然 / Natural","恒定长度 / Constant Length","垂直坡度 / Perpendicular","方向居中 / Direction Center","方向衰减 / Direction Fading"};
constexpr const char* properties[]={"红色 / Red","绿色 / Green","蓝色 / Blue","Alpha 通道 / Alpha","亮度 / Luminance","明度 / Lightness","色相 / Hue","饱和度 / Saturation"};
constexpr const char* edges[]={"透明 / Transparent","延伸 / Repeat","镜像 / Reflect","循环 / Wrap"};
constexpr const char* qualities[]={"草稿 / Draft","标准 / Standard","高 / High","超高 / Ultra"};
constexpr const char* labels[]={
 "预设 / Preset","矢量模糊 / Vector Blur","类型 / Type","数量（像素） / Amount (px)","角度偏移 / Angle Offset","脊线平滑度 / Ridge Smoothness","旋转圈数 / Revolutions",
 "矢量图 / Vector Map","当前图层 / Current Layer","通道属性 / Property","映射柔化（像素） / Map Softness (px)","反转矢量图 / Invert Vector Map",
 "渲染选项 / Rendering","沿曲线流向 / Follow Curved Flow","采样质量 / Sampling Quality","效果混合（%） / Effect Mix (%)","边缘处理 / Edge Handling","保留源透明度 / Preserve Source Alpha","线性光插值 / Linear-Light Interpolation",
 "显示矢量 / Show Vectors","计算预览中… / Rendering preview…","预览读取失败，请重置或取消。 / Preview failed. Reset or cancel.",
 "图层名称不唯一或无法读取。 / Layer name is ambiguous or unreadable.",
 "独立重建，行为与 CC Vector Blur 可能不同。 / Independent implementation; behavior may differ from CC Vector Blur.",
 "数量为 0 时保留原图。输出受 PS 画布和选区限制。 / Amount 0 preserves the image. Output is limited to the Photoshop canvas and selection.",
 "启用后得到更平滑的曲线拖曳；关闭时更接近 AE 原版。 / Enable for smoother curved trails; disable for a closer AE match."};
enum Label {preset_label,main_label,type_label,amount_label,angle_label,ridge_label,revolutions_label,map_label,current_label,property_label,softness_label,invert_label,
 render_label,follow_label,quality_label,mix_label,edge_label,alpha_label,linear_label,overlay_label,working_label,error_label,map_error_label,about_label,bounds_label,follow_help_label};
struct Rendered {Image image;Field field;};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;
 Image map,last;Field field;psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<float> selection;bool selection_approximate=false;
 std::vector<std::uint8_t> display;bool preview=true,dirty=true,display_dirty=true,overlay=false;
 int preset=0;OSErr error=noErr;ULONGLONG update_at=0;
 psui::AsyncPreview<Rendered> worker; // joins before the referenced images are destroyed
 Context(Parameters values,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& ls):p(values),host(f),source(src),layers(ls){for(int i=0;i<5;++i)if(p==vectorblur::preset(i))preset=i+1;}
 bool read_map(){
  worker.stop();map={};if(!p.map_key)return true;const psui::HostLayer* selected=nullptr;
  for(const auto& l:layers)if(l.name_key==p.map_key){if(selected)return false;selected=&l;}
  if(!selected)return false;psui::LayerPixels pixels;
  if(psui::host_read_layer(host,*selected,host->documentInfo->bounds,0,pixels))return false;
  map=from_host(std::move(pixels));return true;
 }
};
bool number(psui::LibraryPanel& ui,int label,double& value,double lo,double hi,double sliding_lo,double sliding_hi){return ui.number(labels[label],value,lo,hi,sliding_lo,sliding_hi);}
bool choice(psui::LibraryPanel& ui,int label,int& value,const char*const* values,int count){return ui.choice(labels[label],value,values,count);}
bool map_control(psui::LibraryPanel& ui,Context& c){
 const char* selected=labels[current_label];for(const auto& l:c.layers)if(l.name_key==c.p.map_key)selected=l.name.c_str();bool changed=false;
 changed|=ui.parameter_label(labels[map_label],c.p.map_key);ImGui::SetNextItemWidth(-1);
 if(ImGui::BeginCombo("##vector-map",ui.text(selected).c_str())){
  if(ImGui::Selectable(ui.text(labels[current_label]).c_str(),c.p.map_key==0)){c.p.map_key=0;changed=true;}
  for(std::size_t i=0;i<c.layers.size();++i){ImGui::PushID(int(i));if(ImGui::Selectable(c.layers[i].name.c_str(),c.p.map_key==c.layers[i].name_key)){c.p.map_key=c.layers[i].name_key;changed=true;}ui.mark(("map.item/"+std::to_string(i)).c_str());ImGui::PopID();}
  ImGui::EndCombo();
 }ui.mark("map.layer");if(changed)c.error=c.read_map()?noErr:filterBadParameters;return changed;
}
#include "preview_update.inc"
void vectors(psui::LibraryPanel& ui,Context& c){
 if(!c.overlay||!c.preview||c.field.vectors.empty())return;
 const auto rect=ui.items.at("image");const float z=float(c.view.percent*.01),s=ui.scale();
 auto* list=ImGui::GetWindowDrawList();list->PushClipRect({rect.x,rect.y},{rect.z,rect.w},true);
 for(float sy=rect.y+14*s;sy<rect.w;sy+=27*s)for(float sx=rect.x+14*s;sx<rect.z;sx+=27*s){
  const float x=float(c.view.center_x()+(sx-(rect.x+rect.z)*.5f)/z-c.source.left);
  const float y=float(c.view.center_y()+(sy-(rect.y+rect.w)*.5f)/z-c.source.top);
  if(x<0||y<0||x>=c.field.width||y>=c.field.height)continue;const auto v=c.field.at(x,y);const float length=std::hypot(v.x,v.y);if(length<.02f)continue;
  const ImVec2 a{sx-v.x*5*s,sy-v.y*5*s},b{sx+v.x*7*s,sy+v.y*7*s};const auto color=IM_COL32(110,215,255,220);
  list->AddLine(a,b,color,1.3f*s);const float dx=v.x/length,dy=v.y/length;
  list->AddLine(b,{b.x-(dx*3-dy*2)*s,b.y-(dy*3+dx*2)*s},color,s);
  list->AddLine(b,{b.x-(dx*3+dy*2)*s,b.y-(dy*3-dx*2)*s},color,s);
 }
 list->PopClipRect();
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, Parameters{});
 ui.inherit_choice_default(labels[type_label],c.p.type);
 ui.inherit_choice_default(labels[property_label],c.p.property);
 ui.inherit_choice_default(labels[edge_label],c.p.edge);
 const float s=ui.scale();bool changed=false,preset_changed=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-170*s,100*s,205*s));
 update(ui,c);ui.image_view(c.view,c.proxy);vectors(ui,c);
 ui.items["vector.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 ui.items["vector.settings"]={float(p.type),float(p.amount),c.overlay?1.f:0.f,p.follow?1.f:0.f};
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;
 if(psui::preview_background_control(ui,c.background))c.display_dirty=true;
 ui.preview_zoom(c.view);ui.check(labels[overlay_label],c.overlay);
 if(c.worker.pending()||c.dirty)ImGui::TextDisabled("%s",ui.text(labels[working_label]).c_str());
 if(c.selection_approximate)ImGui::TextDisabled("%s",ui.text("选区缩览为矩形近似 / Rectangular selection preview").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[error_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=Parameters{};c.preset=1;c.map={};c.error=noErr;changed=preset_changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(choice(ui,preset_label,c.preset,presets,6)&&c.preset){c.worker.stop();p=vectorblur::preset(c.preset-1);c.map={};c.error=noErr;changed=preset_changed=true;}
 if(ui.section(labels[main_label],true)){
  int type=int(p.type);if(choice(ui,type_label,type,types,5)){p.type=Type(type);changed=true;}
  changed|=number(ui,amount_label,p.amount,-2000,2000,-200,200);
  changed|=number(ui,angle_label,p.angle,-360000,360000,-180,180);
  if(p.type==Type::direction_center||p.type==Type::direction_fading)changed|=number(ui,revolutions_label,p.revolutions,-100,100,-5,5);
  else changed|=number(ui,ridge_label,p.ridge,.05,100,.05,10);
  changed|=map_control(ui,c);int property=int(p.property);if(choice(ui,property_label,property,properties,8)){p.property=Property(property);changed=true;}
  changed|=number(ui,softness_label,p.map_softness,0,500,0,100);
  changed|=ui.check(labels[invert_label],p.invert_map);
 }
 if(ui.section(labels[render_label],true)){
  changed|=ui.check(labels[follow_label],p.follow);
  if(ImGui::IsItemHovered())ImGui::SetTooltip("%s",ui.text(labels[follow_help_label]).c_str());
  changed|=choice(ui,quality_label,p.quality,qualities,4);
  changed|=number(ui,mix_label,p.mix,0,100,0,100);
  int edge=int(p.edge);if(choice(ui,edge_label,edge,edges,4)){p.edge=Edge(edge);changed=true;}
  changed|=ui.check(labels[alpha_label],p.preserve_alpha);changed|=ui.check(labels[linear_label],p.linear_light);
 }
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[about_label]).c_str());ImGui::TextWrapped("%s",ui.text(labels[bounds_label]).c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[map_error_label]).c_str());
 ImGui::EndChild();
 if(changed){c.worker.stop();if(!p.map_key||c.map.valid())c.error=noErr;if(!preset_changed)c.preset=0;c.dirty=true;c.update_at=GetTickCount64()+50;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,const Image& source,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& map){
 Context c(p,host,source,layers);if(!c.read_map())c.error=filterBadParameters;
 std::string glyphs;for(auto label:labels)glyphs+=label;
 auto add=[&](const auto& list){for(auto text:list)glyphs+=text;};add(presets);add(types);add(properties);add(edges);add(qualities);for(const auto& layer:layers)glyphs+=layer.name;
 const auto answer=psui::run_library_panel(module,owner,host,title,780,570,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);
 c.worker.stop();if(answer==IDOK&&!c.error){p=c.p;map=std::move(c.map);}return answer;
}
}
