#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "roughen_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/adaptive_preview.hpp"
#include "psui/preview_background.hpp"
#include <cmath>
namespace roughen {namespace {
constexpr const char* types[]={"粗糙化 / Roughen","颜色粗糙化 / Roughen Color","剪切 / Cut","刺状 / Spiky","生锈 / Rusty","生锈颜色 / Rusty Color","影印 / Photocopy","影印颜色 / Photocopy Color"};
#include "preview_adapter.hpp"
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;Prepared& prepared;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;ULONGLONG due=0; psui::PreviewSpace adaptive_mapping;psui::DraftImageCache<Image> adaptive_cache;
 psui::ProxyImage adaptive_selection_geometry;std::vector<float> adaptive_selection;
 psui::AdaptivePreview<psui::SpatialFrame<Image>> worker;
 Context(Parameters p,FilterRecord* f,const Image& src,Prepared& cache):p(p),host(f),source(src),prepared(cache){}
};
#include "preview_update.inc"
void draw(psui::LibraryPanel& ui,Context& c){
 const auto factory=defaults();ui.parameter_defaults(c.p,factory);auto& p=c.p;bool changed=false;
 const float footer = ui.filter_footer_height();
 ui.begin_filter_preview(c.view, psui::filter_document_bounds(c.host), footer);update(ui,c);
 const auto bounds = c.view.bounds;
 psui::PreviewAdjustments handles;
 handles.point("偏移（湍流） / Offset", p.offset_x, p.offset_y,
               psui::PreviewPointSpace::offset(.5 * (double(bounds.left) + bounds.right), .5 * (double(bounds.top) + bounds.bottom)));
 changed |= ui.image_view(c.view,c.proxy,handles);
 ui.items["roughen.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;c.worker.controls(ui,c.dirty);if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);

 if(c.error)ImGui::TextWrapped("%s",ui.text("计算失败，请全部重置或取消。 / Render failed. Reset all or cancel.").c_str());
 ImGui::TextWrapped("%s",ui.text("透明文字和图形可直接处理轮廓。 / Use transparent text or graphics to roughen their contours.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();c.adaptive_cache.clear();p=factory;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ui.begin_filter_parameters();
 auto num=[&](const char* label,double& value,double lo,double hi,double slo,double shi,bool enabled=true,bool integer=false){changed|=ui.number(label,value,lo,hi,slo,shi,enabled,integer);};
 if(ui.section("毛边 / Roughen Edges",true)){
  changed|=ui.choice("边缘类型 / Edge Type",p.type,types,8);
  ImGui::BeginDisabled(p.type!=1&&p.type!=5&&p.type!=7);changed|=psui::color_control(ui,c.host,"边缘颜色 / Edge Color",p.color.data());ImGui::EndDisabled();
  num("边界 / Border",p.border,0,500,0,64);num("边缘锐度 / Edge Sharpness",p.sharpness,0,10,0,5);num("分形影响 / Fractal Influence",p.influence,0,1,0,1);num("比例 / Scale",p.scale,10,1000,10,300);
  num("伸缩宽度或高度 / Stretch Width or Height",p.stretch,-100,100,-100,100);changed|=psui::point_control(ui,"偏移（湍流） / Offset",p.offset_x,p.offset_y);num("复杂度 / Complexity",p.complexity,1,10,1,10);
 }
 if(ui.section("演化 / Evolution",true)){num("演化（度） / Evolution (deg)",p.evolution,-1e9,1e9,-720,720);changed|=ui.check("循环演化 / Cycle Evolution",p.cycle);num("循环（旋转次数） / Cycle",p.cycles,1,88,1,10,p.cycle,true);num("随机植入 / Random Seed",p.seed,0,100000,0,1000,true,true);}
 if(ui.section("输出 / Output",true))num("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);
 ImGui::TextWrapped("%s",ui.text("独立重建；随机毛边和细节与AE可能不同。 / Independent reconstruction; random edges and details may differ from AE.").c_str());
 ui.end_filter_parameters();if(changed){c.dirty=true;c.due=GetTickCount64()+35;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p,Prepared& cache){
 Context c(p,host,src,cache);const auto answer=psui::run_filter_panel(m,owner,host,title,900,760,[&](psui::LibraryPanel& ui){draw(ui,c);},
 "毛边 边缘类型 粗糙化 颜色粗糙化 剪切 刺状 生锈 生锈颜色 影印 影印颜色 边缘颜色 边界 边缘锐度 分形影响 比例 伸缩宽度或高度 偏移 湍流 复杂度 演化 度 循环演化 循环 旋转次数 随机植入 输出 效果混合 正在计算 计算失败，请全部重置或取消。透明文字和图形可直接处理轮廓。独立重建；随机毛边和细节与可能不同。",&plugin_info);
 c.worker.stop();c.adaptive_cache.clear();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
