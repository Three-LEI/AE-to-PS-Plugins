#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "creative_host.hpp"
#include "psui/library_panel.hpp"
#include "psui/generator_controls.hpp"
#include "psui/async_preview.hpp"
#include "psui/preview_background.hpp"
#include <cmath>
namespace creative {namespace {
constexpr const char* types[]={"湍流 / Turbulent","膨胀 / Bulge","扭转 / Twist","湍流（平滑） / Turbulent Smoother","膨胀（平滑） / Bulge Smoother","扭转（平滑） / Twist Smoother","垂直置换 / Vertical","水平置换 / Horizontal","交叉置换 / Cross"};
constexpr const char* pins[]={"无 / None","固定全部 / Pin All","固定上下 / Pin Horizontal","固定左右 / Pin Vertical","固定左 / Pin Left","固定右 / Pin Right","固定上 / Pin Top","固定下 / Pin Bottom","锁定全部 / Lock All","锁定上下 / Lock Horizontal","锁定左右 / Lock Vertical","锁定左 / Lock Left","锁定右 / Lock Right","锁定上 / Lock Top","锁定下 / Lock Bottom"};
constexpr const char* edges[]={"透明 / Transparent","延伸 / Clamp","平铺 / Wrap","镜像 / Mirror"};
constexpr const char* qualities[]={"快速（近邻） / Fast (Nearest)","标准（双线性） / Standard (Bilinear)","高质量（四点） / High (4 Samples)"};
constexpr const char* shapes[]={"线性 / Linear","平滑 / Smooth","锐利 / Sharp"};
constexpr const char* receptions[]={"叠加 / Add","合成 / Composite","剪出 / Cutout"};
constexpr const char* mirrors[]={"展开 / Unfold","轮形 / Wheel","鱼头 / Fish Head","罐形 / Can Meas","翻转 / Flip Flop","花形 / Flower","菱形十字 / Dia Cross","翻折 / Flipper","星形 / Starlish"};
struct Context {
 Parameters p;FilterRecord* host;const Image& source;Image last;
 psui::PreviewView view;psui::ProxyImage proxy,selection_geometry;psui::PreviewBackground background;
 std::vector<std::uint8_t> display;std::vector<float> selection;
 bool dirty=true,display_dirty=true,preview=true;OSErr error=noErr;ULONGLONG due=0;psui::AsyncPreview<Image> worker;
 Context(Parameters p,FilterRecord* f,const Image& src):p(p),host(f),source(src){}
};
#include "preview_update.inc"
void draw(psui::LibraryPanel& ui,Context& c){
 const auto b=c.host->documentInfo->bounds;const auto factory=defaults(effect_kind,b.right-b.left,b.bottom-b.top,b.left,b.top);ui.parameter_defaults(c.p,factory);auto& p=c.p;const float s=ui.scale();bool changed=false;
 ImGui::BeginChild("preview-side",{220*s,0},0,ImGuiWindowFlags_NoScrollbar);const float footer=3*(ImGui::GetFrameHeight()+ImGui::GetStyle().ItemSpacing.y);
 c.view.configure(psui::filter_bounds(c.host),220*s,std::clamp(ImGui::GetContentRegionAvail().y-footer-125*s,110*s,220*s));update(ui,c);ui.image_view(c.view,c.proxy);
 ui.items["creative.state"]={c.last.valid()?1.f:0.f,c.worker.pending()?1.f:0.f,c.dirty?1.f:0.f,float(c.error)};
 const auto r=ui.items.at("image");if(ImGui::IsWindowHovered()&&ImGui::IsMouseHoveringRect({r.x,r.y},{r.z,r.w})&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){const auto m=ImGui::GetMousePos();p.cx=c.view.center_x()+(m.x-(r.x+r.z)*.5)/(c.view.percent*.01);p.cy=c.view.center_y()+(m.y-(r.y+r.w)*.5)/(c.view.percent*.01);changed=true;}
 if(ui.check("预览 / Preview",c.preview))c.display_dirty=true;if(psui::preview_background_control(ui,c.background))c.display_dirty=true;ui.preview_zoom(c.view);
 if(c.dirty||c.worker.pending())ImGui::TextDisabled("%s",ui.text("正在计算… / Rendering preview…").c_str());
 if(c.error)ImGui::TextWrapped("%s",ui.text("计算失败，请全部重置或取消。 / Render failed. Reset all or cancel.").c_str());
 ImGui::TextWrapped("%s",ui.text("右键预览可定位中心。 / Right-click the preview to set the center.").c_str());
 ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.f,ImGui::GetContentRegionAvail().y-footer));ImGui::BeginDisabled(c.error!=noErr);if(ui.button("ok","确定 / OK",-1))ui.close(true);ImGui::EndDisabled();if(ui.button("cancel","取消 / Cancel",-1))ui.close(false);
 if(ui.button("reset","全部重置 / Reset All",-1)){c.worker.stop();p=factory;c.error=noErr;changed=true;}
 ImGui::EndChild();ImGui::SameLine();ImGui::BeginChild("parameters",{0,0});
 auto num=[&](const char* label,double& value,double lo,double hi,double slo,double shi,bool enabled=true,bool integer=false){changed|=ui.number(label,value,lo,hi,slo,shi,enabled,integer);};
 if(effect_kind==Kind::turbulent){
  if(ui.section("湍流 / Turbulence",true)){
   changed|=ui.choice("置换类型 / Displacement",p.type,types,9);num("数量 / Amount",p.amount,-10000,10000,-200,200);num("大小 / Size",p.size,2,1000,2,500);changed|=psui::point_control(ui,"偏移（湍流） / Offset",p.cx,p.cy);num("复杂度 / Complexity",p.complexity,1,10,1,10);
   changed|=ui.choice("固定边缘 / Pinning",p.pinning,pins,15);ImGui::TextDisabled("%s",ui.text("图层扩展：关闭（PS固定边界） / Resize Layer: Off (PS fixed bounds)").c_str());
  }
  if(ui.section("演化 / Evolution",true)){num("演化（度） / Evolution (deg)",p.evolution,-1e9,1e9,-720,720);changed|=ui.check("循环演化 / Cycle Evolution",p.cycle);num("循环（转） / Cycle",p.cycles,1,88,1,10,p.cycle,true);num("随机种子 / Random Seed",p.seed,0,100000,0,1000,true,true);}
  if(ui.section("方向调整 / Axis Controls",false)){num("水平强度（%） / Horizontal (%)",p.horizontal,-500,500,0,200);num("垂直强度（%） / Vertical (%)",p.vertical,-500,500,0,200);}
 }else if(effect_kind==Kind::sweep){
  if(ui.section("扫光 / Light Sweep",true)){changed|=psui::point_control(ui,"中心 / Center",p.cx,p.cy);num("方向（度） / Direction (deg)",p.direction,-360000,360000,-180,180);changed|=ui.choice("形状 / Shape",p.shape,shapes,3);num("宽度 / Width",p.width,0,4000,0,500);num("扫光强度 / Sweep Intensity",p.intensity,0,500,0,200);changed|=psui::color_control(ui,c.host,"光照颜色 / Light Color",p.color.data());changed|=ui.choice("光照接收 / Light Reception",p.reception,receptions,3);}
  if(ui.section("边缘 / Edges",true)){num("边缘强度 / Edge Intensity",p.edge_intensity,0,500,0,200);num("边缘厚度 / Edge Thickness",p.thickness,0,20,0,20);}
 }else {
  if(ui.section("万花筒 / Kaleida",true)){changed|=psui::point_control(ui,"中心 / Center",p.cx,p.cy);num("大小（%） / Size (%)",p.size,1,1000,1,100);changed|=ui.choice("镜像类型 / Mirroring",p.mirroring,mirrors,9);num("旋转（度） / Rotation (deg)",p.rotation,-360000,360000,-180,180);changed|=ui.check("浮动中心 / Floating Center",p.floating);}
 }
 if(ui.section("输出 / Output",true)){if(effect_kind!=Kind::sweep){changed|=ui.choice("边界采样 / Edge Sampling",p.edge,edges,4);changed|=ui.choice("采样质量 / Quality",p.quality,qualities,3);}num("效果混合（%） / Effect Mix (%)",p.mix,0,100,0,100);}
 ImGui::TextWrapped("%s",ui.text("独立重建；随机形态及细节与AE可能不同。 / Independent reconstruction; random patterns and details may differ from AE.").c_str());
 ImGui::EndChild();if(changed){c.worker.stop();c.dirty=true;c.due=GetTickCount64()+35;}
}
}
INT_PTR show_dialog(HINSTANCE m,HWND owner,FilterRecord* host,const Image& src,Parameters& p){
 Context c(p,host,src);const auto answer=psui::run_library_panel(m,owner,host,title,900,760,[&](psui::LibraryPanel& ui){draw(ui,c);},
 "锁定 湍流置换 扫光 万花筒 湍流 膨胀 扭转 平滑 垂直 水平 交叉 置换类型 数量 大小 偏移 复杂度 固定边缘 无 全部边缘 左 右 上 下 左右 上下 左上 右上 左下 右下 除左边 除右边 除上边 除下边 图层扩展 关闭 固定边界 演化 度 循环演化 循环 转 随机种子 方向调整 水平强度 垂直强度 中心 方向 形状 线性 锐利 宽度 扫光强度 光照颜色 光照接收 叠加 合成 剪出 边缘 边缘强度 边缘厚度 镜像类型 展开 轮形 鱼头 罐形 翻转 花形 菱形十字 翻折 星形 旋转 浮动中心 输出 边界采样 透明 延伸 平铺 镜像 采样质量 快速 近邻 标准 双线性 高质量 四点 效果混合 正在计算 计算失败，请全部重置或取消。右键预览可定位中心。独立重建；随机形态及细节与可能不同。",&plugin_info);
 c.worker.stop();if(answer==IDOK&&!c.error)p=c.p;return answer;
}
}
