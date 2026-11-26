#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "pattern_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/native_preview.hpp"
#include "psui/blend_labels.hpp"
#include "psui/adaptive_preview.hpp"
namespace patterns {
namespace {
constexpr const char* sizes[]={"边角点 / Corner Point","宽度滑块 / Width Slider","宽度与高度滑块 / Width and Height Sliders"};
constexpr const char* grid_presets[]={"自定义 / Custom","默认 / Default","细网格 / Fine Grid","三分线 / Rule of Thirds","柔化网格 / Soft Grid"};
constexpr const char* checker_presets[]={"自定义 / Custom","默认 / Default","大棋格 / Large Checks","矩形棋格 / Rectangular Checks","柔化棋格 / Soft Checks"};
constexpr const char* labels[]={"预设 / Preset","图案 / Pattern","锚点 / Anchor","大小依据 / Size From","边角 / Corner","宽度 / Width","高度 / Height","边界 / Border","羽化 / Feather","羽化宽度 / Feather Width","羽化高度 / Feather Height","反转网格 / Invert Grid","交换棋格 / Swap Cells","颜色 / Color","不透明度（%） / Opacity (%)","混合模式 / Blending Mode","变换 / Transform","旋转 / Rotation","锚点居中 / Center Anchor","拖动预览中的锚点或边角。 / Drag the anchor or corner in the preview.","无模式输出透明图案；其余模式与当前层混合。 / None outputs a transparent pattern; other modes composite with the current layer.","正在更新… / Updating…","无法读取原图；选择“无”可直接生成图案。 / Source unavailable; select None to generate the pattern.","计算失败，请重置或取消。 / Rendering failed. Reset or cancel."};
enum Label{preset_label,main_label,anchor_label,size_label,corner_label,width_label,height_label,border_label,feather_label,feather_x_label,feather_y_label,invert_grid_label,invert_check_label,color_label,opacity_label,blend_label,transform_label,rotation_label,center_label,pick_label,none_help_label,working_label,source_error_label,error_label};
struct PreviewFrame{Image image;psui::ProxyImage geometry;std::shared_ptr<psui::NativePreview> input;};
struct Context {
 Parameters p;FilterRecord* host;psui::PreviewView view;psui::ProxyImage geometry;psui::NativePreview input;psui::PreviewBackground background;
 Image last;std::vector<std::uint8_t> display;bool preview=true,dirty=true,display_dirty=true;int preset=0;OSErr error=noErr;ULONGLONG update_at=0;
 psui::AdaptivePreview<PreviewFrame> worker;std::shared_ptr<psui::NativePreview> full_input,draft_input;psui::ProxyImage full_geometry,draft_geometry;int draft_side=0;
 Context(Parameters values,FilterRecord* f):p(values),host(f){}
};
bool number(psui::LibraryPanel& ui,int label,double& value,double lo,double hi,double slide_lo,double slide_hi,bool enabled=true){return ui.number(labels[label],value,lo,hi,slide_lo,slide_hi,enabled);}

void update(psui::LibraryPanel& ui,Context& c){
 if(c.view.dirty||c.draft_side!=c.worker.draft_side()){c.draft_side=c.worker.draft_side();
  c.full_geometry=psui::adaptive_geometry(c.view,false);c.draft_geometry=psui::adaptive_geometry(c.view,psui::PreviewPass{true,c.draft_side});
  c.full_input=std::make_shared<psui::NativePreview>();c.full_input->read(c.host,c.full_geometry);
  c.draft_input=std::make_shared<psui::NativePreview>();c.draft_input->read(c.host,c.draft_geometry);
  c.view.dirty=false;c.dirty=true;
 }

 const auto p=c.p;const bool locked=locked_alpha(c.host);const auto full_geo=c.full_geometry,draft_geo=c.draft_geometry;const auto full=c.full_input,small=c.draft_input;
 try{PreviewFrame completed;
  if(c.worker.update(c.dirty,completed,[p,locked,full_geo,draft_geo,full,small](bool draft,const std::function<bool()>& cancel){
   const auto geometry=draft?draft_geo:full_geo;const auto input=draft?small:full;
   PreviewFrame frame;frame.geometry=geometry;frame.input=input;render(pattern_kind,p,{geometry.origin_x,geometry.origin_y,geometry.step},geometry.width,geometry.height,frame.image,&input->rgba,cancel,locked); return frame;
  })){if(completed.image.valid()){c.last=std::move(completed.image);c.geometry=completed.geometry;c.input=*completed.input;c.display_dirty=true;}else c.error=filterBadParameters;}
 }catch(...){c.error=memFullErr;}
 if(!c.last.valid()||!c.display_dirty)return;
 c.background.prepare(c.host,c.geometry);const auto count=std::size_t(c.geometry.width)*c.geometry.height;c.display.resize(count*3);
 for(std::size_t i=0;i<count;i++){
  std::array<float,4> before{},after{};for(int k=0;k<4;k++){before[k]=c.input.rgba[i*4+k];after[k]=c.preview?c.last.rgba[i*4+k]:before[k];}
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  if(!c.host->outTransparencyMask){for(int k=0;k<3;k++){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto color=c.background.pixel(int(i%c.geometry.width),int(i/c.geometry.width),before,after,c.input.selection.empty()?1:c.input.selection[i]);for(int k=0;k<3;k++)c.display[i*3+k]=color[k];
 }
 if(!ui.preview(c.host,c.display.data(),c.geometry.width,c.geometry.height))c.error=memFullErr;c.display_dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, defaults_for_host(c.host));
 ui.inherit_choice_default(labels[size_label],c.p.size_from);
 ui.inherit_choice_default(labels[blend_label],c.p.blend);
 bool changed=false,preset_changed=false;auto& p=c.p;const auto doc=document_bounds(c.host);
 const float footer = ui.filter_footer_height();
 auto bounds=psui::filter_document_bounds(c.host);if(bounds.right<=bounds.left||bounds.bottom<=bounds.top)bounds=doc;
 ui.begin_filter_preview(c.view, bounds, footer);update(ui,c);
 psui::PreviewAdjustments handles;
 handles.point(labels[anchor_label], p.anchor_x, p.anchor_y);
 if (p.size_from == SizeFrom::corner) handles.point(labels[corner_label], p.corner_x, p.corner_y)
     .line(p.anchor_x, p.anchor_y, p.corner_x, p.corner_y);
 handles.angle(labels[rotation_label], p.rotation, p.anchor_x, p.anchor_y, -360000, 360000);
 changed |= ui.image_view(c.view,c.geometry,handles);
 ui.items["pattern.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);

 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[error_label]).c_str());
 const bool source_error=c.input.source_error&&p.blend!=Blend::none;
 if(source_error)ImGui::TextWrapped("%s",ui.text(labels[source_error_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error||source_error);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=defaults_for_host(c.host);c.preset=1;c.error=noErr;changed=preset_changed=true;}
 ImGui::EndChild();ImGui::SameLine();ui.begin_filter_parameters();
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
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[pick_label]).c_str());ImGui::TextWrapped("%s",ui.text(labels[none_help_label]).c_str());ui.end_filter_parameters();
 if(changed){if(!preset_changed)c.preset=0;c.dirty=true;c.update_at=GetTickCount64()+40;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,Parameters& p){
 Context c(p,host);if(p==defaults_for_host(host))c.preset=1;std::string glyphs;for(auto s:labels)glyphs+=s;
 for(auto s:sizes)glyphs+=s;for(auto s:grid_presets)glyphs+=s;for(auto s:checker_presets)glyphs+=s;for(auto s:psui::blend_labels)glyphs+=s;
 const auto result=psui::run_filter_panel(module,owner,host,pattern_title,740,520,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&pattern_info);
 c.worker.stop();if(result==IDOK&&!c.error)p=c.p;return result;
}
}
