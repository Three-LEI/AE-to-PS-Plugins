#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "longshadow_host.hpp"
#include "psui/adaptive_preview.hpp"
#include "psui/generator_controls.hpp"
#include "psui/library_panel.hpp"

namespace longshadow {namespace {
constexpr const char* presets[]={"自定义 / Custom","经典斜影 / Classic Diagonal","柔和长影 / Fading Shadow","靛蓝实影 / Solid Indigo","暖砖投影 / Warm Brick","悬浮软影 / Floating Soft Shadow"};
constexpr const char* masks[]={"无 / Off","矩形 / Rectangle","椭圆 / Ellipse"};
constexpr const char* colors[]={"纯色 / Solid","拉伸源颜色 / Stretch Source Colors"};
struct Context {
 Parameters p;
 FilterRecord* host;
 const Image& source;
 Image last;
 int selected=0;
 psui::PreviewView view;
 psui::ProxyImage proxy,selection_geometry;
 psui::PreviewBackground background;
 std::vector<std::uint8_t> display;
 std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;
 OSErr error=noErr;
 psui::PreviewSpace mapping;
 psui::DraftImageCache<Image> cache;
 psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters p,FilterRecord* f,const Image& img):p(p),host(f),source(img){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{
  const auto p=c.p;
  psui::SpatialFrame<Image> frame;
  if(c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass pass,const Cancel& cancel){
   psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,pass);
   const auto& source=c.cache.get(c.source,result.space,cancel);
   if(!cancel())render(source,p,result.image,cancel,result.space.step);
   return result;
  })){
   if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;c.error=noErr;}
   else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
void draw(psui::LibraryPanel& ui,Context& c){
 const Parameters factory;auto& p=c.p;ui.parameter_defaults(p,factory);
 const float s=ui.scale();bool changed=false,preset_applied=false;
 ImGui::BeginChild("preview-side",{280*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_document_bounds(c.host),280*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-175*s,115*s,310*s));
 update(ui,c);ui.image_view(c.view,c.proxy);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;
 c.worker.controls(ui,c.dirty);
 if(psui::preview_background_control(ui,c.background))c.display_dirty=true;
 ui.preview_zoom(c.view);
 ui.items["longshadow.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.length),p.shadow_only?1.f:0.f};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.selected=0;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice("预设 / Preset",c.selected,presets,6)&&c.selected){p=preset(c.selected-1);changed=preset_applied=true;}
 auto number=[&](const char* label,double& val,double lo,double hi,double a,double b){changed|=ui.number(label,val,lo,hi,a,b);};
 if(ui.section("投影 / Shadow",true)){
  number("方向（度） / Angle (deg)",p.angle,-360,360,-180,180);
  number("长度（像素） / Length (px)",p.length,0,10000,0,600);
  number("起始间距（像素） / Start Gap (px)",p.gap,0,2000,0,150);
  changed|=ui.choice("颜色模式 / Color Mode",p.color_mode,colors,2);
  if(p.color_mode==0)changed|=psui::color_control(ui,c.host,"颜色 / Color",p.color.data());
  else number("源色明暗（%） / Source Brightness (%)",p.source_brightness,0,200,0,200);
  number("不透明度（%） / Opacity (%)",p.opacity,0,100,0,100);
  number("扩张（像素） / Expansion (px)",p.expansion,0,100,0,32);
  number("尾部渐隐（%） / Tail Fade (%)",p.fade,0,100,0,100);
  changed|=ui.check("仅输出阴影 / Shadow Only",p.shadow_only);
 }
 if(ui.section("柔边 / Soft Edges",true)){
  number("近端模糊（像素） / Near Blur (px)",p.blur_near,0,160,0,48);
  number("远端模糊（像素） / Far Blur (px)",p.blur_far,0,160,0,64);
 }
 if(ui.section("源裁切 / Source Crop",false)){
  changed|=ui.check("启用裁切 / Enable Crop",p.crop);
  ImGui::BeginDisabled(!p.crop);
  number("起点（%） / Start (%)",p.crop_left,0,100,0,100);
  number("终点（%） / End (%)",p.crop_right,0,100,0,100);
  ImGui::EndDisabled();
 }
 if(ui.section("投影遮罩 / Shadow Mask",false)){
  changed|=ui.choice("形状 / Shape",p.mask,masks,3);
  ImGui::BeginDisabled(!p.mask);
  number("中心 X（%） / Center X (%)",p.mask_x,-100,200,0,100);
  number("中心 Y（%） / Center Y (%)",p.mask_y,-100,200,0,100);
  number("宽度（%） / Width (%)",p.mask_width,0,200,0,150);
  number("高度（%） / Height (%)",p.mask_height,0,200,0,150);
  number("羽化（%） / Feather (%)",p.mask_feather,0,100,0,100);
  changed|=ui.check("反转遮罩 / Invert Mask",p.invert_mask);
  ImGui::EndDisabled();
 }
 ImGui::TextWrapped("%s",ui.text("使用带透明背景的文字、形状或抠图；智能对象内请为投影预留透明空间。 / Use text, shapes, or cutouts on transparency. Leave room for shadows inside Smart Objects.").c_str());
 ImGui::EndChild();
 if(changed){c.dirty=true;if(!preset_applied)c.selected=0;}
}
}}
namespace longshadow {
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* f,const Image& source,Parameters& p){
 Context c(p,f,source);
 const char* glyphs="自定义经典斜影柔和长靛蓝实暖砖投方向度长度像素颜色不透明扩张尾部渐隐仅输出阴源裁切启用起点终遮罩形状无矩椭圆中心宽高羽化反转使用带背景文字或抠图智能对象内请为预留空间失败重置取消悬浮软始距模式纯拉伸暗边近端模糊远";
 const auto answer=psui::run_library_panel(module,owner,f,title,1020,800,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&info);
 c.worker.stop();
 if(answer==IDOK&&!c.error)p=c.p;
 return answer==IDOK&&c.error?c.error:answer;
}
}
