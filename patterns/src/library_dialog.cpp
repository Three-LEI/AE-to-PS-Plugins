#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "pattern_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/native_preview.hpp"
#include "psui/blend_labels.hpp"
#include "psui/async_preview.hpp"
namespace patterns {
namespace {
constexpr const char* sizes[]={"边角点 / Corner Point","宽度滑块 / Width Slider","宽度与高度滑块 / Width and Height Sliders"};
constexpr const char* grid_presets[]={"自定义 / Custom","默认 / Default","细网格 / Fine Grid","三分线 / Rule of Thirds","柔化网格 / Soft Grid"};
constexpr const char* checker_presets[]={"自定义 / Custom","默认 / Default","大棋格 / Large Checks","矩形棋格 / Rectangular Checks","柔化棋格 / Soft Checks"};
constexpr const char* labels[]={"预设 / Preset","图案 / Pattern","锚点 / Anchor","大小依据 / Size From","边角 / Corner","宽度 / Width","高度 / Height","边界 / Border","羽化 / Feather","羽化宽度 / Feather Width","羽化高度 / Feather Height","反转网格 / Invert Grid","交换棋格 / Swap Cells","颜色 / Color","不透明度（%） / Opacity (%)","混合模式 / Blending Mode","变换 / Transform","旋转 / Rotation","锚点居中 / Center Anchor","右键缩览图设置锚点。 / Right-click the preview to set the anchor.","无模式输出透明图案；其余模式与当前层混合。 / None outputs a transparent pattern; other modes composite with the current layer.","正在更新… / Updating…","无法读取原图；选择“无”可直接生成图案。 / Source unavailable; select None to generate the pattern.","计算失败，请重置或取消。 / Rendering failed. Reset or cancel."};
enum Label{preset_label,main_label,anchor_label,size_label,corner_label,width_label,height_label,border_label,feather_label,feather_x_label,feather_y_label,invert_grid_label,invert_check_label,color_label,opacity_label,blend_label,transform_label,rotation_label,center_label,pick_label,none_help_label,working_label,source_error_label,error_label};
struct Context {
 Parameters p;FilterRecord* host;psui::PreviewView view;psui::ProxyImage geometry;psui::NativePreview input;psui::PreviewBackground background;
 Image last;std::vector<std::uint8_t> display;bool preview=true,dirty=true,display_dirty=true;int preset=0;OSErr error=noErr;ULONGLONG update_at=0;
 psui::AsyncPreview<Image> worker;
 Context(Parameters values,FilterRecord* f):p(values),host(f){}
};
bool number(psui::LibraryPanel& ui,int label,double& value,double lo,double hi,double slide_lo,double slide_hi,bool enabled=true){return ui.number(labels[label],value,lo,hi,slide_lo,slide_hi,enabled);}
void start(Context& c){
 const auto p=c.p;const auto geometry=c.geometry;const bool locked=locked_alpha(c.host);c.worker.start([&c,p,geometry,locked](std::stop_token token){Image result;render(pattern_kind,p,{geometry.origin_x,geometry.origin_y,geometry.step},geometry.width,geometry.height,result,&c.input.rgba,[token]{return token.stop_requested();},locked);return result;});c.dirty=false;
}
void update(psui::LibraryPanel& ui,Context& c){
 try{Image completed;if(c.worker.take(completed)){if(completed.valid()){c.last=std::move(completed);c.display_dirty=true;}else c.error=filterBadParameters;}}catch(...){c.error=memFullErr;}
 if(c.view.dirty){
  c.worker.stop();const auto roi=c.view.region();const int width=std::max(1,roi.right-roi.left),height=std::max(1,roi.bottom-roi.top);
  const int step=std::max(1,int(std::ceil(double(std::max(width,height))/c.view.proxy_side())));
  c.geometry.width=(width+step-1)/step;c.geometry.height=(height+step-1)/step;c.geometry.origin_x=roi.left;c.geometry.origin_y=roi.top;c.geometry.step=step;
  c.input.read(c.host,c.geometry);c.background.cached=false;
  if(!render(pattern_kind,c.p,{double(roi.left),double(roi.top),double(step)},c.geometry.width,c.geometry.height,c.last,&c.input.rgba,{},locked_alpha(c.host)))c.error=filterBadParameters;
  c.view.dirty=false;c.dirty=false;c.display_dirty=true;
 }
 if(c.dirty&&GetTickCount64()>=c.update_at&&!c.error)start(c);
 if(!c.display_dirty)return;
 c.background.prepare(c.host,c.geometry);const auto count=std::size_t(c.geometry.width)*c.geometry.height;c.display.resize(count*3);
 for(std::size_t i=0;i<count;++i){Pixel before{},after{};for(int k=0;k<4;++k)before[k]=c.input.rgba[i*4+k];after=before;
  if(c.preview&&c.last.valid())for(int k=0;k<4;++k)after[k]=c.last.rgba[i*4+k];
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  if(!c.host->outTransparencyMask){for(int k=0;k<3;++k){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto color=c.background.pixel(int(i%c.geometry.width),int(i/c.geometry.width),before,after,c.input.selection.empty()?1:c.input.selection[i]);
  for(int k=0;k<3;++k)c.display[i*3+k]=color[k];
 }
 if(!ui.preview(c.host,c.display.data(),c.geometry.width,c.geometry.height))c.error=memFullErr;c.display_dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, defaults_for_host(c.host));
 ui.inherit_choice_default(labels[size_label],c.p.size_from);
 ui.inherit_choice_default(labels[blend_label],c.p.blend);
 const float s=ui.scale();bool changed=false,preset_changed=false;auto& p=c.p;const auto doc=document_bounds(c.host);
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 auto bounds=psui::filter_bounds(c.host);if(bounds.right<=bounds.left||bounds.bottom<=bounds.top)bounds=doc;
 c.view.configure(bounds,205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-150*s,110*s,205*s));update(ui,c);ui.image_view(c.view,c.geometry);
 ui.items["pattern.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");
 if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
  const auto mouse=ImGui::GetMousePos();const double x=c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01),y=c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01);
  if(x>=doc.left&&x<doc.right&&y>=doc.top&&y<doc.bottom){p.anchor_x=x;p.anchor_y=y;changed=true;}
 }
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.dirty||c.worker.pending())ImGui::TextDisabled("%s",ui.text(labels[working_label]).c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[error_label]).c_str());
 const bool source_error=c.input.source_error&&p.blend!=Blend::none;
 if(source_error)ImGui::TextWrapped("%s",ui.text(labels[source_error_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error||source_error);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=defaults_for_host(c.host);c.preset=1;c.error=noErr;changed=preset_changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(ui.choice(labels[preset_label],c.preset,pattern_kind==Kind::grid?grid_presets:checker_presets,5)&&c.preset){p=patterns::preset(pattern_kind,c.preset-1,doc.right-doc.left,doc.bottom-doc.top);changed=preset_changed=true;}
 if(ui.section(labels[main_label],true)){
  changed|=psui::point_control(ui,labels[anchor_label],p.anchor_x,p.anchor_y);
  int mode=int(p.size_from);if(ui.choice(labels[size_label],mode,sizes,3)){p.size_from=SizeFrom(mode);changed=true;}
  changed|=psui::point_control(ui,labels[corner_label],p.corner_x,p.corner_y,p.size_from==SizeFrom::corner);
  changed|=number(ui,width_label,p.width,.01,1.e7,2,200,p.size_from!=SizeFrom::corner);
  changed|=number(ui,height_label,p.height,.01,1.e7,2,200,p.size_from==SizeFrom::width_height);
  if(pattern_kind==Kind::grid)changed|=number(ui,border_label,p.border,0,1.e7,0,50);
  changed|=ui.check(labels[pattern_kind==Kind::grid?invert_grid_label:invert_check_label],p.invert);
 }
 if(ui.section(labels[feather_label],false)){changed|=number(ui,feather_x_label,p.feather_x,0,1.e7,0,100);changed|=number(ui,feather_y_label,p.feather_y,0,1.e7,0,100);}
 changed|=psui::color_control(ui,c.host,labels[color_label],p.color);changed|=number(ui,opacity_label,p.opacity,0,100,0,100);
 int blend=int(p.blend);if(ui.choice(labels[blend_label],blend,psui::blend_labels,int(Blend::count))){p.blend=Blend(blend);changed=true;}
 if(ui.section(labels[transform_label],false)){changed|=number(ui,rotation_label,p.rotation,-360000,360000,-180,180);if(ui.button("center",labels[center_label],-1)){p.anchor_x=(doc.left+doc.right)*.5;p.anchor_y=(doc.top+doc.bottom)*.5;changed=true;}}
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[pick_label]).c_str());ImGui::TextWrapped("%s",ui.text(labels[none_help_label]).c_str());ImGui::EndChild();
 if(changed){c.worker.stop();if(!preset_changed)c.preset=0;c.dirty=true;c.update_at=GetTickCount64()+40;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,Parameters& p){
 Context c(p,host);if(p==defaults_for_host(host))c.preset=1;std::string glyphs;for(auto s:labels)glyphs+=s;
 for(auto s:sizes)glyphs+=s;for(auto s:grid_presets)glyphs+=s;for(auto s:checker_presets)glyphs+=s;for(auto s:psui::blend_labels)glyphs+=s;
 const auto result=psui::run_library_panel(module,owner,host,pattern_title,740,520,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&pattern_info);
 c.worker.stop();if(result==IDOK&&!c.error)p=c.p;return result;
}
}
