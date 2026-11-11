<!-- parallel-0.24 -->
Glow 0.8 / Deep Glow 0.9：仅并行大图的最终合成，以及Deep Glow每个既有尺度内互不依赖的输出行。各尺度仍按原顺序完成，模糊方法、半径、质量与累加顺序不变。小图保持串行，避免线程调度拖慢预览。固定1024×768案例分别约1.20倍、1.25倍加速，小图基准与原版接近。

此更新随0.24发布。参数存储和宿主生命周期未变，CPU工作线程从不调用PS宿主接口；调用线程负责取消，所有工作线程结束后才释放缓冲。工作线程继承调用者浮点环境。详细逐字节比较、生产DLL/界面与PS记录见`../evidence/parallel-0.24/qa.json`；此前精度/色彩管理和功能限制仍适用。性能为固定输入的CPU内核测试，不含PS传输及显示。下方保留旧版本说明。
<!-- /parallel-0.24 -->

0.21 图标微调：按用户参考图重绘空心吸管图标，使用圆润胶头、圆角护圈、细长管身和渐窄尖端。新增可复用 ui/include/psui/icons.hpp，统一跟随主题文字色与 DPI 缩放。按钮无边框、无背景，仅在悬停/按下时将图标变为蓝色。此次只修改按钮图形及显示版本，保持原有全屏取色、拖动操作与 PS 原生吸管鼠标指针。100%/200% 共用面板已检查。

0.20 共用吸管增强：色块旁改为 PS 风格矢量吸管图标，鼠标指针通过官方 UI Hooks 使用 PS 原生吸管。支持点击后选色，也支持按住图标拖至目标、松开取色。缩览图采样当前预览；画布、其它软件、桌面及界面区域统一读取屏幕显示 RGB，不调用宿主交互式 SamplePoint，完整支持全屏幕取色，无法读取时保持原色。支持负坐标多显示器，预览缩放/平移/裁切，Esc/右键取消以及鼠标捕获清理。版本：Tint 0.21 / Fractal Noise 0.15 / Displacement Map 0.8 / Glow 0.6 / Deep Glow 0.7 / Chromatic Dispersion 0.5 / Vector Blur 0.5 / Grid 0.4 / Checkerboard 0.4。

0.19 共用颜色控件：保留 PS 原生拾色器，在所有颜色块旁新增“取色 / Pick”，可从缩览图的可见结果取色，也可请求 PS 从窗口外的画布采样；Esc/右键/丢失鼠标捕获取消，无法取色的位置保留模式以便重试。统一处理缩放/平移/裁切、显示色彩管理之前的 RGB、目标隔离与鼠标捕获清理，不修改前景色、图层或预览视点。新效果复用 LibraryPanel::color_swatch / color_control 即可继承。详见 native/ui/COLOR-CONTROLS.md。版本：Tint 0.20 / Fractal Noise 0.14 / Displacement Map 0.7 / Glow 0.5 / Deep Glow 0.6 / Chromatic Dispersion 0.4 / Vector Blur 0.4 / Grid 0.3 / Checkerboard 0.3。

0.18 共用界面更新：九款插件的数值统一使用随主题调整的蓝色，数值区域支持左右拖动，Shift 为 10 倍速度、Ctrl 为 0.1 倍速度；轻点或双击进入输入，Enter 确认，Esc 取消本次编辑/拖动并保留滤镜窗口。普通参数、网格/棋盘的双坐标和预览缩放均复用同一组件，拖动速度按 DPI 校正；整数支持细调累积，禁用值不可编辑。后续效果使用 LibraryPanel::number / numeric_value / point_control 即可继承。版本：Tint 0.19 / Fractal Noise 0.13 / Displacement Map 0.6 / Glow 0.4 / Deep Glow 0.5 / Chromatic Dispersion 0.3 / Vector Blur 0.3 / Grid 0.2 / Checkerboard 0.2。

# Glow 0.2 / Deep Glow 风格发光 0.3

Deep Glow 0.3 修正发光偏弱：十个对数尺度叠加，同时保留亚像素近处辉光和宽范围扩散；不再把所有尺度随半径同比放大并平均。光能完成伽马转换后再生成输出 alpha，避免透明图层在合成时再次压暗辉光。Unmult 开关在同一个输出颜色空间中执行一次预乘/取消预乘转换。原参数结构保留，旧设置无需重新输入，但重新计算后会明显更亮。

透明素材按发光源的有效范围缩小细尺度计算，背景预览仍由共用组件处理。完整画布参考对照验证了该优化没有遗漏光晕。`evidence/deep-glow-0.3-comparison.png` 为本项目 0.2 与 0.3 使用同一自制测试图、半径 1000、曝光 1 的对照；不是与原版 Deep Glow 的像素一致性校准。白色细线、伽马/Alpha、Unmult、橙色高光、8/16/32 位和 PS 实机撤销/背景开关验证均通过；原生 Glow 的旧/新内核输出逐字节一致。

0.12 共用组件：五款缩览图统一新增“显示背景”复选框。勾选显示当前效果图层及其下方可见图层，不显示上方图层；关闭恢复透明棋盘。仅改变预览，不修改图层、选区、参数或历史。新增 preview_background.hpp / preview_background.cpp；图层读取均为只读 Channel Ports，按调用索引而不是持久 ID 排序，缓存按预览原点、尺寸与采样步长更新。宿主可能返回 Action Manager 混合 ID，现统一映射为 SDK 混合模式。支持图层透明度、填充透明度、启用的像素/矢量蒙版与常用混合模式。调整图层、图层组及高级混合/剪贴设置不能完整还原；可识别的缺失路径标注“近似预览”，读不到安全数据时回退当前层棋盘并提示不可用，绝不使用含上方层的整文档 merged 数据替代。

