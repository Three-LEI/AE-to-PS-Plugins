0.21 图标微调：按用户参考图重绘空心吸管图标，使用圆润胶头、圆角护圈、细长管身和渐窄尖端。新增可复用 ui/include/psui/icons.hpp，统一跟随主题文字色与 DPI 缩放。按钮无边框、无背景，仅在悬停/按下时将图标变为蓝色。此次只修改按钮图形及显示版本，保持原有全屏取色、拖动操作与 PS 原生吸管鼠标指针。100%/200% 共用面板已检查。

0.20 共用吸管增强：色块旁改为 PS 风格矢量吸管图标，鼠标指针通过官方 UI Hooks 使用 PS 原生吸管。支持点击后选色，也支持按住图标拖至目标、松开取色。缩览图采样当前预览；画布、其它软件、桌面及界面区域统一读取屏幕显示 RGB，不调用宿主交互式 SamplePoint，完整支持全屏幕取色，无法读取时保持原色。支持负坐标多显示器，预览缩放/平移/裁切，Esc/右键取消以及鼠标捕获清理。版本：Tint 0.21 / Fractal Noise 0.15 / Displacement Map 0.8 / Glow 0.6 / Deep Glow 0.7 / Chromatic Dispersion 0.5 / Vector Blur 0.5 / Grid 0.4 / Checkerboard 0.4。

0.19 共用颜色控件：保留 PS 原生拾色器，在所有颜色块旁新增“取色 / Pick”，可从缩览图的可见结果取色，也可请求 PS 从窗口外的画布采样；Esc/右键/丢失鼠标捕获取消，无法取色的位置保留模式以便重试。统一处理缩放/平移/裁切、显示色彩管理之前的 RGB、目标隔离与鼠标捕获清理，不修改前景色、图层或预览视点。新效果复用 LibraryPanel::color_swatch / color_control 即可继承。详见 native/ui/COLOR-CONTROLS.md。版本：Tint 0.20 / Fractal Noise 0.14 / Displacement Map 0.7 / Glow 0.5 / Deep Glow 0.6 / Chromatic Dispersion 0.4 / Vector Blur 0.4 / Grid 0.3 / Checkerboard 0.3。

0.18 共用界面更新：九款插件的数值统一使用随主题调整的蓝色，数值区域支持左右拖动，Shift 为 10 倍速度、Ctrl 为 0.1 倍速度；轻点或双击进入输入，Enter 确认，Esc 取消本次编辑/拖动并保留滤镜窗口。普通参数、网格/棋盘的双坐标和预览缩放均复用同一组件，拖动速度按 DPI 校正；整数支持细调累积，禁用值不可编辑。后续效果使用 LibraryPanel::number / numeric_value / point_control 即可继承。版本：Tint 0.19 / Fractal Noise 0.13 / Displacement Map 0.6 / Glow 0.4 / Deep Glow 0.5 / Chromatic Dispersion 0.3 / Vector Blur 0.3 / Grid 0.2 / Checkerboard 0.2。

0.3：新增仅在预览框内触发的滚轮缩放，沿鼠标位置缩放，支持快速移入/移出。兼容只有背景层的图像。共用组件 0.10。

0.2：新增共用预览平移与边框。放大后按住左键拖动，双击居中；适合恢复完整居中图像。仅影响预览视图。

# Displacement Map 置换图 0.1

独立 Windows x64 Photoshop `.8bf`，不需要安装 AE。菜单：滤镜 → ZaYu-AE 效果复刻 → Displacement Map 置换图。

在要变形的 RGB 图层上打开滤镜，在“置换图层”下拉列表选择当前文档的采样图层。文档和采样层均不需要另存 PSD，隐藏图层也可以采样。参数支持水平/垂直通道、正负最大位移、中心图/拉伸适合/平铺图、像素回绕；共用预览开关、预览缩放、语言、主题、DPI 和关于弹窗。

采样通道：红、绿、蓝、Alpha、亮度、色相、明度、饱和度、关闭。8 位中灰 128 为零位移，黑/白分别为负/正最大位移。双线性源图取样使用预乘透明度，避免透明边缘出现色边。原图浮点 HDR 值保留；采样图 RGB 用于驱动位移时限制在 0–1。选区由 Photoshop 的 autoMask 处理。

## 实现与验证

`host_layers.hpp / host_layers.cpp` 属于共用 PSUI：通过官方 SDK 的 `documentInfo.layersDescriptor` 枚举图层，使用 Channel Ports 的 `readPixelsProc` 读取各通道；名称取 Unicode 字段，不切换目标图层、不改变显示状态、不填充原图。源图使用权威的 `targetCompositeChannels`，不能用两处描述符的端口句柄相等来推断目标，因为 PS 27.7 会提供不同句柄。

PS 27.7 实机的 `sheetID` 会随图层重排变化，不能当成永久图层 ID。本版保存采样层名称指纹，在名称唯一时重新定位；重名或采样层改名/删除后，重复滤镜会报错，需重新打开并明确选择。尚未提供跨图层智能滤镜的依赖自动失效/重算协议。

实机 RGB8 验证：未保存文档内选择隐藏采样层，彩条精确水平右移 5 像素、垂直不动，左侧 5 列透明；49,601 个像素逐一检查。一次撤销恢复原始 RGBA；最终版本在采样层重排后重复滤镜，结果逐字节相同。RGB16/32、透明度、9 个不规则分块、选区外/行填充保护、错误/取消路径、端口句柄差异、重排/重名/改名由真实 DLL 的模拟宿主测试覆盖。中英文生产界面测试覆盖图层切换、参数、预览缩放、关于、重置和确定。

构建：先运行 `native/build.ps1`，再运行 `displacement/build.ps1`。核心检查可单独运行 `displacement/build.ps1 -CoreOnly`。自有窗口验证：`build/dialog_preview.exe --qa-zh` 或 `--qa-en`。

## 第一版范围

- 读取 PS 接口提供的图层源像素，不额外合并图层蒙版、图层样式和 AE 的“效果和蒙版”管线。
- 输出以 PS 当前滤镜作用范围为界，暂未实现 AE 的“扩展输出”，所以没有放置无效的同名复选框。需透明边缘时使用可编辑透明度的普通图层；背景图层本身无法保存透明像素。
- 每次调用先缓存源图和采样图，确保分块写回时不会读到已被滤镜修改的像素。每份原生 RGBA 快照上限 256 MiB，超出时报内存错误，不悄悄降低最终分辨率；大图的分块缓存是后续优化方向。
- 尚未进行与 AE 的像素级一致性比较。此版本是按公开行为实现的独立复刻。

依据：本机官方 SDK `PIGeneral.h` 的 ReadLayerDesc/ReadChannelDesc/ReadPixelsProc，以及 [Adobe 的置换图说明](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/distort-effects.html)。
