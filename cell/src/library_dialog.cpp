#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "cell_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/native_preview.hpp"
#include "psui/async_preview.hpp"
#include <memory>
namespace cell {
namespace {
constexpr const char* modes[]={"气泡 / Bubbles","晶体 / Crystals","印板 / Plates","静态板 / Static Plates","晶格化 / Crystallize","枕状 / Pillow","晶体 HQ / Crystals HQ","印板 HQ / Plates HQ","静态板 HQ / Static Plates HQ","晶格化 HQ / Crystallize HQ","混合晶体 / Mixed Crystals","管状 / Tubular"};
constexpr const char* overflows[]={"剪切 / Clip","柔和限制 / Soft Clamp","反绕 / Wrap Back"};
constexpr const char* labels[]={"单元格图案 / Cell Pattern","反转 / Invert","对比度 / Contrast","锐度 / Sharpness","溢出 / Overflow","分散 / Disperse","大小 / Size","偏移 / Offset","平铺选项 / Tiling Options","启用平铺 / Enable Tiling","水平单元格 / Cells Horizontal","垂直单元格 / Cells Vertical","演化 / Evolution","演化（度） / Evolution (degrees)","演化选项 / Evolution Options","循环演化 / Cycle Evolution","循环（转数） / Cycle (revolutions)","随机种子 / Random Seed","偏移居中 / Center Offset","右键缩览图设置偏移。 / Right-click the preview to set the offset.","晶格化读取当前图层颜色；纯色输入仍为纯色。 / Crystallize samples the current layer; a uniform input stays uniform.","独立重建，随机布局及细节可能与 AE 不同。 / Independent reconstruction; layout and details may differ from AE.","正在更新… / Updating…","无法读取原图，晶格化需要可读取的 RGB 图层。 / Crystallize needs a readable RGB source layer.","计算失败，请重置或取消。 / Rendering failed. Reset or cancel.","预览取样为近似显示，最终输出使用原始分辨率。 / Preview sampling is approximate; final output uses native resolution.","平铺尺寸 / Tile Size"};
enum Label{mode_label,invert_label,contrast_label,sharpness_label,overflow_label,disperse_label,size_label,offset_label,tiling_label,tile_label,cells_x_label,cells_y_label,evolution_label,degrees_label,evo_options_label,cycle_evo_label,cycle_label,seed_label,center_label,pick_label,source_help_label,disclaimer_label,working_label,source_error_label,error_label,approx_label,tile_size_label};
struct Context {
 Parameters p;FilterRecord* host;psui::PreviewView view;psui::ProxyImage geometry;psui::NativePreview input;psui::PreviewBackground background;
 std::shared_ptr<Image> source;OSErr source_error=noErr,error=noErr;
 Image last;std::vector<std::uint8_t> display;bool preview=true,dirty=true,display_dirty=true;ULONGLONG update_at=0;psui::AsyncPreview<Image> worker;
 Context(Parameters values,FilterRecord* f):p(values),host(f){}
};
bool number(psui::LibraryPanel& ui,Label label,double& value,double lo,double hi,double slide_lo,double slide_hi,bool enabled=true){return ui.number(labels[label],value,lo,hi,slide_lo,slide_hi,enabled);}
bool integer(psui::LibraryPanel& ui,Label label,int& value,int lo,int hi,int slide_hi,bool enabled=true){ui.inherit_number_default(labels[label],value);double d=value;const bool changed=ui.number(labels[label],d,lo,hi,lo,slide_hi,enabled,true);if(changed)value=int(std::lround(d));return changed;}
void load_source(Context& c){
 if(!samples_source(c.p.mode)||c.source||c.source_error)return;
 auto image=std::make_shared<Image>();c.source_error=read_source(c.host,1024,*image);if(!c.source_error)c.source=std::move(image);
}
void start(Context& c){
 const auto p=c.p;const auto geometry=c.geometry;const auto source=c.source;const auto doc=document_bounds(c.host);
 c.worker.start([p,geometry,source,doc](std::stop_token token){Image result;Source s;if(source)s=source_view(*source,doc);
  render(p,{geometry.origin_x,geometry.origin_y,geometry.step},geometry.width,geometry.height,result,s,[token]{return token.stop_requested();});return result;});c.dirty=false;
}
void update(psui::LibraryPanel& ui,Context& c){
 try{Image completed;if(c.worker.take(completed)){if(completed.valid()){c.last=std::move(completed);c.display_dirty=true;}else c.error=filterBadParameters;}}catch(...){c.error=memFullErr;}
 if(c.view.dirty){
  c.worker.stop();const auto roi=c.view.region();const int width=std::max(1,roi.right-roi.left),height=std::max(1,roi.bottom-roi.top);
  const int step=std::max(1,int(std::ceil(double(std::max(width,height))/c.view.proxy_side())));
  c.geometry.width=(width+step-1)/step;c.geometry.height=(height+step-1)/step;c.geometry.origin_x=roi.left;c.geometry.origin_y=roi.top;c.geometry.step=step;
  c.input.read(c.host,c.geometry);c.background.cached=false;c.last={};c.view.dirty=false;c.dirty=true;c.display_dirty=true;c.update_at=0;
 }
 load_source(c);
 if(c.dirty&&GetTickCount64()>=c.update_at&&!c.error){
  if(!samples_source(c.p.mode)||c.source)start(c);else {c.dirty=false;c.last={};c.display_dirty=true;}
 }
 if(!c.display_dirty)return;
 c.background.prepare(c.host,c.geometry);const auto count=std::size_t(c.geometry.width)*c.geometry.height;c.display.resize(count*3);
 for(std::size_t i=0;i<count;++i){std::array<float,4> before{},after{};for(int k=0;k<4;++k)before[k]=c.input.rgba[i*4+k];after=before;
  if(c.preview&&c.last.valid())for(int k=0;k<4;++k)after[k]=c.last.rgba[i*4+k];
  if(locked_alpha(c.host))after[3]=before[3];
  if(!c.host->outTransparencyMask){for(int k=0;k<3;++k){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto color=c.background.pixel(int(i%c.geometry.width),int(i/c.geometry.width),before,after,c.input.selection.empty()?1:c.input.selection[i]);
  for(int k=0;k<3;++k)c.display[i*3+k]=color[k];
 }
 if(!ui.preview(c.host,c.display.data(),c.geometry.width,c.geometry.height))c.error=memFullErr;c.display_dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, defaults_for_host(c.host));
 ui.inherit_choice_default(labels[mode_label],c.p.mode);
 ui.inherit_choice_default(labels[overflow_label],c.p.overflow);
 const float s=ui.scale();auto& p=c.p;bool changed=false;const auto doc=document_bounds(c.host);
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 auto bounds=psui::filter_bounds(c.host);if(bounds.right<=bounds.left||bounds.bottom<=bounds.top)bounds=doc;
 c.view.configure(bounds,205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-160*s,110*s,205*s));update(ui,c);ui.image_view(c.view,c.geometry);
 ui.items["cell.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");
 if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
  const auto mouse=ImGui::GetMousePos();p.offset_x=std::clamp(c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01),double(doc.left),double(doc.right));p.offset_y=std::clamp(c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01),double(doc.top),double(doc.bottom));changed=true;
 }
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.dirty||c.worker.pending())ImGui::TextDisabled("%s",ui.text(labels[working_label]).c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text(labels[error_label]).c_str());
 const bool source_error=samples_source(p.mode)&&c.source_error;
 if(source_error)ImGui::TextWrapped("%s",ui.text(labels[source_error_label]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error||source_error);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();
 if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=defaults_for_host(c.host);c.error=noErr;c.source_error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 int mode=int(p.mode);if(ui.choice(labels[mode_label],mode,modes,12)){p.mode=Mode(mode);changed=true;}
 changed|=ui.check(labels[invert_label],p.invert);
 changed|=number(ui,sharpness_mode(p.mode)?sharpness_label:contrast_label,p.contrast,0,10000,0,600);
 int over=int(p.overflow);ImGui::BeginDisabled(sharpness_mode(p.mode));if(ui.choice(labels[overflow_label],over,overflows,3)){p.overflow=Overflow(over);changed=true;}ImGui::EndDisabled();
 changed|=number(ui,disperse_label,p.disperse,0,1.5,0,1);
 changed|=number(ui,size_label,p.size,2,2000,2,600);
 changed|=psui::point_control(ui,labels[offset_label],p.offset_x,p.offset_y);
 if(ui.section(labels[tiling_label],false)){
  changed|=ui.check(labels[tile_label],p.tiling);changed|=integer(ui,cells_x_label,p.cells_x,1,1000,20,p.tiling);changed|=integer(ui,cells_y_label,p.cells_y,1,1000,20,p.tiling);
  if(p.tiling)ImGui::Text("%s: %.8g x %.8g px",ui.text(labels[tile_size_label]).c_str(),p.cells_x*p.size,p.cells_y*p.size);
 }
 if(ui.section(labels[evolution_label],true)){
  changed|=number(ui,degrees_label,p.evolution,-36000000,36000000,0,720);
  if(ui.section(labels[evo_options_label],false)){
   changed|=ui.check(labels[cycle_evo_label],p.cycle_evolution);changed|=integer(ui,cycle_label,p.cycle,1,88,10,p.cycle_evolution);
  }
  changed|=integer(ui,seed_label,p.seed,0,100000,100);
 }
 if(ui.button("center",labels[center_label],-1)){p.offset_x=(doc.left+doc.right)*.5;p.offset_y=(doc.top+doc.bottom)*.5;changed=true;}
 ImGui::Spacing();ImGui::TextWrapped("%s",ui.text(labels[pick_label]).c_str());
 if(samples_source(p.mode)){ImGui::TextWrapped("%s",ui.text(labels[source_help_label]).c_str());if(c.source&&(c.source->width<doc.right-doc.left||c.source->height<doc.bottom-doc.top))ImGui::TextDisabled("%s",ui.text(labels[approx_label]).c_str());}
 ImGui::TextWrapped("%s",ui.text(labels[disclaimer_label]).c_str());ImGui::EndChild();
 if(changed){c.worker.stop();c.dirty=true;c.update_at=GetTickCount64()+35;}
}
}
INT_PTR show_dialog(HINSTANCE module,HWND owner,FilterRecord* host,Parameters& p){
 Context c(p,host);std::string glyphs;for(auto s:labels)glyphs+=s;for(auto s:modes)glyphs+=s;for(auto s:overflows)glyphs+=s;
 const auto result=psui::run_library_panel(module,owner,host,cell_title,740,540,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&cell_info);
 c.worker.stop();if(result==IDOK&&!c.error)p=c.p;return result;
}
}
