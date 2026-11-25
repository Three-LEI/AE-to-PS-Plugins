#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "filmdamage_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "preview_area.hpp"
namespace filmdamage {namespace {
constexpr const char* presets[]={"自定义 / Custom","经典胶片 / Classic Film","轻微磨损 / Subtle Wear","黑白纪实 / Monochrome Archive","泛黄旧照 / Aged Sepia","放映机损伤 / Projector Damage","暖色漏光 / Warm Light Leak","细密划痕 / Fine Scratches","废弃胶卷 / Abandoned Film"};
constexpr const char* borders[]={"延伸边缘 / Extend","循环 / Wrap","透明 / Transparent"};
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
 const Parameters factory;ui.parameter_defaults(c.p,factory);bool changed=false,applied=false;auto& p=c.p;
 const float footer = ui.filter_footer_height();
 ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);update(ui,c);
 ui.image_view(c.view, c.proxy);
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 ui.items["filmdamage.state"]={c.last.valid()?1.f:0.f,float(c.error),float(p.grain),float(p.dust)};
 if(c.error)ImGui::TextWrapped("%s",ui.text("预览失败，请重置或取消。 / Preview failed. Reset or cancel.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.selected=0;c.error=noErr;changed=true;}ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 auto number=[&](const char* label,double& value,double lo,double hi,double a,double b){changed|=ui.number(label,value,lo,hi,a,b);};
 auto integer=[&](const char* label,auto& value,int lo,int hi,int slider){ui.inherit_number_default(label,value);double d=value;if(ui.number(label,d,lo,hi,lo,slider,true,true)){value=static_cast<std::remove_reference_t<decltype(value)>>(d);changed=true;}};
 if(ui.choice("预设 / Preset",c.selected,presets,9)&&c.selected){p=preset(c.selected-1);changed=applied=true;}
 if(ui.button("reroll-damage","换一组损伤 / Reroll Damage",-1)){p.damage_variation=(p.damage_variation+1)%1000001;changed=true;}
 integer("损伤分布 / Damage Layout",p.damage_variation,0,1000000,100);
 if(ui.section("颗粒 / Film Grain",true)){
  number("颗粒强度（%） / Grain Amount (%)",p.grain,0,100,0,40);number("颗粒尺寸（像素） / Grain Size (px)",p.grain_size,.25,40,.25,8);number("彩色颗粒（%） / Color Grain (%)",p.grain_color,0,100,0,100);changed|=ui.check("单色颗粒 / Monochrome Grain",p.grain_mono);
  number("暗部颗粒（%） / Grain in Shadows (%)",p.grain_darks,0,100,0,100);number("亮部颗粒（%） / Grain in Highlights (%)",p.grain_lights,0,100,0,100);
  number("颗粒柔和度（%） / Grain Softness (%)",p.grain_softness,0,100,0,100);
 }
 if(ui.section("灰尘与污渍 / Dust and Stains",false)){
  integer("灰尘数量 / Dust Count",p.dust,0,2000,500);number("灰尘尺寸（像素） / Dust Size (px)",p.dust_size,.1,200,.1,15);number("灰尘不透明度（%） / Dust Opacity (%)",p.dust_opacity,0,100,0,100);number("白色灰尘比例（%） / White Dust (%)",p.dust_white,0,100,0,100);
  integer("污渍数量 / Stain Count",p.stains,0,100,30);number("污渍尺寸（像素） / Stain Size (px)",p.stain_size,.1,2000,10,500);number("污渍不透明度（%） / Stain Opacity (%)",p.stain_opacity,0,100,0,100);changed|=psui::color_control(ui,c.host,"污渍颜色 / Stain Color",p.stain_color.data());
  number("污渍纹理（%） / Stain Texture (%)",p.stain_texture,0,100,0,100);
  number("污渍边缘粗糙度（%） / Stain Edge Roughness (%)",p.stain_edge_roughness,0,100,0,100);
  number("污渍边缘尺寸（%） / Stain Edge Size (%)",p.stain_edge_size,5,250,5,250);
 }
 if(ui.section("毛发 / Hairs",false)){
  integer("毛发数量 / Hair Count",p.hairs,0,100,20);number("毛发宽度（像素） / Hair Width (px)",p.hair_width,.1,64,.1,6);number("毛发长度（像素） / Hair Length (px)",p.hair_length,.1,2000,10,600);number("毛发弯曲（%） / Hair Curl (%)",p.hair_curl,0,200,0,100);number("毛发不透明度（%） / Hair Opacity (%)",p.hair_opacity,0,100,0,100);
 }
 if(ui.section("划痕 / Scratches",false)){
  integer("划痕数量 / Scratch Count",p.scratches,0,200,50);number("划痕宽度（像素） / Scratch Width (px)",p.scratch_width,.1,64,.1,8);number("划痕长度（%） / Scratch Length (%)",p.scratch_length,0,200,0,150);number("划痕不透明度（%） / Scratch Opacity (%)",p.scratch_opacity,0,100,0,100);number("白色划痕比例（%） / White Scratches (%)",p.scratch_white,0,100,0,100);
  number("划痕粗糙（像素） / Scratch Roughness (px)",p.scratch_roughness,0,200,0,15);number("划痕断裂（%） / Scratch Gaps (%)",p.scratch_gaps,0,100,0,100);number("划痕角度（度） / Scratch Angle (deg)",p.scratch_angle,-1e9,1e9,-90,90);number("划痕区域中心 / Scratch Area Center",p.scratch_center,-200,200,-100,100);number("划痕区域宽度（%） / Scratch Area Width (%)",p.scratch_spread,0,200,0,100);changed|=ui.check("垂直翻转损伤 / Flip Damage Vertically",p.flip);
 }
 if(ui.section("色调与暗角 / Color and Vignette",false)){
  number("饱和度（%） / Saturation (%)",p.saturation,0,300,0,200);number("对比度（%） / Contrast (%)",p.contrast,0,300,0,200);number("曝光（档） / Exposure (stops)",p.exposure,-10,10,-3,3);number("褪色（%） / Fade (%)",p.fade,0,100,0,50);changed|=psui::color_control(ui,c.host,"高光染色 / Highlight Tint",p.tint.data());
  number("暗角强度（%） / Vignette (%)",p.vignette,0,100,0,100);number("暗角半径（%） / Vignette Radius (%)",p.vignette_radius,1,200,10,150);number("暗角柔和（%） / Vignette Softness (%)",p.vignette_softness,0,100,0,100);number("漏光强度（%） / Light Leak (%)",p.leak,0,100,0,100);number("漏光方向（度） / Leak Angle (deg)",p.leak_angle,-1e9,1e9,-180,180);
 }
 if(ui.section("高光红晕 / Highlight Halation",false)){
  number("红晕强度（%） / Halation Amount (%)",p.halation,0,100,0,100);
  number("红晕半径（像素） / Halation Radius (px)",p.halation_radius,.5,160,.5,64);
  number("红晕阈值（%） / Halation Threshold (%)",p.halation_threshold,0,100,0,100);
 }
 if(ui.section("画面与输出 / Frame and Output",false)){
  number("位移抖动（像素） / Gate Weave (px)",p.shake,0,200,0,20);number("旋转抖动（度） / Gate Rotation (deg)",p.tilt,0,30,0,3);number("失焦（像素） / Defocus (px)",p.defocus,0,200,0,20);number("亮度波动（%） / Flicker (%)",p.flicker,0,200,0,100);
  changed|=ui.choice("边界 / Border",p.edge,borders,3);changed|=ui.check("保留原图透明度 / Preserve Source Alpha",p.preserve_alpha);number("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 }
 integer("随机种子 / Random Seed",p.seed,0,2147483647,1000);integer("样式变化 / Variation",p.variation,0,1000000,100);
 ImGui::TextWrapped("%s",ui.text("独立重建的静态胶片损伤。 / Independently reconstructed static film damage.").c_str());
 ImGui::EndChild();if(changed){if(!applied)c.selected=0;c.dirty=true;c.error=noErr;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const char* glyphs="自定义经典胶片轻微磨损黑白纪实泛黄旧照放映机伤暖色漏光细密划痕废弃卷延伸边缘循环透明预览失败，请重置或取消。确全部设颗粒强度（）尺寸像素彩单暗亮灰尘与污渍数量不比例颜毛发宽长弯曲粗糙断裂角区域中心垂直翻转调饱和对曝档褪高染半径柔方向画面输出位移抖动旋焦波界保留原图效果混合随种子样式变化独立建的静态换一组分布红晕阈值纹理";
 const auto answer=psui::run_filter_panel(m,owner,host,title,1100,920,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs,&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
