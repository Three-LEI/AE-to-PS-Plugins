#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "displacement_host.hpp"
#include "displacement_info.hpp"
#include "psui/library_panel.hpp"
#include "psui/preview_background.hpp"
#include <algorithm>
#include <cmath>
namespace displacement {
namespace {
constexpr const char* channels[]={"红色 / Red","绿色 / Green","蓝色 / Blue","Alpha / Alpha",
 "亮度 / Luminance","色相 / Hue","明度 / Lightness","饱和度 / Saturation","关闭 / Off"};
constexpr const char* placements[]={"中心图 / Center Map","拉伸适合 / Stretch To Fit","平铺图 / Tile Map"};
struct Context {
 Parameters p,defaults;int default_selected=0;const Image& source;Image map;
 FilterRecord* host;const std::vector<psui::HostLayer>& layers;
 psui::PreviewView view;psui::ProxyImage proxy;
 psui::PreviewBackground background;
 int selected=0;bool dirty=true,preview=true;OSErr error=noErr;
 std::vector<float> output;std::vector<std::uint8_t> display;
 Context(Parameters values,const Image& src,FilterRecord* f,const std::vector<psui::HostLayer>& ls):p(values),source(src),host(f),layers(ls){
  for(std::size_t i=0;i<ls.size();++i)if(ls[i].id==p.layer_id)selected=int(i);
  for(std::size_t i=0;i<ls.size();++i)if(!ls[i].target) {default_selected=int(i);break;}
  defaults=Parameters{};defaults.layer_id=ls[default_selected].id;defaults.layer_name_key=ls[default_selected].name_key;
 }
 void read_map(){
  psui::LayerPixels pixels;error=psui::host_read_layer(host,layers[selected],layer_bounds(layers[selected]),0,pixels);
  if(!error){map=from_host(std::move(pixels));p.layer_id=layers[selected].id;p.layer_name_key=layers[selected].name_key;dirty=true;}
 }
};
void update(psui::LibraryPanel& ui,Context& c){
 if(!c.dirty&&!c.view.dirty)return;
 if(c.error||!c.map.valid())return;
 const auto roi=c.view.region();
 const int w=std::max(1,roi.right-roi.left),h=std::max(1,roi.bottom-roi.top);
 const int step=std::max(1,int(std::ceil(double(std::max(w,h))/c.view.proxy_side())));
 const int pw=(w+step-1)/step,ph=(h+step-1)/step;
 c.proxy.width=pw;c.proxy.height=ph;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 c.background.prepare(c.host,c.proxy);
 c.output.resize(std::size_t(pw)*ph*4);c.display.resize(std::size_t(pw)*ph*3);
 auto parameters=c.p;if(!c.preview)parameters.amount_x=parameters.amount_y=0;
 if(!render(c.source,c.map,parameters,{double(roi.left),double(roi.top),double(step),pw,ph},c.output.data()))return;
 for(int y=0;y<ph;++y)for(int x=0;x<pw;++x){
  const auto i=std::size_t(y)*pw+x;const double a=c.host->outTransparencyMask?std::clamp(double(c.output[i*4+3]),0.,1.):1.;
  const int bg=((x/8+y/8)&1)?156:200;
  if(c.background.enabled){
   const int sx=std::clamp(int(std::lround((roi.left+x*step-c.source.left)/c.source.step)),0,c.source.width-1);
   const int sy=std::clamp(int(std::lround((roi.top+y*step-c.source.top)/c.source.step)),0,c.source.height-1);
   const auto si=(std::size_t(sy)*c.source.width+sx)*4;
   std::array<float,4> original{},processed{};for(int k=0;k<4;++k){original[k]=c.source.rgba[si+k];processed[k]=c.output[i*4+k];}
   if(!c.host->outTransparencyMask)original[3]=processed[3]=1;
   const auto pixel=c.background.pixel(x,y,original,processed);
   for(int k=0;k<3;++k)c.display[i*3+k]=pixel[k];
   continue;
  }
  for(int k=0;k<3;++k)c.display[i*3+k]=std::uint8_t(std::lround(std::clamp(double(c.output[i*4+k]),0.,1.)*255*a+bg*(1-a)));
 }
 if(!ui.preview(c.host,c.display.data(),pw,ph)){c.error=memFullErr;return;}
 c.dirty=c.view.dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, c.defaults);
 const float s=ui.scale();bool changed=false;
 ImGui::BeginChild("preview-side",{190*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),190*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-112*s,100*s,190*s));
 update(ui,c);ui.image_view(c.view,c.proxy);changed|=ui.check("预览 / Preview",c.preview);
 if(psui::preview_background_control(ui,c.background))c.dirty=true;
 ui.preview_zoom(c.view);
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr||!c.map.valid());if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.p=c.defaults;c.selected=c.default_selected;c.read_map();changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.parameter_label("置换图层 / Displacement Map Layer",c.p.layer_id)){c.selected=c.default_selected;c.read_map();changed=true;}
 ImGui::SetNextItemWidth(-1);
 if(ImGui::BeginCombo("##source-layer",c.layers[c.selected].name.c_str())){
  for(std::size_t i=0;i<c.layers.size();++i){ImGui::PushID(int(i));
   const auto label=std::to_string(i+1)+". "+c.layers[i].name;
   if(ImGui::Selectable(label.c_str(),c.selected==int(i))){c.selected=int(i);c.read_map();}
   ui.mark(("map.layer/"+std::to_string(i)).c_str());ImGui::PopID();}
  ImGui::EndCombo();
 }ui.mark("map.layer");
 ImGui::TextDisabled("%s",ui.text("源像素（不含图层蒙版） / Source pixels (without layer masks)").c_str());
 ui.inherit_choice_default("用于水平置换 / Horizontal Channel",c.p.horizontal);ui.inherit_choice_default("用于垂直置换 / Vertical Channel",c.p.vertical);ui.inherit_choice_default("置换图特性 / Map Behavior",c.p.placement);
 int horizontal=int(c.p.horizontal),vertical=int(c.p.vertical),placement=int(c.p.placement);
 if(ui.choice("用于水平置换 / Horizontal Channel",horizontal,channels,9)){c.p.horizontal=Channel(horizontal);changed=true;}
 changed|=ui.number("最大水平置换 / Horizontal Amount",c.p.amount_x,-10000,10000,-100,100);
 if(ui.choice("用于垂直置换 / Vertical Channel",vertical,channels,9)){c.p.vertical=Channel(vertical);changed=true;}
 changed|=ui.number("最大垂直置换 / Vertical Amount",c.p.amount_y,-10000,10000,-100,100);
 if(ui.choice("置换图特性 / Map Behavior",placement,placements,3)){c.p.placement=Placement(placement);changed=true;}
 changed|=ui.check("像素回绕 / Wrap Pixels Around",c.p.wrap);
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text("输出限于 PS 当前滤镜作用范围。 / Output is limited to the current Photoshop filter bounds.").c_str());
 if(c.error)ImGui::TextWrapped("%s (%d)",ui.text("无法读取此采样图层，请选择其他图层。 / Unable to read this map layer. Choose another layer.").c_str(),int(c.error));
 ImGui::EndChild();if(changed)c.dirty=true;
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,const std::vector<psui::HostLayer>& layers,const Image& source,Parameters& p,Image& map){
 Context c{p,source,host,layers};c.read_map();
 std::string glyphs="置换图层 用于水平置换 最大水平置换 用于垂直置换 最大垂直置换 置换图特性 像素回绕 源像素（不含图层蒙版） 输出限于 PS 当前滤镜作用范围。 无法读取此采样图层，请选择其他图层。";
 for(auto text:channels)glyphs+=text;for(auto text:placements)glyphs+=text;for(const auto& l:layers)glyphs+=l.name;
 const auto answer=psui::run_library_panel(module,owner,host,L"Displacement Map 置换图",760,440,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);
 if(answer==IDOK&&c.error==noErr){p=c.p;map=std::move(c.map);}
 return answer;
}
}