版本：Tint 0.17 / Fractal Noise 0.11 / Displacement Map 0.4 / Glow 0.2 / Deep Glow 0.2。原算法和最终输出流程未改变。实机对照与五款生产窗口结果见 evidence/preview-background-qa.json。


两个 Windows x64 `.8bf`，菜单统一为 **ZaYu-AE 效果复刻**。

- `ZaYuGlow.8bf` → `Glow 发光`：参考 AE 原生 Glow 的常用参数和操作方式。
- `ZaYuDeepGlow.8bf` → `Deep Glow 深度发光`：参考 Deep Glow 的使用方式，以自主多尺度算法实现柔和、宽范围光晕。不是 Plugin Everything 原版产品，没有使用其代码、二进制或素材，不是官方移植版本。

无需安装 AE 或 Deep Glow。源代码免费开放。署名：by：淚泪泪。

## 参数

Glow：颜色/Alpha 输入、阈值、半径、强度、原图前后合成或仅发光、5 种混合方式、原色/双色、三角/锯齿循环、次数/相位/中点、颜色 A/B、水平/垂直发光。

Deep Glow 风格：指数/宽尾光晕、半径/曝光倍率、5 种混合方式、亮度/最大通道/Alpha 阈值、平滑过渡、饱和度偏差、现有图层遮罩（Alpha/亮度及反转）、手动工作伽马与校正量、长宽比/角度、像素色差/通道半径差、单色/内外双色、色调映射、现有图层镜头污垢纹理、质量、最终/仅发光/发光源视图、源透明度、取消预乘和边缘扩展。

遮罩和纹理包含隐藏图层，按整个文档画布读取；不包含图层蒙版。引用通过唯一名称验证；改名、重名或图层删除后需重新选择。不会切换用户的活动图层或要求先保存 PSD。

## 共用界面

复用 PSUI/ImGui 的主题、DPI、中英即时切换、关于、折叠与滚动参数区、预览边框、滚轮缩放、拖动平移与适合窗口。新增 `native/ui/include/psui/async_preview.hpp`：CPU 工作线程可取消，合并 50ms 内的改动，结果完整后在 UI 线程上传纹理，计算中保留上一帧。任何 PS 回调或 ImGui/Direct3D 操作均在宿主线程执行。

预览从原始全分辨率 RGBA 计算，放大时重新采样完整结果；视图缩放/平移不改变效果参数。点击确定后生成一次滤镜历史记录，取消不写文档。

## 算法与边界

- RGB8、Photoshop 原生 16 位（32768 标尺）和 float32/HDR；读取非预乘源颜色，透明像素中的隐藏 RGB 不发光，输出可编辑 alpha。保护透明度时由 PS 保持原透明范围。
- Glow 使用三次可分离小数半径盒式模糊近似高斯；Deep 使用十个对数尺度的光晕叠加。宽尾模式为自主视觉近似，不声称是原版的精确反平方实现。
- 质量影响宽光晕的工作分辨率，亮区提取在降采样前执行；旋转只作用于非等比光晕，不旋转原图。32 位输出不在 1.0 处截断。
- 输出限于 PS 提供的滤镜作用范围及文档画布，不扩展画布。建议在发光对象周围预留透明空间。
- 内核 RGB 使用文档原始通道值；工作伽马由用户设置，默认 2.2。没有自动解析 ICC、ACES 或 AE 项目色彩管理。预览把 HDR 限幅到显示范围，因此不保证与 PS 最终合成显示完全相同。
- 本版不提供 PS 主画布实时预览、GPU 计算、Deep Glow 2 自定义光圈/图像卷积、AMP 任意颜色曲线或全部 AE 混合模式。没有与 AE/Deep Glow 完成像素一致性校准。
- 单次完整快照最多 16,777,216 像素（沿用共用读取器限制）。更大图像需要后续分块加光晕边界缓存方案；内存不足会报错，不静默降采样最终输出。

## 验证

`build.ps1` 构建两款内核、真实 DLL 模拟宿主测试和生产窗口预览程序。模拟宿主覆盖 RGB8/PS16/HDR32、透明/背景、9 个不规则输出块、选区写入保护、取消/错误清理与 16/32 位坐标。

生产窗口程序支持 `--qa-zh`、`--qa-en`、`--qa-zh-200`、`--qa-en-200`，在 D3D 软件渲染下检查后台预览、半径输入、关于、布局、确认及线程退出。

`tests/host_smoke.jsx`、`tests/host_depth.jsx` 只创建临时 PS 文档，并保留已有文档；`tests/verify_host.py` 检查实际导出的原生 8/16/float32 RGBA、向透明区域扩展、一次撤销和选区外像素。证据见 `evidence/glow-qa.json`。实际 PS 27.7 测试已通过；参数窗口由用户确认可见并手动取消，自动截图来自生产窗口测试程序。

公开参考：[Adobe Glow 参数](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/stylize-effects.html)、[Plugin Everything Deep Glow](https://www.plugineverything.com/deep-glow)。
