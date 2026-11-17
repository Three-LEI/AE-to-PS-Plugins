#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "ascii_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "../../native/common/stage_cache.hpp"
namespace asciifx {namespace {
constexpr const char* charsets[]={"经典 ASCII / Classic ASCII","精细字符 / Detailed Characters","二进制 / Binary","数字 / Digits"};
constexpr const char* fonts[]={"Consolas","Courier New","Lucida Console"};
constexpr const char* dithers[]={"关闭 / None","有序抖动 / Ordered","误差扩散 / Error Diffusion"};
constexpr const char* colours[]={"单色 / Monochrome","原图彩色 / Source Colors"};
constexpr const char* backgrounds[]={"实色 / Solid Color","透明 / Transparent"};
constexpr const char* presets[]={"自定义 / Custom","绿色终端 / Green Terminal","黑字纸面 / Ink on Paper","彩色字符 / Color Characters","二进制屏幕 / Binary Screen","密集文本 / Dense Text","透明字符 / Transparent Characters"};
struct Frame {Image image;psui::PreviewSpace space;std::string family;};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;int selected=0;std::string family;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;
 pscpu::StageCache<std::array<int,3>,GlyphAtlas> fonts_cache{8u*1024*1024};
 psui::AdaptivePreview<Frame> worker;
 Context(Parameters v,FilterRecord* f,const Image& s):p(v),host(f),source(s){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;Frame frame;
  if(c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass pass,const Cancel& cancel){
   Frame result;result.space=psui::preview_space(c.source,pass);const auto& s=result.space;const auto& src=c.cache.get(c.source,s,cancel);if(cancel())return result;
   // Fixed upper byte cost covers integral masks, names and vector metadata.
   const auto* atlas=c.fonts_cache.get({p.font,p.charset,p.bold?1:0},3u*1024*1024,[&](GlyphAtlas& value){value=make_atlas(p,cancel);return value.valid()&&!cancel();});if(!atlas||cancel())return result;
   auto q=p;if(s.step>1){q.cell_width/=s.step;q.shift_x=s.x(p.shift_x);q.shift_y=s.y(p.shift_y);}
   result.family=atlas->family;render(src,q,*atlas,result.image,cancel);return result;
  })){if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.family=std::move(frame.family);c.display_dirty=true;c.error=noErr;}else c.error=filterBadParameters;}
 }catch(...){c.error=memFullErr;}
 psui::compose_spatial_preview(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
void draw(psui::LibraryPanel& ui,Context& c){
 const Parameters factory;ui.parameter_defaults(c.p,factory);const float s=ui.scale();bool changed=false,applied=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{250*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),250*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-180*s,100*s,280*s));update(ui,c);ui.image_view(c.view,c.proxy);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["ascii.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.cell_width),float(p.dither)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请切换字体、重置或取消。 / Preview failed. Change font, reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.selected=0;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice("预设 / Preset",c.selected,presets,7)&&c.selected){p=preset(c.selected-1);changed=applied=true;}
 if(ui.section("字符 / Characters",true)){
  changed|=ui.choice("字符集 / Character Set",p.charset,charsets,4);changed|=ui.choice("字体 / Font",p.font,fonts,3);changed|=ui.check("粗体 / Bold",p.bold);
  changed|=ui.number("字符宽度（像素） / Cell Width (px)",p.cell_width,4,256,4,64);
  changed|=ui.number("单元高宽比 / Cell Height Ratio",p.cell_aspect,.5,3,.5,3);
  changed|=ui.number("字符填充（%） / Character Fill (%)",p.fill,20,100,20,100);
 }
 if(ui.section("明暗与抖动 / Tone and Dither",true)){
  changed|=ui.choice("抖动方式 / Dither Mode",p.dither,dithers,3);
  changed|=ui.number("抖动强度（%） / Dither Amount (%)",p.dither_amount,0,100,0,100);
  changed|=ui.number("明亮度 / Brightness",p.brightness,-100,100,-100,100);
  changed|=ui.number("对比度（%） / Contrast (%)",p.contrast,0,400,0,200);
  changed|=ui.number("伽马 / Gamma",p.gamma,.1,5,.1,3);changed|=ui.check("反向映射 / Invert Mapping",p.invert);
 }
 if(ui.section("颜色与输出 / Color and Output",false)){
  changed|=ui.choice("字符颜色 / Character Color",p.color_mode,colours,2);
  changed|=psui::color_control(ui,c.host,"字符色 / Ink Color",p.ink.data());changed|=psui::color_control(ui,c.host,"底色 / Paper Color",p.paper.data());
  changed|=ui.number("字符亮度（%） / Character Gain (%)",p.color_gain,0,400,0,200);
  changed|=ui.choice("背景 / Background",p.background,backgrounds,2);changed|=ui.number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 }
 if(ui.section("位置 / Position",false))changed|=psui::point_control(ui,"字符网格偏移 / Grid Shift",p.shift_x,p.shift_y);
 if(!c.family.empty())ImGui::TextDisabled("%s: %s",ui.text("实际字体 / Actual Typeface").c_str(),c.family.c_str());
 ImGui::TextWrapped("%s",ui.text("字符按实际疏密映射亮度。可放大到 100% 检查字形；透明背景适合叠层。 / Character density follows brightness. Zoom to 100% to inspect glyphs; transparent output supports compositing.").c_str());
 ImGui::EndChild();if(changed){if(!applied)c.selected=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const char* glyphs="字符抖动 自定义 绿色终端 黑字纸面 彩色字符 二进制屏幕 密集文本 透明字符 经典 精细字符 二进制 数字 关闭 有序抖动 误差扩散 单色 原图彩色 实色 透明 字符集 字体 粗体 字符宽度 像素 单元高宽比 字符填充 明暗与抖动 抖动方式 抖动强度 明亮度 对比度 伽马 反向映射 颜色与输出 字符颜色 字符色 底色 字符亮度 背景 效果混合 位置 字符网格偏移 实际字体 字符按实际疏密映射亮度 可放大到检查字形 透明背景适合叠层 预览失败请切换字体重置或取消";
 auto answer=psui::run_library_panel(m,owner,host,title,940,800,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
