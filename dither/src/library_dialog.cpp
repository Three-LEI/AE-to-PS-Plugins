#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "dither_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "preview_area.hpp"
namespace dither {namespace {
constexpr const char* algorithms[]={"有序抖动 / Ordered","误差扩散 FS / Floyd-Steinberg","Atkinson","关闭抖动 / None"};
constexpr const char* patterns[]={"Bayer 2×2","Bayer 4×4","Bayer 8×8","聚集点阵 / Clustered Dots","棋盘 / Checkerboard","横线 / Horizontal Lines","竖线 / Vertical Lines","斜线 / Diagonal Lines","十字 / Cross"};
constexpr const char* palettes[]={"双色 / Duotone","灰阶 / Grayscale","RGB 限色 / RGB Levels","复古绿屏 / Retro Green","蓝色月光 / Blue Moon","自定义四色 / Custom Four Colors"};
constexpr const char* counts[]={"2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"};
constexpr const char* presets[]={"自定义 / Custom","黑白像素 / Monochrome Pixels","四阶灰度 / Four Grays","复古绿屏 / Retro Green","蓝色月光 / Blue Moon","八色像素 / Eight Colors","图形竖线 / Graphic Lines"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;int selected=0;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;preview::SourceAreaCache before_cache;
 std::vector<std::uint8_t> display;std::vector<float> selection;bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;
 psui::PreviewSpace mapping;psui::DraftImageCache<Image> cache;psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters v,FilterRecord* f,const Image& s):p(v),host(f),source(s){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{const auto p=c.p;psui::SpatialFrame<Image> frame;
  if(c.worker.update(c.dirty,frame,[&c,p](psui::PreviewPass pass,const Cancel& cancel){psui::SpatialFrame<Image> result;result.space=psui::preview_space(c.source,pass);const auto& s=result.space;const auto& src=c.cache.get(c.source,s,cancel);if(cancel())return result;auto q=p;if(s.step>1){q.pixel_size/=s.step;q.shift_x=s.x(p.shift_x);q.shift_y=s.y(p.shift_y);}render(src,q,result.image,cancel);return result;})){
   if(frame.image.valid()){c.last=std::move(frame.image);c.mapping=frame.space;c.display_dirty=true;c.error=noErr;}else c.error=filterBadParameters;
  }
 }catch(...){c.error=memFullErr;}
 preview::compose(ui,c.host,c.view,c.proxy,c.background,c.selection,c.selection_geometry,c.display,c.before_cache,c.source,c.last,c.mapping,c.preview,c.display_dirty,c.error);
}
void draw(psui::LibraryPanel& ui,Context& c){
 const Parameters factory;ui.parameter_defaults(c.p,factory);const float s=ui.scale();bool changed=false,applied=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{250*s,0},0,ImGuiWindowFlags_NoScrollbar);const float footer=3*(ui.button_height()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),250*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-180*s,100*s,280*s));update(ui,c);ui.image_view(c.view,c.proxy);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["dither.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.pixel_size),float(p.algorithm)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.selected=0;c.error=noErr;changed=true;}ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice("预设 / Preset",c.selected,presets,7)&&c.selected){p=preset(c.selected-1);changed=applied=true;}
 if(ui.section("抖动 / Dither",true)){
  changed|=ui.choice("算法 / Algorithm",p.algorithm,algorithms,4);ImGui::BeginDisabled(p.algorithm!=0);changed|=ui.choice("图案 / Pattern",p.pattern,patterns,9);ImGui::EndDisabled();
  changed|=ui.number("像素尺寸 / Pixel Size",p.pixel_size,1,64,1,24,true,true);
  changed|=ui.number("抖动强度（%） / Dither Amount (%)",p.amount,0,200,0,200);
  ImGui::BeginDisabled(p.algorithm!=1&&p.algorithm!=2);changed|=ui.check("蛇形扫描 / Serpentine",p.serpentine);ImGui::EndDisabled();
 }
 if(ui.section("调色板 / Palette",true)){
  if(ui.choice("颜色模式 / Color Mode",p.palette,palettes,6)){if(p.palette==2)p.levels=std::min(p.levels,6);changed=true;}
  ImGui::BeginDisabled(p.palette!=1&&p.palette!=2);changed|=ui.choice("级数 / Levels",p.levels,counts,p.palette==2?7:15);ImGui::EndDisabled();
  if(p.palette==0||p.palette==5){changed|=psui::color_control(ui,c.host,"颜色 1 / Color 1",p.colors[0].data());changed|=psui::color_control(ui,c.host,"颜色 2 / Color 2",p.colors[1].data());}
  if(p.palette==5){changed|=psui::color_control(ui,c.host,"颜色 3 / Color 3",p.colors[2].data());changed|=psui::color_control(ui,c.host,"颜色 4 / Color 4",p.colors[3].data());}
 }
 if(ui.section("明暗与输出 / Tone and Output",true)){
  changed|=ui.number("明亮度 / Brightness",p.brightness,-100,100,-100,100);changed|=ui.number("对比度（%） / Contrast (%)",p.contrast,0,400,0,200);changed|=ui.number("伽马 / Gamma",p.gamma,.1,5,.1,3);changed|=ui.number("饱和度（%） / Saturation (%)",p.saturation,0,200,0,200);
  changed|=ui.check("线性光计算 / Linear Light",p.linear_light);changed|=ui.number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 }
 if(ui.section("位置 / Position",false))changed|=psui::point_control(ui,"像素网格偏移 / Pixel Grid Shift",p.shift_x,p.shift_y);
 ImGui::TextWrapped("%s",ui.text("100% 为标准抖动，降低可保留色块，提高可强调纹理。缩小预览会平均细像素，100% 可检查像素图案。 / 100% is standard dithering. Lower values keep bands; higher values emphasize texture. Zoom to 100% to inspect the pixel pattern.").c_str());
 ImGui::EndChild();if(changed){if(!applied)c.selected=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const char* glyphs="像素抖动 自定义 黑白像素 四阶灰度 复古绿屏 蓝色月光 八色像素 图形竖线 有序抖动 误差扩散 关闭抖动 聚集点阵 棋盘 横线 竖线 斜线 十字 双色 灰阶 限色 自定义四色 算法 图案 像素尺寸 抖动强度 蛇形扫描 调色板 颜色模式 级数 颜色 明暗与输出 明亮度 对比度 伽马 饱和度 线性光计算 效果混合 位置 像素网格偏移 为标准抖动降低可保留色块提高可强调纹理缩小预览会平均细像素可检查像素图案 预览失败请重置或取消";
 auto answer=psui::run_library_panel(m,owner,host,title,940,860,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
