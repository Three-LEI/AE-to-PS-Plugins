#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "lens_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/async_preview.hpp"
#include "psui/preview_background.hpp"
#include <cmath>
namespace lens {namespace {
constexpr const char* axes[]={"水平 / Horizontal","垂直 / Vertical","对角线 / Diagonal"};
constexpr const char* labels[]={"视场 (FOV) / Field Of View","反转镜头扭曲 / Reverse Lens Distortion","FOV 方向 / FOV Orientation","视图中心 / View Center","最佳像素（非可逆） / Optimal Pixels (Non-reversible)","中心 / Center","大小 / Size","收敛度 / Convergence","中心居中 / Center","调整大小：关闭（PS画布固定）。保留边缘需先扩展画布。 / Resize: Off (fixed Photoshop canvas). Enlarge the canvas first to retain edges.","最佳像素使用同源参考拟合，细节可能与AE不同。 / Optimal Pixels is fitted to native references; details can differ from AE.","右键缩览图设置中心。 / Right-click the preview to set the center.","正在计算… / Rendering preview…","计算失败，请重置或取消。 / Rendering failed. Reset or cancel.","独立复刻，不调用AE或原版插件。 / Independent reconstruction; no AE or original plug-in dependency."};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;ULONGLONG due=0;
 psui::AsyncPreview<Image> worker;
 Context(Parameters values,FilterRecord* f,const Image& src):p(values),host(f),source(src){}
};
void update(psui::LibraryPanel& ui,Context& c){
 try{Image result;if(c.worker.take(result)){if(result.valid()){c.last=std::move(result);c.display_dirty=true;}else c.error=filterBadParameters;}}catch(...){c.error=memFullErr;}
 if(c.dirty&&GetTickCount64()>=c.due&&!c.error){const auto p=c.p;c.worker.start([&c,p](std::stop_token token){Image result;render(c.source,p,result,[token]{return token.stop_requested();});return result;});c.dirty=false;}
 if(!c.display_dirty&&!c.view.dirty)return;
 const auto roi=c.view.region();const int step=std::max(1,int(std::ceil(double(std::max(roi.right-roi.left,roi.bottom-roi.top))/c.view.proxy_side())));
 const int w=std::max(1,(roi.right-roi.left+step-1)/step),h=std::max(1,(roi.bottom-roi.top+step-1)/step);
 c.proxy.width=w;c.proxy.height=h;c.proxy.origin_x=roi.left;c.proxy.origin_y=roi.top;c.proxy.step=step;
 if(c.host->haveMask&&(!c.selection.size()||c.selection_geometry.width!=w||c.selection_geometry.height!=h||c.selection_geometry.origin_x!=roi.left||c.selection_geometry.origin_y!=roi.top||c.selection_geometry.step!=step)){
  auto* channel=c.host->documentInfo?c.host->documentInfo->selection:nullptr;
  if(channel&&!psui::host_read_preview_channel(c.host,channel,c.proxy,c.selection))c.selection_geometry=c.proxy;else c.selection.clear();
 }
 c.background.prepare(c.host,c.proxy);c.display.resize(std::size_t(w)*h*3);const auto& result=c.preview&&c.last.valid()?c.last:c.source;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){const int xx=std::clamp(roi.left+x*step-c.source.left,0,c.source.width-1),yy=std::clamp(roi.top+y*step-c.source.top,0,c.source.height-1);const auto n=(std::size_t(yy)*c.source.width+xx)*4;
  std::array<float,4> before{},after{};for(int k=0;k<4;++k){before[k]=c.source.rgba[n+k];after[k]=result.rgba[n+k];}
  if(c.host->filterCase==filterCaseProtectedTransparencyNoSelection||c.host->filterCase==filterCaseProtectedTransparencyWithSelection)after[3]=before[3];
  if(!c.host->outTransparencyMask){for(int k=0;k<3;++k){before[k]*=before[3];after[k]*=after[3];}before[3]=after[3]=1;}
  const auto colour=c.background.pixel(x,y,before,after,c.selection.empty()?1:c.selection[std::size_t(y)*w+x]);for(int k=0;k<3;++k)c.display[(std::size_t(y)*w+x)*3+k]=colour[k];
 }
 if(!ui.preview(c.host,c.display.data(),w,h))c.error=memFullErr;c.display_dirty=c.view.dirty=false;
}
void draw(psui::LibraryPanel& ui,Context& c){
 ui.parameter_defaults(c.p, defaults(effect_kind,c.source.width,c.source.height,c.source.left,c.source.top));
 const float s=ui.scale();bool changed=false;auto& p=c.p;
 ImGui::BeginChild("preview-side",{205*s,0},0,ImGuiWindowFlags_NoScrollbar);
 const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),205*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-120*s,110*s,205*s));update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["lens.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto rect=ui.items.at("image");if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({rect.x,rect.y},{rect.z,rect.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
  const auto mouse=ImGui::GetMousePos();p.cx=c.view.center_x()+(mouse.x-(rect.x+rect.z)*.5)/(c.view.percent*.01);p.cy=c.view.center_y()+(mouse.y-(rect.y+rect.w)*.5)/(c.view.percent*.01);changed=true;
 }
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.dirty||c.worker.pending())ImGui::TextDisabled("%s",ui.text(labels[12]).c_str());if(c.error)ImGui::TextWrapped("%s",ui.text(labels[13]).c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));
 ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=defaults(effect_kind,c.source.width,c.source.height,c.source.left,c.source.top);c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 if(effect_kind==Kind::optics){changed|=ui.number(labels[0],p.fov,0,179.99998,0,179);changed|=ui.check(labels[1],p.reverse);changed|=ui.choice(labels[2],p.axis,axes,3);changed|=psui::point_control(ui,labels[3],p.cx,p.cy);changed|=ui.check(labels[4],p.optimal);ImGui::TextWrapped("%s",ui.text(labels[9]).c_str());if(p.optimal)ImGui::TextWrapped("%s",ui.text(labels[10]).c_str());}
 else {changed|=psui::point_control(ui,labels[5],p.cx,p.cy);changed|=ui.number(labels[6],p.size,0,500,0,200);changed|=ui.number(labels[7],p.convergence,-200,100,-200,100);}
 if(ui.button("center",labels[8],-1)){p.cx=c.source.left+c.source.width*.5;p.cy=c.source.top+c.source.height*.5;changed=true;}
 ImGui::TextWrapped("%s",ui.text(labels[11]).c_str());ImGui::TextWrapped("%s",ui.text(labels[14]).c_str());ImGui::EndChild();
 if(changed){c.worker.stop();c.dirty=true;c.error=noErr;c.due=GetTickCount64()+35;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* f,const Image& src,Parameters& p){Context c(p,f,src);std::string glyphs;for(auto label:labels)glyphs+=label;for(auto axis:axes)glyphs+=axis;const auto answer=psui::run_library_panel(m,owner,f,title,760,520,[&](psui::LibraryPanel& ui){draw(ui,c);},glyphs.c_str(),&plugin_info);c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;}
}
