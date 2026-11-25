#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "halftone_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
namespace halftone {namespace {
constexpr const char* shapes[]={"圆点 / Round","方点 / Square","菱形 / Diamond","线条 / Lines"};
constexpr const char* polarities[]={"暗色网点 / Dark Dots","亮色网点 / Light Dots"};
constexpr const char* backgrounds[]={"纸色 / Paper Color","透明 / Transparent"};
constexpr const char* presets[]={"自定义 / Custom","标准网点 / Classic","漫画细网 / Manga","复古双色 / Vintage Duotone","冷色亮点 / Cool Light Dots","斜线印刷 / Diagonal Lines","透明油墨 / Transparent Ink"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;int selected=0;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters v,FilterRecord* f,const Image& s):p(v),host(f),source(s){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass pass,const Cancel& cancel){
   psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,pass);const auto& s=result.space;const auto& src=c.cache.get(c.source,s,cancel);if(cancel())return result;
   auto q=p;if(s.step>1){q.spacing/=s.step;q.smooth/=s.step;q.shift_x=s.x(p.shift_x);q.shift_y=s.y(p.shift_y);}
   render(src,q,result.image,cancel);return result;
  })){if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;c.error=noErr;}else c.error=filterBadParameters;}
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
void draw(psui::LibraryPanel& ui,Context& c){
 const Parameters factory;ui.parameter_defaults(c.p,factory);bool changed=false,applied=false;auto& p=c.p;
 const float footer = ui.filter_footer_height();
 ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);update(ui,c);
 const auto bounds = c.view.bounds;
 const double cx = .5 * (double(bounds.left) + bounds.right), cy = .5 * (double(bounds.top) + bounds.bottom);
 psui::PreviewAdjustments handles;
 handles.point("网纹偏移 / Screen Shift", p.shift_x, p.shift_y, psui::PreviewPointSpace::offset(cx, cy))
        .angle("角度 / Angle", p.angle, cx + p.shift_x, cy + p.shift_y, -1e9, 1e9);
 changed |= ui.image_view(c.view,c.proxy,handles);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["halftone.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.spacing),float(p.angle)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.selected=0;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice("预设 / Preset",c.selected,presets,7)&&c.selected){p=preset(c.selected-1);changed=applied=true;}
 if(ui.section("网纹 / Screen",true)){
  changed|=ui.choice("网点形状 / Dot Shape",p.shape,shapes,4);changed|=ui.choice("网点类型 / Dot Type",p.polarity,polarities,2);
  changed|=ui.number("网点间距（像素） / Dot Spacing (px)",p.spacing,2,1000,2,80);
  changed|=ui.number("角度（度） / Angle (deg)",p.angle,-1e9,1e9,-180,180);
  changed|=ui.number("宽高比 / Aspect Ratio",p.aspect,.1,10,.2,5);
  changed|=ui.number("边缘柔化（%） / Edge Softness (%)",p.softness,0,100,0,100);
 }
 if(ui.section("明暗与细节 / Tone and Detail",true)){
  changed|=ui.number("源平滑（像素） / Source Smooth (px)",p.smooth,0,200,0,20);
  changed|=ui.number("明亮度 / Brightness",p.brightness,-100,100,-100,100);
  changed|=ui.number("对比度（%） / Contrast (%)",p.contrast,0,400,0,200);
  changed|=ui.number("伽马 / Gamma",p.gamma,.1,5,.1,3);
 }
 if(ui.section("颜色与输出 / Color and Output",true)){
  changed|=psui::color_control(ui,c.host,"暗色 / Dark Color",p.dark.data());changed|=psui::color_control(ui,c.host,"亮色 / Light Color",p.light.data());
  changed|=ui.choice("背景 / Background",p.background,backgrounds,2);changed|=ui.number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 }
 if(ui.section("位置 / Position",false))changed|=psui::point_control(ui,"网纹偏移 / Screen Shift",p.shift_x,p.shift_y);
 ImGui::TextWrapped("%s",ui.text("间距越小网点越密。透明背景仅保留网点，适合叠加在其他图层上。 / Smaller spacing makes finer dots. Transparent background keeps only the dots for layer compositing.").c_str());
 ImGui::TextWrapped("%s",ui.text("独立重建的半调效果。背景层及锁定透明度会限制透明输出。 / Independently implemented halftone. Background layers and locked transparency restrict transparent output.").c_str());
 ImGui::EndChild();if(changed){if(!applied)c.selected=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const char* glyphs="网点印刷 自定义 标准网点 漫画细网 复古双色 冷色亮点 斜线印刷 透明油墨 圆点 方点 菱形 线条 暗色网点 亮色网点 纸色 透明 网纹 网点形状 网点类型 网点间距 像素 角度 宽高比 边缘柔化 明暗与细节 源平滑 明亮度 对比度 伽马 颜色与输出 暗色 亮色 背景 效果混合 位置 网纹偏移 间距越小网点越密 透明背景仅保留网点 适合叠加在其他图层上 独立重建的半调效果 背景层及锁定透明度会限制透明输出 预览失败请重置或取消";
 auto answer=psui::run_filter_panel(m,owner,host,title,940,800,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
