0.21 图标微调：按用户参考图重绘空心吸管图标，使用圆润胶头、圆角护圈、细长管身和渐窄尖端。新增可复用 ui/include/psui/icons.hpp，统一跟随主题文字色与 DPI 缩放。按钮无边框、无背景，仅在悬停/按下时将图标变为蓝色。此次只修改按钮图形及显示版本，保持原有全屏取色、拖动操作与 PS 原生吸管鼠标指针。100%/200% 共用面板已检查。

0.20 共用吸管增强：色块旁改为 PS 风格矢量吸管图标，鼠标指针通过官方 UI Hooks 使用 PS 原生吸管。支持点击后选色，也支持按住图标拖至目标、松开取色。缩览图采样当前预览；画布、其它软件、桌面及界面区域统一读取屏幕显示 RGB，不调用宿主交互式 SamplePoint，完整支持全屏幕取色，无法读取时保持原色。支持负坐标多显示器，预览缩放/平移/裁切，Esc/右键取消以及鼠标捕获清理。版本：Tint 0.21 / Fractal Noise 0.15 / Displacement Map 0.8 / Glow 0.6 / Deep Glow 0.7 / Chromatic Dispersion 0.5 / Vector Blur 0.5 / Grid 0.4 / Checkerboard 0.4。

0.19 共用颜色控件：保留 PS 原生拾色器，在所有颜色块旁新增“取色 / Pick”，可从缩览图的可见结果取色，也可请求 PS 从窗口外的画布采样；Esc/右键/丢失鼠标捕获取消，无法取色的位置保留模式以便重试。统一处理缩放/平移/裁切、显示色彩管理之前的 RGB、目标隔离与鼠标捕获清理，不修改前景色、图层或预览视点。新效果复用 LibraryPanel::color_swatch / color_control 即可继承。详见 native/ui/COLOR-CONTROLS.md。版本：Tint 0.20 / Fractal Noise 0.14 / Displacement Map 0.7 / Glow 0.5 / Deep Glow 0.6 / Chromatic Dispersion 0.4 / Vector Blur 0.4 / Grid 0.3 / Checkerboard 0.3。

0.18 共用界面更新：九款插件的数值统一使用随主题调整的蓝色，数值区域支持左右拖动，Shift 为 10 倍速度、Ctrl 为 0.1 倍速度；轻点或双击进入输入，Enter 确认，Esc 取消本次编辑/拖动并保留滤镜窗口。普通参数、网格/棋盘的双坐标和预览缩放均复用同一组件，拖动速度按 DPI 校正；整数支持细调累积，禁用值不可编辑。后续效果使用 LibraryPanel::number / numeric_value / point_control 即可继承。版本：Tint 0.19 / Fractal Noise 0.13 / Displacement Map 0.6 / Glow 0.4 / Deep Glow 0.5 / Chromatic Dispersion 0.3 / Vector Blur 0.3 / Grid 0.2 / Checkerboard 0.2。

0.17：新增 Grid 网格 / Checkerboard 棋盘 0.1，现共九款效果。支持三种尺寸依据、锚点/边角、独立羽化、颜色/透明度/混合、旋转和预设；可在透明空层直接生成。新增分块生成运行器、标准像素合成、双坐标/色块控件及原生预览快照，可供后续生成效果复用。实际 PS 已核对 8/16/32 位空层/非空层、选区、锁定 Alpha、平面图和撤销；63 组同参数 AE 参考见 evidence/patterns/ae-parity.json。既有七款二进制未改变。

0.16：Vector Blur 矢量模糊 0.2 通过同源 AE 25.6.5x3 校准。修正自然模式的单向坡度累积、后向差分/法线响应、映射柔化尺度、其它类型的权重与方向角度基准。曲线流向默认关闭，保留艺术化选项。0.1 参数保留数值并自动迁移；其余六款二进制与共享宿主 ABI 未改动。21组参考见 evidence/vector-v02/vector-ae-parity.json；PS/界面/旧参数迁移与安装校验见 evidence/vector-v02/vector-qa.json。公共组件包加入校准后的源码、迁移与同源参考生成步骤。

0.15：新增第七款 Vector Blur 矢量模糊 0.1。五种类型、八种通道、PS 图层矢量图、可选曲线流向、方向箭头、质量/混合/边缘/透明度及预设。复用缓存空间滤镜运行器、只读图层快照、异步预览、背景、缩放平移、主题/语言/DPI/关于等组件。当前七款统一使用“英文 中文”名称，ComponentName 与已有六款参数存储保持不变。版本：Tint 0.18 / Fractal Noise 0.12 / Displacement Map 0.5 / Glow 0.3 / Deep Glow 0.4 / Chromatic Dispersion 0.2 / Vector Blur 0.1。已有六款仅更新名称和显示版本；Tint 现有宿主支持范围为 RGB8，其它六款为 RGB8/16/32。验证和限制见 vector/README.md、evidence/vector-qa.json。

