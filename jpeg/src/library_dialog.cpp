#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "jpeg_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/async_preview.hpp"
#include "psui/preview_background.hpp"
#include <cmath>
namespace jpegfx {namespace {
constexpr const char* presets[]={"自定义 / Custom","经典低质 JPEG / Classic JPEG","重度压缩 / Heavy Compression","传输损坏 / Transmission Errors","块错位 / Block Displacement","色彩损坏 / Chroma Damage","频率破碎 / Broken Frequencies"};
constexpr const char* chromas[]={"4:4:4 全色度 / 4:4:4 Full Chroma","4:2:2 水平减半 / 4:2:2 Half Horizontal","4:2:0 双向减半 / 4:2:0 Half Both","黑白 / Monochrome"};
constexpr const char* errors[]={"混合 / Mixed","频率噪声 / Coefficient Noise","亮度跳变 / DC Jumps","块错位 / Block Displacement","系数丢失 / Coefficient Dropout","色彩跳变 / Chroma Jumps"};
constexpr const char* masks[]={"亮度 / Luma","Alpha / Alpha"};
constexpr const char* none="无 / None";
struct Context {
 Parameters p;FilterRecord* host;const Image& source;const std::vector<psui::HostLayer>& layers;Image mask,last;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;int preset=0;OSErr error=noErr;ULONGLONG due=0;
 psui::AsyncPreview<Image> worker;
 Context(Parameters values,FilterRecord* f,const Image& src,const std::vector<psui::HostLayer>& ls):p(values),host(f),source(src),layers(ls){for(int i=0;i<6;++i)if(p==jpegfx::preset(i))preset=i+1;}
 bool read_mask(){worker.stop();mask={};if(!p.mask_key)return true;const psui::HostLayer* selected=nullptr;for(const auto& l:layers)if(l.name_key==p.mask_key){if(selected)return false;selected=&l;}if(!selected)return false;psui::LayerPixels pixels;if(psui::host_read_layer(host,*selected,host->documentInfo->bounds,0,pixels))return false;mask=from_host(std::move(pixels));return true;}
};
#include "preview_update.inc"
bool mask_control(psui::LibraryPanel& ui,Context& c){
 bool changed=ui.parameter_label("蒙版图层 / Mask Layer",c.p.mask_key);const char* selected=none;for(const auto& l:c.layers)if(l.name_key==c.p.mask_key)selected=l.name.c_str();
 ImGui::SetNextItemWidth(-1);if(ImGui::BeginCombo("##mask",ui.text(selected).c_str())){
  if(ImGui::Selectable(ui.text(none).c_str(),c.p.mask_key==0)){c.p.mask_key=0;changed=true;}
  for(std::size_t i=0;i<c.layers.size();++i){ImGui::PushID(int(i));if(ImGui::Selectable(c.layers[i].name.c_str(),c.p.mask_key==c.layers[i].name_key)){c.p.mask_key=c.layers[i].name_key;changed=true;}ui.mark(("mask.item/"+std::to_string(i)).c_str());ImGui::PopID();}ImGui::EndCombo();
 }ui.mark("mask.layer");if(changed)c.error=c.read_mask()?noErr:filterBadParameters;return changed;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p,Parameters{});auto& p=c.p;const float s=ui.scale();bool changed=false,preset_changed=false;
 ImGui::BeginChild("preview-side",{220*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),220*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-130*s,110*s,220*s));update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["jpeg.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.dirty||c.worker.pending())ImGui::TextDisabled("%s",ui.text("正在计算… / Rendering preview…").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text("读取或计算失败，请重新选择蒙版、全部重置或取消。 / Failed to read or render. Reselect the mask, reset all, or cancel.").c_str());
 ImGui::TextWrapped("%s",ui.text("相同种子和变化序号得到相同损坏。 / Seed and variation reproduce the same damage.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=Parameters{};c.mask={};c.preset=1;c.error=noErr;changed=preset_changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice("预设 / Preset",c.preset,presets,7)&&c.preset){c.worker.stop();p=jpegfx::preset(c.preset-1);c.mask={};c.error=noErr;changed=preset_changed=true;}
 auto number=[&](const char* label,double& value,double lo,double hi,double slo,double shi,bool enabled=true,bool integer=false){changed|=ui.number(label,value,lo,hi,slo,shi,enabled,integer);};
 auto integer=[&](const char* label,int& value){ui.inherit_number_default(label,value);double v=value;if(ui.number(label,v,0,1000000,0,1000,true,true)){value=int(std::lround(v));changed=true;}};
 if(ui.section("压缩 / Compression",true)){
  number("质量（%） / Quality (%)",p.quality,1,100,1,100);
  number("分辨率系数 / Resolution Factor",p.resolution,1,64,1,16,true,true);
  number("横向比例 / Horizontal Factor",p.aspect,.125,8,.25,4);
  changed|=ui.choice("色度采样 / Chroma Subsampling",p.chroma,chromas,4);
  number("色度量化（%） / Chroma Quantization (%)",p.chroma_damage,0,400,0,250);
 }
 if(ui.section("错误 / Errors",true)){
  number("错误率 / Error Rate",p.error_rate,0,64,0,8);
  ImGui::BeginDisabled(p.error_rate==0);
  changed|=ui.choice("损坏类型 / Damage Type",p.error_mode,errors,6);
  number("错误块密度（%） / Damaged Blocks (%)",p.density,0,100,0,100);
  number("错误幅度 / Error Amplitude",p.error_amp,0,10,0,3);
  number("错误聚集 / Error Clustering",p.coherence,0,32,0,8);
  number("块错位（像素） / Block Shift (px)",p.block_shift,0,256,0,64);
  number("色彩错误（%） / Chroma Error (%)",p.color_error,0,200,0,100);
  changed|=ui.check("垂直翻转错误 / Flip Errors Vertically",p.flip_noise);
  ImGui::EndDisabled();
  integer("随机种子 / Random Seed",p.seed);integer("变化序号 / Variation",p.variation);
  if(ui.button("new-damage","换一组损坏 / New Damage",-1)){p.variation=(p.variation+1)%1000001;changed=true;}
 }
 if(ui.section("频率 / Frequencies",false)){
  number("全频缩放 / All Frequencies",p.all_freq,0,8,0,3);number("X 频率缩放 / X Frequencies",p.x_freq,0,8,0,3);number("Y 频率缩放 / Y Frequencies",p.y_freq,0,8,0,3);
  number("低频缩放 / Low Frequencies",p.low_freq,0,8,0,3);number("中频缩放 / Mid Frequencies",p.mid_freq,0,8,0,3);number("高频缩放 / High Frequencies",p.high_freq,0,8,0,3);
  number("影响亮度 / Affect Luma",p.affect_luma,0,2,0,2);number("影响色度 / Affect Chroma",p.affect_chroma,0,2,0,2);
 }
 if(ui.section("颜色与混合 / Color and Mix",false)){
  number("高光缩放 / Scale Lights",p.lights,0,8,0,3);number("暗部偏移 / Offset Darks",p.darks,-1,1,-.5,.5);number("饱和度 / Saturation",p.saturation,0,4,0,2);
  changed|=ui.check("保留 HDR 超范围 / Preserve HDR Residual",p.preserve_hdr);
 }
 number("混合（%） / Mix (%)",p.mix,0,100,0,100);
 if(ui.section("蒙版 / Mask",false)){
  changed|=mask_control(ui,c);ImGui::BeginDisabled(!p.mask_key);
  changed|=ui.choice("蒙版用途 / Mask Use",p.mask_mode,masks,2);number("模糊蒙版（像素） / Mask Blur (px)",p.mask_blur,0,256,0,64);changed|=ui.check("反转蒙版 / Invert Mask",p.invert_mask);ImGui::EndDisabled();
 }
 ImGui::TextWrapped("%s",ui.text("独立 DCT 压缩与损坏模型；不依赖 Sapphire，不保证原版逐像素一致。 / Independent DCT compression and damage model; no Sapphire dependency or pixel-parity claim.").c_str());
 ImGui::EndChild();
 if(changed){c.worker.stop();c.dirty=true;c.due=GetTickCount64()+35;if(!preset_changed)c.preset=0;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,const std::vector<psui::HostLayer>& layers,Parameters& p,Image& mask){
 Context c(p,host,src,layers);if(!c.read_mask())c.error=filterBadParameters;
 const auto answer=psui::run_library_panel(m,owner,host,title,900,760,[&](psui::LibraryPanel& ui){draw(ui,c);},
 "压缩 质量 分辨率系数 横向比例 色度采样 色度量化 错误 错误率 损坏类型 错误块密度 错误幅度 错误聚集 块错位 像素 色彩错误 垂直翻转错误 随机种子 变化序号 换一组损坏 频率 全频缩放 频率缩放 低频缩放 中频缩放 高频缩放 影响亮度 影响色度 颜色与混合 高光缩放 暗部偏移 饱和度 保留超范围 混合 蒙版 蒙版图层 蒙版用途 模糊蒙版 反转蒙版 无 自定义 经典低质 重度压缩 传输损坏 块错位 色彩损坏 频率破碎 全色度 水平减半 双向减半 黑白 混合 频率噪声 亮度跳变 系数丢失 色彩跳变 亮度 正在计算 读取或计算失败，请重新选择蒙版、全部重置或取消。相同种子和变化序号得到相同损坏。独立压缩与损坏模型；不依赖，不保证原版逐像素一致。",&plugin_info);
 c.worker.stop();if(answer==IDOK&&!c.error){p=c.p;mask=std::move(c.mask);}return answer;
}
}
