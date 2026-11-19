#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "etching_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "preview_area.hpp"
namespace etching {namespace {
constexpr const char* presets[]={"自定义 / Custom","经典蚀刻 / Classic Etching","绿钞雕版 / Green Engraving","复古棕墨 / Vintage Sepia","单线肖像 / Single-line Portrait","交叉排线 / Crosshatch","蓝墨版画 / Blue Ink"};
constexpr const char* modes[]={"双线蚀刻 / Double Etching","单线版画 / Single Engraving","暗部交叉排线 / Tonal Crosshatch"};
constexpr const char* backgrounds[]={"纸色背景 / Paper Color","仅墨线（透明） / Ink Only (Transparent)"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;int selected=0;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;preview::SourceAreaCache before_cache;
 std::vector<std::uint8_t> display;std::vector<float> selection;bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters v,FilterRecord* f,const Image& s):p(v),host(f),source(s){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass pass,const Cancel& cancel){psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,pass);const auto& src=c.cache.get(c.source,result.space,cancel);if(cancel())return result;render(src,p,result.image,cancel,{double(c.source.left),double(c.source.top),double(c.source.width),double(c.source.height),result.space.step});return result;})){
   if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;c.error=noErr;}else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 preview::compose(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.before_cache,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
void draw(psui::LibraryPanel& ui,Context& c){
 const Parameters factory;ui.parameter_defaults(c.p,factory);const float s=ui.scale();bool changed=false,applied=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{310*s,0},0,ImGuiWindowFlags_NoScrollbar);const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_document_bounds(c.host),310*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-190*s,120*s,380*s));update(ui,c);ui.image_view(c.view,c.proxy);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["etching.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.frequency),float(p.mode)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.selected=0;c.error=noErr;changed=true;}ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 auto number=[&](const char* label,double& value,double lo,double hi,double a,double b){changed|=ui.number(label,value,lo,hi,a,b);};
 if(ui.choice("预设 / Preset",c.selected,presets,7)&&c.selected){const auto x=p.shift_x,y=p.shift_y;p=preset(c.selected-1);p.shift_x=x;p.shift_y=y;changed=applied=true;}
 if(ui.section("主要 / Main",true)){
  changed|=ui.choice("线刻模式 / Etching Mode",p.mode,modes,3);number("线条频率 / Lines Frequency",p.frequency,0,1000,5,200);number("整体角度（度） / Lines Angle (deg)",p.angle,-1e9,1e9,-180,180);
  number("线条锐度 / Lines Sharpness",p.sharpness,0,30,0,15);number("线宽增减（%） / Add Width (%)",p.add_width,-100,100,-50,50);number("源图平滑（像素） / Smooth Source (px)",p.smooth_source,0,200,0,20);
 }
 if(ui.section("两组线条 / Line Sets",false)){
  number("第一组频率倍率 / Lines 1 Frequency",p.frequency1,0,8,0,4);number("第一组相对角度（度） / Lines 1 Angle (deg)",p.angle1,-1e9,1e9,-180,180);
  ImGui::BeginDisabled(p.mode==1);number("第二组频率倍率 / Lines 2 Frequency",p.frequency2,0,8,0,4);number("第二组相对角度（度） / Lines 2 Angle (deg)",p.angle2,-1e9,1e9,-180,180);number("交叉排线强度（%） / Crosshatch Amount (%)",p.cross_amount,0,100,0,100);ImGui::BeginDisabled(p.mode!=2);number("交叉起始暗度 / Crosshatch Start",p.cross_start,0,.99,0,.9);ImGui::EndDisabled();ImGui::EndDisabled();
 }
 if(ui.section("波形与扭曲 / Wave and Warp",false)){
  number("波形幅度 / Wave Amplitude",p.wave_amp,0,.5,0,.2);number("波形频率 / Wave Frequency",p.wave_frequency,0,30,0,10);number("亮度扭曲 / Warp Amplitude",p.warp_amp,-.5,.5,-.15,.15);number("扭曲平滑 / Warp Smooth",p.warp_smooth,0,.5,0,.15);changed|=psui::point_control(ui,"线条偏移 / Lines Shift",p.shift_x,p.shift_y);
 }
 if(ui.section("轮廓细节 / Source Edges",false)){
  number("边缘强度 / Edges Strength",p.edge_strength,0,3,0,2);ImGui::BeginDisabled(p.edge_strength==0);number("边缘阈值 / Edges Threshold",p.edge_threshold,0,2,0,1);number("边缘宽度（像素） / Edges Width (px)",p.edge_width,0,200,0,15);number("边缘锐度 / Edges Sharpness",p.edge_sharpness,0,30,0,15);ImGui::EndDisabled();
 }
 if(ui.section("着色与纸张 / Color and Paper",true)){
  changed|=psui::color_control(ui,c.host,"墨线颜色 / Ink Color",p.dark.data());changed|=psui::color_control(ui,c.host,"纸张颜色 / Paper Color",p.light.data());number("保留源色（%） / Source Color (%)",p.source_color,0,100,0,100);number("纸张颗粒（%） / Paper Grain (%)",p.paper_grain,0,100,0,60);
  ImGui::BeginDisabled(p.paper_grain==0);number("纸纹尺寸（像素） / Paper Grain Size (px)",p.paper_scale,.5,100,.5,12);double seed=p.seed;ui.inherit_number_default("随机种子 / Random Seed",p.seed);if(ui.number("随机种子 / Random Seed",seed,0,4294967295.,0,1000,true,true)){p.seed=std::uint32_t(seed);changed=true;}ImGui::EndDisabled();
 }
 if(ui.section("色调与输出 / Tone and Output",false)){
  number("明亮度 / Brightness",p.brightness,-100,100,-100,100);number("对比度（%） / Contrast (%)",p.contrast,0,400,0,200);number("伽马 / Gamma",p.gamma,.1,5,.1,3);changed|=ui.choice("背景 / Background",p.background,backgrounds,2);number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 }
 ImGui::TextWrapped("%s",ui.text("频率越高线条越密；100%缩放可检查细线。 / Higher frequency produces finer lines. Use 100% zoom to inspect them.").c_str());
 ImGui::EndChild();if(changed){if(!applied)c.selected=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const char* glyphs="蚀刻 自定义 经典蚀刻 绿钞雕版 复古棕墨 单线肖像 交叉排线 蓝墨版画 双线蚀刻 单线版画 暗部交叉排线 纸色背景 仅墨线 透明 预览失败请重置或取消 主要 线刻模式 线条频率 整体角度 度 线条锐度 线宽增减 源图平滑 像素 两组线条 第一组频率倍率 第一组相对角度 第二组频率倍率 第二组相对角度 交叉排线强度 交叉起始暗度 波形与扭曲 波形幅度 波形频率 亮度扭曲 扭曲平滑 线条偏移 轮廓细节 边缘强度 边缘阈值 边缘宽度 边缘锐度 着色与纸张 墨线颜色 纸张颜色 保留源色 纸张颗粒 纸纹尺寸 随机种子 色调与输出 明亮度 对比度 伽马 背景 效果混合 频率越高线条越密缩放可检查细线";
 const auto answer=psui::run_library_panel(m,owner,host,title,1080,900,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