0.14 共用组件与第六款效果：Chromatic Dispersion 色差色散 0.1。两种模式（RGB 分离 / 连续光谱）、直线/径向/切向、逐通道变换、光谱采样/色相/柔化、中心衰减、四种边缘、透明度策略、高光和图层遮罩。新增 common/cached_filter_runtime.hpp，统一缓存型空间滤镜的原生 RGBA 快照、参数、命名输入、完整结果、分块输出、选区和取消清理；新增 host_read_preview_channel 供选区/预览蒙版共用。旧五款插件的安装文件与核心算法未改动。测试与参考见 chromatic/README.md 和 evidence/chromatic-host-qa.json。

0.12 共用组件：五款缩览图统一新增“显示背景”复选框。勾选显示当前效果图层及其下方可见图层，不显示上方图层；关闭恢复透明棋盘。仅改变预览，不修改图层、选区、参数或历史。新增 preview_background.hpp / preview_background.cpp；图层读取均为只读 Channel Ports，按调用索引而不是持久 ID 排序，缓存按预览原点、尺寸与采样步长更新。宿主可能返回 Action Manager 混合 ID，现统一映射为 SDK 混合模式。支持图层透明度、填充透明度、启用的像素/矢量蒙版与常用混合模式。调整图层、图层组及高级混合/剪贴设置不能完整还原；可识别的缺失路径标注“近似预览”，读不到安全数据时回退当前层棋盘并提示不可用，绝不使用含上方层的整文档 merged 数据替代。

版本：Tint 0.17 / Fractal Noise 0.11 / Displacement Map 0.4 / Glow 0.2 / Deep Glow 0.2。原算法和最终输出流程未改变。实机对照与五款生产窗口结果见 evidence/preview-background-qa.json。

0.11 公共组件：新增可复用的 async_preview.hpp CPU 后台预览任务。任务可取消、替换并等待退出，保留上一帧完整纹理；PS 回调、ImGui 和 GPU 上传仍在主线程执行。Glow 0.1 / Deep Glow 0.1 使用此组件，共用主题、中英切换、DPI、关于和缩放平移。Deep Glow 为独立多尺度视觉重建，不是第三方原版移植；详见 glow/README.md。

0.10 公共组件：预览框内支持滚轮缩放（每格 1.2 倍，锚定鼠标下的图像位置），框外滚动不缩放。Win32 WM_MOUSEWHEEL 按消息的屏幕坐标路由，避免快速移出时旧 ImGui 悬停状态误触发；不额外要求中键按下。保留拖动平移、双击居中和适合。

置换图修复单背景/平面图像首次调用报错：PS 可以只提供 targetCompositeChannels 而不提供 layersDescriptor，现将有效的当前目标作为采样项。源像素和读取接口均缺失时仍正常报错；不会伪造不可读的图层。正式默认构建关闭 PortProbe 诊断。证据见 evidence/wheel-flat-qa.json。

0.9 公共组件：三款预览统一支持放大后左键拖动平移、双击居中、适合按钮恢复居中与完整画面；预览区增加跟随主题/DPI 的边框。平移以屏幕像素换算文档坐标，限制在作用范围边缘；缩放/窗口尺寸改变时约束视点。PreviewView 只保存视图状态，不修改滤镜参数。image_view 改为接收可修改的 PreviewView，需重新编译调用方。ROI 更新期间保留上一幅完整纹理及对应显示变换，避免新旧视图混用。验证见 evidence/preview-pan-qa.json。

0.8：统一客户区创建尺寸与最小尺寸，避免初次移动触发窗口撑高；根窗口禁止滚动，参数区独立处理。新增 host_layers.hpp / host_layers.cpp，只读访问当前文档图层的 Unicode 名称及原生 8/16/32 位像素。集成规范见 ../common/README.md。

0.7：新增 plugin_info.hpp、preview_view.hpp，共用关于模态弹窗、作者/版本声明、百分比预览缩放及“适合”。host_read_proxy_area 重新读取中心裁切并返回原点/采样步长，image_view 按真实像素倍率显示；数值和圆形轨道继续复用库控件。接入规范见 ../common/README.md。

0.6：新增 preview_composite.hpp，统一处理生成效果、源 alpha、生成的 alpha、选区和棋盘格预览，避免透明源让生成结果不可见。效果自行指定是否填充输出 alpha；当前 Fractal 使用，Tint 保持原逻辑。

0.5：新增 localization.hpp 和宿主 propLocale 读取。LibraryPanel 自动按 PS 语言显示，可在右上角即时切换中文 / English / 跟随 PS；同次 PS 会话共用选择，参数与控件状态不重置。直接文字通过 ui.text() 翻译，控件 ID 保持使用原始元数据。完整说明见 ../common/README.md。

0.4：新增 ui_metrics.hpp，滑块外观、目标 DPI 字体重建和线程级 DPI 恢复统一由 LibraryPanel 提供。

