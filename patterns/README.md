0.21 图标微调：按用户参考图重绘空心吸管图标，使用圆润胶头、圆角护圈、细长管身和渐窄尖端。新增可复用 ui/include/psui/icons.hpp，统一跟随主题文字色与 DPI 缩放。按钮无边框、无背景，仅在悬停/按下时将图标变为蓝色。此次只修改按钮图形及显示版本，保持原有全屏取色、拖动操作与 PS 原生吸管鼠标指针。100%/200% 共用面板已检查。

整包0.36-rc1：本效果已接入完整智能滤镜参数保存，可随PSD保留并重编辑；使用方法与旧文件限制见[共用说明](../docs/PARAMETER_PERSISTENCE.md)。

0.20 共用吸管增强：色块旁改为 PS 风格矢量吸管图标，鼠标指针通过官方 UI Hooks 使用 PS 原生吸管。支持点击后选色，也支持按住图标拖至目标、松开取色。缩览图采样当前预览；画布、其它软件、桌面及界面区域统一读取屏幕显示 RGB，不调用宿主交互式 SamplePoint，完整支持全屏幕取色，无法读取时保持原色。支持负坐标多显示器，预览缩放/平移/裁切，Esc/右键取消以及鼠标捕获清理。版本：Tint 0.21 / Fractal Noise 0.15 / Displacement Map 0.8 / Glow 0.6 / Deep Glow 0.7 / Chromatic Dispersion 0.5 / Vector Blur 0.5 / Grid 0.4 / Checkerboard 0.4。

0.19 共用颜色控件：保留 PS 原生拾色器，在所有颜色块旁新增“取色 / Pick”，可从缩览图的可见结果取色，也可请求 PS 从窗口外的画布采样；Esc/右键/丢失鼠标捕获取消，无法取色的位置保留模式以便重试。统一处理缩放/平移/裁切、显示色彩管理之前的 RGB、目标隔离与鼠标捕获清理，不修改前景色、图层或预览视点。新效果复用 LibraryPanel::color_swatch / color_control 即可继承。详见 native/ui/COLOR-CONTROLS.md。版本：Tint 0.20 / Fractal Noise 0.14 / Displacement Map 0.7 / Glow 0.5 / Deep Glow 0.6 / Chromatic Dispersion 0.4 / Vector Blur 0.4 / Grid 0.3 / Checkerboard 0.3。

0.18 共用界面更新：九款插件的数值统一使用随主题调整的蓝色，数值区域支持左右拖动，Shift 为 10 倍速度、Ctrl 为 0.1 倍速度；轻点或双击进入输入，Enter 确认，Esc 取消本次编辑/拖动并保留滤镜窗口。普通参数、网格/棋盘的双坐标和预览缩放均复用同一组件，拖动速度按 DPI 校正；整数支持细调累积，禁用值不可编辑。后续效果使用 LibraryPanel::number / numeric_value / point_control 即可继承。版本：Tint 0.19 / Fractal Noise 0.13 / Displacement Map 0.6 / Glow 0.4 / Deep Glow 0.5 / Chromatic Dispersion 0.3 / Vector Blur 0.3 / Grid 0.2 / Checkerboard 0.2。

Grid 网格 / Checkerboard 棋盘 0.1
================================

两款独立 Windows x64 Photoshop `.8bf` 滤镜，不需要 After Effects。菜单位于 **滤镜 → ZaYu-AE 效果复刻**。

- `Grid 网格`：锚点、三种大小依据、边角、宽高、边界、独立横/纵羽化、反转、颜色、不透明度和混合模式。
- `Checkerboard 棋盘`：相同尺寸/定位/羽化控制，白色棋格与透明棋格交替；支持交换棋格。
- 增加旋转、锚点居中、常用预设和右键缩览图定位锚点。旋转以锚点为中心，默认 0°。
- 复用 PS 主题、中英文切换、DPI、关于、背景、缩放/平移及异步预览。颜色使用 Photoshop 自带拾色器，没有十六进制文本框。

使用
----

