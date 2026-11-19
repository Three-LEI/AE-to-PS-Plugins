#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "blinds_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include <cmath>
#include "preview_renderer.hpp"
namespace blinds {namespace {
constexpr const char* presets[]={"自定义 / Custom","半开竖向 / Vertical Half","半开横向 / Horizontal Half","斜向柔边 / Soft Diagonal","细密条纹 / Fine Stripes"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;int preset=0;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters v,FilterRecord* f,const Image& s):p(v),host(f),source(s){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{
  const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(!c.error&&c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass draft,const Cancel& cancel){
   return preview_mask(c.source,p,draft,cancel);
  })){if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;}else c.error=filterBadParameters;}
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error,nullptr,true);
}
void draw(psui::LibraryPanel& ui,Context& c){
 const auto factory=defaults(c.source.width,c.source.height,c.source.left,c.source.top);ui.parameter_defaults(c.p,factory);
 const float s=ui.scale();bool changed=false,preset_applied=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{230*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_document_bounds(c.host),230*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-180*s,100*s,230*s));update(ui,c);ui.image_view(c.view,c.proxy);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["blinds.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.completion),float(p.direction)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.preset=0;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice("预设 / Preset",c.preset,presets,5)&&c.preset){p.completion=50;p.direction=c.preset==2?90:c.preset==3?30:0;p.width=c.preset==4?8:20;p.feather=c.preset==3?5:0;changed=preset_applied=true;}
 changed|=ui.number("过渡完成（%） / Transition Completion (%)",p.completion,0,100,0,100);
 changed|=ui.number("方向（度） / Direction (deg)",p.direction,-1e9,1e9,-180,180);
 changed|=ui.number("宽度（像素） / Width (px)",p.width,1,32000,1,200);
 changed|=ui.number("羽化（像素） / Feather (px)",p.feather,0,32000,0,100);
 if(ui.section("位置 / Position",false)){
  changed|=psui::point_control(ui,"条纹中心 / Stripe Center",p.cx,p.cy);
  if(ui.button("center","中心居中 / Center",-1)){p.cx=factory.cx;p.cy=factory.cy;changed=true;}
 }
 ImGui::TextWrapped("%s",ui.text("0% 保留原图，100% 完全隐藏；间隙显示下方图层。 / 0% keeps the source; 100% hides it. Gaps reveal layers below.").c_str());
 ImGui::TextWrapped("%s",ui.text("请使用普通图层或智能对象；背景层会以黑色呈现透明部分，锁定透明度时不能隐藏像素。 / Use a regular layer or smart object. Background layers flatten gaps to black; locked transparency cannot remove pixels.").c_str());
 ImGui::EndChild();if(changed){if(!preset_applied)c.preset=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);std::string glyphs="百叶窗 预设 半开竖向 半开横向 斜向柔边 细密条纹 过渡完成 方向 度 宽度 像素 羽化 位置 条纹中心 中心居中 保留原图 完全隐藏 间隙显示下方图层 请使用普通图层或智能对象 背景层会以黑色呈现透明部分 锁定透明度时不能隐藏像素 预览失败 请重置或取消";
 const auto answer=psui::run_library_panel(m,owner,host,title,780,540,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