0.3：新效果默认使用 library_panel.hpp / library_panel.cpp（Dear ImGui + Win32/DX11），不再扩展旧的原生自绘控件。接入规范和构建检查见 ../common/README.md。

0.2 新增 `combo.hpp`（主题下拉框）和 `scroll_panel.hpp`（折叠组头、滚动条、布局与焦点滚动），代理图读取支持 8/16/32 位。接入方式见 `../common/README.md`。

# PSUI：原生滤镜共用控件

Windows x64 C++20 小型静态库，构建产物为 `../build/psui.lib`。Tint 0.8 与 Fractal Noise 0.2 已共同使用此库。完整组件包与宿主适配说明见 `../common/README.md`。最终 `.8bf` 内包含库代码，用户无需额外安装 DLL、AE 或开发工具。

## 模块

| 文件 | 可复用内容 |
| --- | --- |
| `include/psui/widgets.hpp`、`src/widgets.cpp` | 四档主题、文字/输入框配色、按钮、色块、复选框、RGB 预览绘制、居中、按需展开的错误行、独立滑块 |
| `include/psui/photoshop_host.hpp`、`src/photoshop_host.cpp` | PS 界面亮度、PS 原生 RGB 选色器 |
| `src/proxy.cpp` | `advanceState` / `inputRate` 输入缩略图读取与释放，透明通道、选区蒙版、请求状态恢复、PS `displayPixels` 显示回调 |

`widgets.cpp` 不依赖 Tint 或 Photoshop SDK；宿主桥接单独放在另外两个源文件中。参数算法与业务事件保留在 `../src/tint_dialog.cpp`。

## 接入一个新滤镜

1. 将 `ui/include` 添加到 include 路径，链接 `psui.lib`、`user32`、`gdi32`、`dwmapi`；宿主桥接使用官方 Photoshop SDK 头文件。
2. 创建窗口前调用 `psui::register_widgets(module)`。在资源中使用 `"ZaYu.PSUI.Slider.1"` 类名创建带 `WS_TABSTOP` 的滑块。
3. 在窗口上下文中持有 `psui::Theme`，其生命周期必须覆盖全部子控件。初始化时 `theme.apply(dialog, psui::host_brightness(record), true)`，再将滑块通过 `slider_bind` 绑定到主题。
4. 用 `slider_range`、`slider_steps`、`slider_set` 设置整数范围/步长/值。滑块支持鼠标捕获拖动、方向键、PageUp/PageDown、Home/End 和 Tab 导航。父窗口接到 `WM_HSCROLL` 时用 `slider_get` 读取值；不使用 `wParam` 的 16 位位置字段。
5. `WM_DRAWITEM` 中调用 `draw_button`、`draw_checkbox`；`draw_button` 的可选 RGB 参数用于色块。`WM_CTLCOLOR*` 调用 `theme.control_color`。主题变化后 `Theme::apply` 自动通知全部子控件重绘。
6. 对话框结束、子控件销毁后调用 `psui::release_widgets(module)`，避免 DLL 卸载后遗留窗口类回调。

完整接入范例是 `../src/tint_dialog.cpp` 和 `../src/tint.rc`。PS 风格由我们绘制，不使用 Adobe 私有 UI 框架；当前自定义滑块和复选框具备键盘操作，但尚未提供完整的专用 UI Automation 数值/勾选语义接口。

## 滑块绘制与残影处理

滑块使用自己的窗口过程，不再使用 Windows 原生 trackbar 的 `NM_CUSTOMDRAW`。它拦截背景擦除，每次重绘完整背景、轨道和滑块，通过复用的内存 DC/位图一次提交；值变化会使整个控件失效，不留旧位置脏区。每个控件的缓冲在尺寸变化或销毁时释放，不逐帧保留 GDI 对象。

`../tests/ui_tests.cpp` 检查四档主题下 960 帧白色填充后的完整覆盖、位置往返的像素一致性、键盘/范围、GDI 句柄数量和窗口类释放。

## 预览边界

`host_read_proxy` 只请求输入，始终将输出矩形置空；复制缩略图后释放宿主缓冲并恢复原请求、采样率、布局、指针等字段。失败或取消会传播宿主错误，遵循 SDK 在 `advanceState` 返回错误后不再重复调用的要求。

Tint 的预览窗口显示当前目标图层/选区区域的缩略图，参数改变后以 25 ms 合并更新；取消勾选显示原图。预览保留透明度的棋盘背景，并按选区蒙版混合效果。它不临时写入 PS 主画布，点击确定后才走全分辨率滤镜流程。不同图层混合模式、上方调整图层或最终合成不能仅凭这个缩略图准确模拟。

宿主支持时使用 `displayPixels` 绘制；开发预览程序使用 GDI RGB 回退。宿主缺少缩略图 API 时显示“无预览”并禁用预览开关。读取桥接可将 RGB 8/16/32 位归一化为显示用的 8 位代理图，不能精确预览 HDR 混合结果。
