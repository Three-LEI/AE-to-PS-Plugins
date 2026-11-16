#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "lens_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "psui/preview_background.hpp"
#include <cmath>
namespace lens {namespace {
constexpr const char* axes[]={"水平 / Horizontal","垂直 / Vertical","对角线 / Diagonal"};
constexpr const char* labels[]={"视场 (FOV) / Field Of View","反转镜头扭曲 / Reverse Lens Distortion","FOV 方向 / FOV Orientation","视图中心 / View Center","最佳像素（非可逆） / Optimal Pixels (Non-reversible)","中心 / Center","大小 / Size","收敛度 / Convergence","中心居中 / Center","调整大小：关闭（PS画布固定）。保留边缘需先扩展画布。 / Resize: Off (fixed Photoshop canvas). Enlarge the canvas first to retain edges.","最佳像素使用同源参考拟合，细节可能与AE不同。 / Optimal Pixels is fitted to native references; details can differ from AE.","右键缩览图设置中心。 / Right-click the preview to set the center.","正在计算… / Rendering preview…","计算失败，请重置或取消。 / Rendering failed. Reset or cancel.","独立复刻，不调用AE或原版插件。 / Independent reconstruction; no AE or original plug-in dependency."};
#include "preview_adapter.hpp"
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;ULONGLONG due=0;
  psui::PreviewSpace adaptive_mapping;psui::DraftImageCache<Image> adaptive_cache;
 psui::ProxyImage adaptive_selection_geometry;std::vector<float> adaptive_selection;
 psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters values,FilterRecord* f,const Image& src):p(values),host(f),source(src){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{
  const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(!c.error&&c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass draft,const std::function<bool()>& cancel){
   psui::SpatialFrame<Image> frame;const auto space=psui::preview_space(c.source,draft);frame.space=space;
   const auto& source=c.adaptive_cache.get(c.source,space,cancel);
   if(cancel())return frame;const auto q=space.step>1?draft_parameters(p,space):p;
   render(source,q,frame.image,cancel);
   return frame;
  })){
   if(frame.image.valid()){c.last=std::move(frame.image);c.adaptive_mapping=frame.space; c.display_dirty=true;}else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.adaptive_selection,c.adaptive_selection_geometry,c.display,c.source,c.last,c.adaptive_mapping,c.preview,c.display_dirty,c.error);
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, defaults(effect_kind,c.source.width,c.source.height,c.source.left,c.source.top));
 const float s=ui.scale();bool changed=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-ImGui::GetFrameHeight()-ImGui::GetStyle().ItemSpacing.y-120*s,110*s,205*s));update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["lens.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
  const auto mouse=ImGui::GetMousePos();p.cx=c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01);p.cy=c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01);changed=true;
 }
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[13]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();c.adaptive_cache.clear();p=defaults(effect_kind,c.source.width,c.source.height,c.source.left,c.source.top);c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(effect_kind==Kind::optics){changed|=ui.number(labels[0],p.fov,0,179.99998,0,179);changed|=ui.check(labels[1],p.reverse);changed|=ui.choice(labels[2],p.axis,axes,3);changed|=psui::point_control(ui,labels[3],p.cx,p.cy);changed|=ui.check(labels[4],p.optimal);ImGui::TextWrapped("%s",ui.text(labels[9]).c_str());if(p.optimal)ImGui::TextWrapped("%s",ui.text(labels[10]).c_str());}
 else {changed|=psui::point_control(ui,labels[5],p.cx,p.cy);changed|=ui.number(labels[6],p.size,0,500,0,200);changed|=ui.number(labels[7],p.convergence,-200,100,-200,100);}
 if(ui.button("center",labels[8],-1)){p.cx=c.source.left+c.source.width*.5;p.cy=c.source.top+c.source.height*.5;changed=true;}
 ImGui::TextWrapped("%s",ui.text(labels[11]).c_str());ImGui::TextWrapped("%s",ui.text(labels[14]).c_str());ImGui::EndChild();
 if(changed){c.dirty=true;c.error=noErr;c.due=GetTickCount64()+35;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,Parameters& p){Context c(p,f,src);std::string glyphs;for(auto label:labels)glyphs+=label;for(auto axis:axes)glyphs+=axis;const auto answer=psui::run_library_panel(m,owner,f,title,760,520,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);c.worker.stop();c.adaptive_cache.clear();if(answer==IDOK&&!c.error)p=c.p;return answer;}
}