可以直接在新建的透明像素图层上生成，不需要先填白。“无（仅图案）”会用透明图案替换当前层的内容；需要叠加原图时选择“正常”等混合模式。“显示背景”只影响缩览图，显示当前图层与其下方可见层，不显示上方层。

大小依据支持边角点、宽度滑块（正方形）、独立宽高。默认网格使用文档中心到 60% 位置的边角点；棋盘使用宽度 16 像素。坐标是完整文档像素，不因选区或分块改变。

网格边界为 0 时不显示线条，反转后为实心图案。羽化向线条内部柔化；棋盘羽化在透明/实色棋格之间过渡。完全透明处不写入有效隐藏颜色。

支持 RGB 8/16/32 位、透明空层、选区与锁定透明度。锁定透明度时保留原 Alpha，并将图案的覆盖率编码进 RGB，仍保留不透明度和边缘抗锯齿。背景/平面图层没有透明通道时，透明区域以黑底输出；需要透明图案请用普通透明像素层。

混合菜单含 31 种独立标准实现，包括无、正常、相加、正片叠底、滤色、叠加、HSL、Alpha/亮度模板与轮廓等。溶解使用完整文档坐标的稳定散列，不随分块变化。部分旧版 AE 经典/溶解及色彩合成细节不保证一致。

校准与测试
----------

原参数语义参考 [Adobe 生成效果文档](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/generate-effects.html)。在本机 After Effects 25.6.5x3 中，通过正常脚本与 PNG 输出生成参考，没有使用原版二进制代码。

已对照 63 个同参数输出，涵盖默认值、三种尺寸模式、横纵羽化、0–6 像素边界、小数锚点、不透明度及常用混合。核心图案案例的 Alpha 平均误差，网格不超过约 0.21/255，棋盘不超过约 0.48/255，主要是 8 位量化取整；这些是固定案例的实测范围，不是任意参数的误差保证。AE `saveFrameToPng` 的颜色为预乘输出，校准时分别比较预乘 RGB 和 Alpha。

实际 PS 验证：两款各 RGB8/16/32，在空层生成完整画布的有效 Alpha；有内容的图层在“无”模式下产生相同图案；精确撤销、选区外保持、锁定 Alpha/抗锯齿及平面 RGB。实际 DLL 模拟宿主另检查不规则分块、取消/读错清理、现代/旧坐标和大文档局部分块。生产面板检查中文、英文、200% DPI、预设、滑块、反转、背景、缩放、关于及确认/取消。

证据位于主工程 `evidence/patterns/qa.json`、`ae-parity.json`、`ae-comparison.png`。参考生成脚本位于 `tests/patterns-ae-reference.jsx`；校准测试源码位于本模块 `tests`。

构建与复用
----------

先构建主工程的共享 UI 静态库，再运行 `patterns/build.ps1`。可选 `-CoreOnly` 或 `-BuildTarget Grid|Checkerboard|Both`。依赖 LLVM-MinGW/C++20、官方 Photoshop C++ SDK 与 Dear ImGui 1.91.9b；运行插件只需要 Windows/Photoshop。

两款共用同一算法和面板源文件，通过独立元数据和参数 magic 分开注册。新增可复用组件：

- `native/common/generator_filter_runtime.hpp`：每次仅生成 512×64 的输出块；需要混合时才读取同一区域原图。无需整幅文档快照，避免缓存型空间滤镜的整图像素限制。
- `native/common/pixel_blend.hpp`：独立 RGB/Alpha 合成及锁定 Alpha 的颜色处理。
- `native/ui/include/psui/generator_controls.hpp`：紧凑双坐标输入、PS 拾色器色块。
- `native/ui/include/psui/native_preview.hpp`：有界原生精度 RGB/Alpha/选区快照。
- `native/ui/include/psui/blend_labels.hpp`：可复用中英混合模式列表。

缩览图在低倍率使用有界采样，细网格可能有缩放近似；100% 可检查原生像素边缘。预览的背景、平移和缩放不写入滤镜参数。

by：淚泪泪。开源免费。项目代码 MIT，Dear ImGui 保留原始许可证。独立复刻，与 AE 原版不保证百分之百相同。
