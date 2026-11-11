0.34：共用关于窗口显示蓝色无边框“开源地址”，跳转 https://github.com/Three-LEI/AE-to-PS-Plugins 。重编译静态库调用方时需链接Windows shell32。项目可用根目录build.ps1和独立SDK路径完整构建；生成公式与参数ABI不变。

<!-- parameter-reset-0.27 -->
0.27：十二款示例接入参数单项重置；共享C++面板增加默认值绑定接口，需要重新编译library_ui.lib及其调用方。Photoshop宿主ABI、效果参数布局、组件标识和渲染内核不变。接入说明见../ui/PARAMETER-RESETS.md；发布归档中见PARAMETER-RESETS.md。
<!-- /parameter-reset-0.27 -->

<!-- lens-0.26 -->
0.26：加入光学补偿/Lens双效果示例，复用CachedRuntime、完整原生RGBA快照、纯CPU并行、异步预览及共用面板。未改公共库/宿主ABI或十款旧二进制。接入与限制见examples/lens/README.md、LENS-INTEGRATION.md。
<!-- /lens-0.26 -->

<!-- lens-0.25 -->
0.25：加入光学补偿/CC Lens双效果示例，复用CachedRuntime、完整原生RGBA快照、纯CPU并行、异步预览及共用面板。未改公共库/宿主ABI或十款旧二进制。接入与限制见examples/lens/README.md、LENS-INTEGRATION.md。
<!-- /lens-0.25 -->

<!-- parallel-0.24 -->
0.24：新增纯CPU头文件parallel_ranges.hpp，供分形杂色、Glow/Deep Glow及色散使用。最多8个工作范围，小任务串行；逐范围异常汇总、调用者线程取消、FP环境复制及同步退出。不更改公共UI库、宿主/参数ABI或既有图像算法。集成及验证见PARALLEL-INTEGRATION.md与validation/parallel。
<!-- /parallel-0.24 -->

0.23：Cell Pattern示例更新至0.2，加入每次渲染独立的有界特征/最近邻缓存，保留原始值与遍历顺序。新增与旧内核逐字节比较、性能及PS回归步骤；已有九款、公共UI静态库及宿主/保存参数ABI不变。详见examples/cell/README.md及CELL-INTEGRATION.md。

0.22：加入第十款Cell Pattern单元格图案0.1的接入示例、源图缓存与程序生成双路径、参考生成/比较及宿主验证步骤。复用现有运行器与UI，公共ABI和九款旧插件二进制均未改变。示例保留主工程相对路径，不是可独立构建的十款工程；入口见`examples/cell/README.md`及`CELL-INTEGRATION.md`。

0.21 图标微调：按用户参考图重绘空心吸管图标，使用圆润胶头、圆角护圈、细长管身和渐窄尖端。新增可复用 ui/include/psui/icons.hpp，统一跟随主题文字色与 DPI 缩放。按钮无边框、无背景，仅在悬停/按下时将图标变为蓝色。此次只修改按钮图形及显示版本，保持原有全屏取色、拖动操作与 PS 原生吸管鼠标指针。100%/200% 共用面板已检查。

0.20 共用吸管增强：色块旁改为 PS 风格矢量吸管图标，鼠标指针通过官方 UI Hooks 使用 PS 原生吸管。支持点击后选色，也支持按住图标拖至目标、松开取色。缩览图采样当前预览；画布、其它软件、桌面及界面区域统一读取屏幕显示 RGB，不调用宿主交互式 SamplePoint，完整支持全屏幕取色，无法读取时保持原色。支持负坐标多显示器，预览缩放/平移/裁切，Esc/右键取消以及鼠标捕获清理。版本：Tint 0.21 / Fractal Noise 0.15 / Displacement Map 0.8 / Glow 0.6 / Deep Glow 0.7 / Chromatic Dispersion 0.5 / Vector Blur 0.5 / Grid 0.4 / Checkerboard 0.4。

0.19 共用颜色控件：保留 PS 原生拾色器，在所有颜色块旁新增“取色 / Pick”，可从缩览图的可见结果取色，也可请求 PS 从窗口外的画布采样；Esc/右键/丢失鼠标捕获取消，无法取色的位置保留模式以便重试。统一处理缩放/平移/裁切、显示色彩管理之前的 RGB、目标隔离与鼠标捕获清理，不修改前景色、图层或预览视点。新效果复用 LibraryPanel::color_swatch / color_control 即可继承。详见 native/ui/COLOR-CONTROLS.md。版本：Tint 0.20 / Fractal Noise 0.14 / Displacement Map 0.7 / Glow 0.5 / Deep Glow 0.6 / Chromatic Dispersion 0.4 / Vector Blur 0.4 / Grid 0.3 / Checkerboard 0.3。

0.18 共用界面更新：九款插件的数值统一使用随主题调整的蓝色，数值区域支持左右拖动，Shift 为 10 倍速度、Ctrl 为 0.1 倍速度；轻点或双击进入输入，Enter 确认，Esc 取消本次编辑/拖动并保留滤镜窗口。普通参数、网格/棋盘的双坐标和预览缩放均复用同一组件，拖动速度按 DPI 校正；整数支持细调累积，禁用值不可编辑。后续效果使用 LibraryPanel::number / numeric_value / point_control 即可继承。版本：Tint 0.19 / Fractal Noise 0.13 / Displacement Map 0.6 / Glow 0.4 / Deep Glow 0.5 / Chromatic Dispersion 0.3 / Vector Blur 0.3 / Grid 0.2 / Checkerboard 0.2。

0.17：新增 Grid 网格 / Checkerboard 棋盘 0.1，现共九款效果。支持三种尺寸依据、锚点/边角、独立羽化、颜色/透明度/混合、旋转和预设；可在透明空层直接生成。新增分块生成运行器、标准像素合成、双坐标/色块控件及原生预览快照，可供后续生成效果复用。实际 PS 已核对 8/16/32 位空层/非空层、选区、锁定 Alpha、平面图和撤销；63 组同参数 AE 参考见 evidence/patterns/ae-parity.json。既有七款二进制未改变。

0.16：Vector Blur 矢量模糊 0.2 通过同源 AE 25.6.5x3 校准。修正自然模式的单向坡度累积、后向差分/法线响应、映射柔化尺度、其它类型的权重与方向角度基准。曲线流向默认关闭，保留艺术化选项。0.1 参数保留数值并自动迁移；其余六款二进制与共享宿主 ABI 未改动。21组参考见 evidence/vector-v02/vector-ae-parity.json；PS/界面/旧参数迁移与安装校验见 evidence/vector-v02/vector-qa.json。公共组件包加入校准后的源码、迁移与同源参考生成步骤。

0.15：新增第七款 Vector Blur 矢量模糊 0.1。五种类型、八种通道、PS 图层矢量图、可选曲线流向、方向箭头、质量/混合/边缘/透明度及预设。复用缓存空间滤镜运行器、只读图层快照、异步预览、背景、缩放平移、主题/语言/DPI/关于等组件。当前七款统一使用“英文 中文”名称，ComponentName 与已有六款参数存储保持不变。版本：Tint 0.18 / Fractal Noise 0.12 / Displacement Map 0.5 / Glow 0.3 / Deep Glow 0.4 / Chromatic Dispersion 0.2 / Vector Blur 0.1。已有六款仅更新名称和显示版本；Tint 现有宿主支持范围为 RGB8，其它六款为 RGB8/16/32。验证和限制见 vector/README.md、evidence/vector-qa.json。

0.14 共用组件与第六款效果：Chromatic Dispersion 色差色散 0.1。两种模式（RGB 分离 / 连续光谱）、直线/径向/切向、逐通道变换、光谱采样/色相/柔化、中心衰减、四种边缘、透明度策略、高光和图层遮罩。新增 common/cached_filter_runtime.hpp，统一缓存型空间滤镜的原生 RGBA 快照、参数、命名输入、完整结果、分块输出、选区和取消清理；新增 host_read_preview_channel 供选区/预览蒙版共用。旧五款插件的安装文件与核心算法未改动。测试与参考见 chromatic/README.md 和 evidence/chromatic-host-qa.json。

Deep Glow 0.3：修正大半径发光偏弱。保留近处辉光的十个对数尺度，并统一光能伽马转换与 Alpha 合成；背景开关、参数布局及存储结构保留。透明文字/图形的计算范围已优化，并与不裁切的完整计算参考进行像素对照。原生 Glow 0.2 的输出不变。详见 glow/README.md、evidence/deep-glow-strength-qa.json 和 evidence/deep-glow-0.3-comparison.png。

0.12 共用组件：五款缩览图统一新增“显示背景”复选框。勾选显示当前效果图层及其下方可见图层，不显示上方图层；关闭恢复透明棋盘。仅改变预览，不修改图层、选区、参数或历史。新增 preview_background.hpp / preview_background.cpp；图层读取均为只读 Channel Ports，按调用索引而不是持久 ID 排序，缓存按预览原点、尺寸与采样步长更新。宿主可能返回 Action Manager 混合 ID，现统一映射为 SDK 混合模式。支持图层透明度、填充透明度、启用的像素/矢量蒙版与常用混合模式。调整图层、图层组及高级混合/剪贴设置不能完整还原；可识别的缺失路径标注“近似预览”，读不到安全数据时回退当前层棋盘并提示不可用，绝不使用含上方层的整文档 merged 数据替代。

版本：Tint 0.17 / Fractal Noise 0.11 / Displacement Map 0.4 / Glow 0.2 / Deep Glow 0.2。原算法和最终输出流程未改变。实机对照与五款生产窗口结果见 evidence/preview-background-qa.json。

0.11 公共组件：新增可复用的 async_preview.hpp CPU 后台预览任务。任务可取消、替换并等待退出，保留上一帧完整纹理；PS 回调、ImGui 和 GPU 上传仍在主线程执行。Glow 0.1 / Deep Glow 0.1 使用此组件，共用主题、中英切换、DPI、关于和缩放平移。Deep Glow 为独立多尺度视觉重建，不是第三方原版移植；详见 glow/README.md。

0.10 公共组件：预览框内支持滚轮缩放（每格 1.2 倍，锚定鼠标下的图像位置），框外滚动不缩放。Win32 WM_MOUSEWHEEL 按消息的屏幕坐标路由，避免快速移出时旧 ImGui 悬停状态误触发；不额外要求中键按下。保留拖动平移、双击居中和适合。

置换图修复单背景/平面图像首次调用报错：PS 可以只提供 targetCompositeChannels 而不提供 layersDescriptor，现将有效的当前目标作为采样项。源像素和读取接口均缺失时仍正常报错；不会伪造不可读的图层。正式默认构建关闭 PortProbe 诊断。证据见 evidence/wheel-flat-qa.json。

0.9 公共组件：三款预览统一支持放大后左键拖动平移、双击居中、适合按钮恢复居中与完整画面；预览区增加跟随主题/DPI 的边框。平移以屏幕像素换算文档坐标，限制在作用范围边缘；缩放/窗口尺寸改变时约束视点。PreviewView 只保存视图状态，不修改滤镜参数。image_view 改为接收可修改的 PreviewView，需重新编译调用方。ROI 更新期间保留上一幅完整纹理及对应显示变换，避免新旧视图混用。验证见 evidence/preview-pan-qa.json。

# AEPS 共用组件 0.8：稳定布局与文档图层采样

用于 Tint 0.14、Fractal Noise 0.8、Displacement Map 0.1。

窗口创建与最小拖动尺寸统一按客户区计算，再加上当前 DPI 的非客户区边框；窗口尚未附加状态的早期 WM_GETMINMAXINFO 不施加通用最小高度。Tint 不再以比最小限制还小的尺寸创建。根窗口禁止滚动，具体参数区自行滚动；Tint 用独立参数子区域和预留按钮区，移动窗口不触发原来的整页下移。`--qa-layout` 检查初始/最小尺寸、移动、缩小/恢复及各控件矩形稳定；DPI 回归也覆盖该版本。

新增 `host_layers.hpp / host_layers.cpp`：只读枚举当前文档的 RGB 图层（包括隐藏图层）并用 Channel Ports 读取原生 8/PS16/float32 像素，提供 Unicode 名称、调用期间索引、名称指纹、透明度和有界代理读取。端口和描述符只在当前滤镜调用期间借用，不能保存到参数。PS 的目标通道端口与图层列表端口可能不同，应直接从 targetCompositeChannels 读取源图。PS 27.7 实机的 sheetID 会随图层顺序改变，不能当成持久 ID；置换图的参数引用使用名称唯一性校验，重名/改名/缺失需重新选择。

置换图的取样缓存、内核与独立滤镜生命周期留在 displacement 工程。空间变换滤镜需要原始源图缓存、跨块取样和可写 alpha，不能直接照搬原本仅请求 RGB 同址输入/输出的颜色处理 Runtime。

---

# 0.7：关于、预览缩放与文档中心变换

用于 Tint 0.13、Fractal Noise 0.7，菜单保持“ZaYu-AE 效果复刻”。

共用“关于”位于参数窗口左上角，使用同一 ImGui 模态弹窗，居中于滤镜窗口，跟随主题、DPI 和当前中英文。显示效果名称、各自版本、“本插件为独立复刻，与 AE 原版效果并非百分之百相同。”、精确署名 `by：淚泪泪` 与蓝色“开源地址”链接。关闭按钮、右上角 X 或 Esc 返回原滤镜窗口；Esc 不传递成父窗口取消。`plugin_info.hpp` 提供元数据和共用文案；每个效果通过自己的 `*_info.hpp` 定义名称/版本，并作为 `run_library_panel` 的末尾参数传入。宿主旧有 About 选择器也使用同一元数据和声明，其展示也使用共享面板和可点击链接。

两款预览共用 `PreviewView` 与 `LibraryPanel::image_view / preview_zoom`。默认适合窗口；百分比表示实际屏幕像素与图像像素之比，100% 为 1:1，滑块采用对数行程，支持数字输入和“适合”按钮。缩放以完整滤镜作用范围的中心查看，放大时通过 `host_read_proxy_area` 重新读取中心区域，并携带真实原点与采样步长；不是只把旧缩略图拉大。选区/alpha 与颜色管理继续生效。预览状态只在对话框上下文保存，不写入滤镜参数，不改变最终效果的缩放参数。字体仍按 DPI 重新栅格化。

Fractal 的 Region 新增完整文档中心 center_x / center_y。宿主渲染和预览统一传入 imageSize/2，所有分块与裁切共用该中心。缩放/旋转的逆变换使用 W−C，偏移另作为噪声场平移；移动偏移后再缩放/旋转也不把基点移走。透视偏移同样在外部变换之前作用于噪声场。默认 100%/0° 输出保留已验证的旧版字节基线；已有非默认变换的结果可能随基点修正改变。

验证入口：`--qa-about-zh / --qa-about-en / --qa-about-200` 检查共用弹窗、作者字形、关闭、真实裁切缩放、“适合”及滤镜参数逐字节保持；既有语言、主题、DPI、透明源回归继续通过。核心测试检查所有 17 个模型（含透视偏移）在偏移后改变角度/缩放时中心采样不变、非统一缩放的逆映射、带中心的分块一致性；DLL 适配测试检查完整文档尺寸传递。新 Region 字段属于渲染上下文，没有改变存储参数结构或旧参数迁移格式。

接入新效果时，窗口调用为 `run_library_panel(..., glyphs, &plugin_info)`；预览先按视口配置 `PreviewView`，视图 dirty 时调用 `host_read_proxy_area`，然后按 ProxyImage 的 origin_x / origin_y / step 计算结果并显示。生成类效果仍需自行声明 0.6 的透明度协议。组件包 examples 提供两款窗口、参数/版本头文件及生成滤镜 PiPL；完整算法与构建入口保留在主工程。

0.7 发行包根目录的 `build-library-ui.ps1` 已改为独立构建入口，同时生成两份静态库，不依赖本机工程路径。传入 `-Toolchain <LLVM-MinGW的bin目录> -PhotoshopSdk <官方pluginsdk目录>`，可选 `-OutputDir <输出目录>`；ImGui 源码使用包内 `third_party/imgui`。包内 tests 保留主工程相对路径，作为集成回归参考；不是单独可运行的 Photoshop SDK 测试工程。

## 0.6：生成滤镜与透明图层

用于 Tint 0.12、Fractal Noise 0.6，统一菜单为“ZaYu-AE 效果复刻”。

生成滤镜使用 Photoshop SDK 的云彩类协议：在可编辑透明度的 FilterCaseInfo 第 4、5 项开启 WorksWithBlankData，输入使用 InWhiteZap，输出使用 OutFillMask。PS 将完全透明像素的无定义 RGB 作为白色提供，滤镜仍分块处理 RGB，PS 负责在作用范围内产生不透明输出并处理选区。保护透明度的第 6、7 项继续使用原协议。此策略由效果自身声明，不套用于 Tint 等颜色调整效果。

不提前修改文档或调用填充命令：预览只读，取消后没有白色填充或历史记录；确定后生成与透明度变更属于一次滤镜操作，一次撤销恢复原始透明图层。选区外保持原图；未锁定透明像素时，生成滤镜会在作用范围内创建不透明内容，与 PS 云彩的行为一致。锁定透明像素则保留原透明范围。

新增 `ui/include/psui/preview_composite.hpp`，用于统一合成原图、生成结果、透明棋盘格及选区覆盖率。生成型效果必须按输出 alpha 合成预览，不能继续沿用源 alpha，否则空图层的预览会错误地保持全透明。Fractal 使用此组件；原始算法的 RGBA alpha 复制行为保持独立，由 PS 的输出协议产生图层不透明度。

验证：PS 27.7 RGB8 全透明图层的预览、取消、确定、一次撤销；RGB16 与 float32 全透明生成；RGB8 选区外 alpha=0；锁定透明像素时原 alpha 范围不变。`fractal/tests/verify_host_blank.py` 检查实际导出 PNG/TIFF 的原始位深像素；`--qa-blank / --qa-blank-selection` 与 `verify_blank_preview.py` 检查生产对话框的 GPU 预览帧。嵌入资源测试检查生成协议只用于可编辑透明度的情形，Tint 不启用生成行为。详细记录见 `evidence/fractal-blank-qa.json`。

依据：本机官方 SDK `PIFilter.h` 中 filterDataHandlingWhiteZap、filterDataHandlingFillMask、PIFilterWorksWithBlankDataBit 的说明，以及 `pipl-schema.json` 的字段定义。

## 0.5：宿主语言与即时切换

本版用于 Tint 0.11 与 Fractal Noise 0.5。右上角统一提供“跟随 PS / Auto、中文、English”。默认读取 Photoshop SDK 的 `propLocale`（`locl`）属性，zh 系列显示简体中文，其他语言或宿主接口不可用时显示英文，仍可手动切换。语言名称保持双语，方便随时找回切换入口。

切换当帧更新参数名、分组、下拉选项、预览复选框和操作按钮。参数、折叠状态、滚动、预览图保持原值；两种语言预先装入同一字库，不重建字体或预览纹理。效果名称和滤镜菜单沿用原命名。

PS 27.7 实机返回的 locale 句柄为 UTF-16LE（zh_CN 含终止符共 12 字节），不能直接按零结尾的 UTF-8 字符串截取。共用解码器按句柄字节长度读取，兼容 UTF-16 与字节字符串，拒绝截断的双字节数据；显示层元数据继续使用 UTF-8。句柄通过宿主 Handle Suite 成对锁定、解锁并释放。

手动选择通过 Photoshop 主窗口上的 `ZaYu.AEPS.UILanguage.v1` 属性在本次 PS 会话中共享；重开插件、换到另一款复刻效果会沿用选择，退出 PS 后恢复默认跟随。没有写入注册表或偏好文件，也没有把语言混入效果参数。测试钩子不读写此会话偏好。

新效果仍提供 UTF-8 `中文 / English` 元数据：`number/choice/check/section/button` 自动选择显示语言，原始键与隐藏控件 ID 始终不变。直接绘制文字时使用 `ui.text("中文 / English")`，不要用翻译后的文字作为控件 ID。未含分隔符的文字原样显示。纯逻辑位于 `ui/include/psui/localization.hpp`，宿主语言的句柄读取与释放位于 `photoshop_host.cpp`。

验证：两款 `dialog_preview.exe --qa-language-zh / --qa-language-en / --qa-language-missing` 分别覆盖中文、英文、缺失宿主语言；依次切换 English→中文→Auto，检查非默认效果参数逐字节不变、折叠状态保留、下拉选项可打开、字体不重建、取消不保存。`native/tests/verify_language_frames.py` 比较 GPU 完整帧中的预览区域，要求各语言的预览像素完全相同。SDK 句柄测试覆盖不带终止符的字符串、锁定失败、无接口、过长数据与读取失败时的释放。

## 0.4：滑块与 DPI

本版用于 Tint 0.10 与 Fractal Noise 0.4。改进统一位于 LibraryPanel，后续效果不需要另写滑块或字体缩放逻辑。

- 滑块保留 ImGui SliderScalar 的鼠标、捕获、键盘导航与数值范围逻辑，隐藏原矩形外观，在同一 DrawList 中显示 2 逻辑像素轨道与直径 11 逻辑像素的圆形滑钮。交互热区仍为整行。滑块本身不进入文字编辑模式，左侧数值框保留输入。
- 字体基准由 13 调为 14 逻辑像素，使用 2× 横向采样和像素对齐。实际字号为 round(14 × DPI比例)，不通过 FontGlobalScale 放大旧纹理。
- DPI 改变时在帧边界释放并重建字体设备资源，按目标像素重新栅格化；布局从未缩放的基准样式重算，避免反复相乘引起的误差。预览纹理独立保留，仍整帧提交。
- 插件窗口创建时临时使用线程级 Per-Monitor V2 感知，结束后恢复调用方上下文。没有调用 SetProcessDpiAwareness 或修改 Windows / Photoshop 全局设置。窗口尺寸及最小尺寸考虑当前屏幕工作区。
- run_library_panel 的末尾可传 localized_text。应把该效果的全部参数名和下拉选项传入，用于在常用简体中文字形之外补充所需字符；Fractal 的源码已展示自动收集方法。字形范围持续保存在窗口上下文，不会引用临时数组。字体文件来自系统，不打包分发。

## 验证

metrics_tests.cpp 检查滑钮与轨道比例、热区、端点和字号；两款 --qa / --qa-warp 保留连续拖动、弹层、主题及预览回归。--qa-dpi 在自己的窗口中模拟 100%→125%→150%→200%→100%，检查字体像素、样式、重建次数与回到 100% 后字形图集的一致性。--qa-dpi-unaware 还模拟 DPI 不感知的宿主调用线程，检查自身窗口是 PMv2，退出后恢复宿主线程状态。

已在本机 PS 检查新版轨道显示、点击调节与预览；自动化对真实 Photoshop 的连续拖动未得出可靠结论，自有生产对话框输入测试通过。不同物理显示器的跨屏效果仍应随环境回归。测试没有改系统缩放设置。

## 接入资料

详细接口和构建说明沿用下文 0.3；新增 ui_metrics.hpp、metrics_tests.cpp 和 DPI 测试。组件包已包含更新源码、静态库与 MIT 许可。字体与缩放实现参考：

- https://github.com/ocornut/imgui/blob/v1.91.9b/docs/FONTS.md
- https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-improvements-for-desktop-applications

---

# AEPS 共用组件 0.3 — UI 库版

当前滤镜：Tint 色调 0.9、Fractal Noise 分形杂色 0.3。统一菜单“AE 效果复刻”。两款均使用 Dear ImGui v1.91.9b + 官方 Win32 / DX11 后端，静态编入 `.8bf`，最终用户不需要单独安装 UI 库或 AE。

## 绘制约定

所有参数控件、下拉弹层、折叠组、滚动条与预览图由一个 HWND 和一个交换链渲染。先计算完整帧，再 Present。预览计算期间继续显示上一帧；不向显示窗口直接清空或绘制 GDI 图像。PS 显示转换只在内存 DIB 中执行，随后上传纹理，与控件在同一帧显示。

旧版白闪原因已复现：33 个自绘控件中有 14 个按钮在 WM_ERASEBKGND 阶段改写背景；焦点变化又触发整页重排。旧绘制层也做了擦除保护和缓冲修复，但新效果应使用 UI 库，旧控件只作兼容与研究保留。

## 新效果接入

1. 内核继续是独立 C++ 像素算法。通过 `filter_runtime.hpp` 处理 PS 选择器、参数存储、分块、取消和进度。
2. 对话框调用 `psui::run_library_panel(module,owner,host,title,width,height,draw)`。在回调中用 `LibraryPanel::number / choice / check / section / button` 描述参数，不再添加原生子控件。
3. 用 `host_read_proxy` 获取目标图层的只读代理图。参数变化时合并更新；计算完成后 `LibraryPanel::preview` 上传完整纹理，用 `image` 显示。
4. 确定时 `close(true)`，取消时 `close(false)`。只有返回 IDOK 才把临时参数复制回宿主参数。不要在参数预览阶段写 PS 主文档。
5. 每个效果定义默认值、范围、参数启用关系和算法。中文英文使用 UTF-8 字符串；库从 Windows 加载微软雅黑字体，字体文件不打包分发。PS 的四档主题由共用宿主桥接读取。

实际示例：`fractal/src/library_dialog.cpp`、`native/src/tint_library_dialog.cpp`。窗口管理与绘制入口：`ui/include/psui/library_panel.hpp`、`ui/src/library_panel.cpp`。

## 构建与检查

`native/build-library-ui.ps1` 构建 `library_ui.lib`。完整构建依次运行 `native/build.ps1`、`fractal/build.ps1`。包含路径：`ui/include`、Dear ImGui 根目录及 backends；宿主桥接还需本机 Photoshop SDK。链接 `library_ui.lib`、`psui.lib`，及 Windows 系统库 d3d11 / dxgi / d3dcompiler / imm32 / dwmapi / gdi32 / user32。使用本项目 LLVM-MinGW / C++20；其他编译器应重新编译。

`ui/verify-contract.ps1` 是构建必经检查：效果窗口不得使用 WM_DRAWITEM、WM_ERASEBKGND、CreateWindow、GetDC、FillRect 或 InvalidateRect，必须通过共用 LibraryPanel。新效果加入后应将入口文件加入该检查列表。

两款 `build/dialog_preview.exe --qa` 驱动自己的 UI 库输入，验证中文字符、下拉选择、连续拖动、预览开关、滚动、深浅主题、重置、确定、再次打开和取消不保存参数。`--qa-warp` 强制走 Direct3D 软件渲染。截图从绘制完成后的 GPU 后缓冲读取，检查操作中的帧。测试不会通过这些接口操作 Photoshop 窗口。

真实 Photoshop 27.7 RGB8 检查：统一菜单、两款窗口、下拉选择、滑块拖动、预览、分形杂色应用、Tint 原生选色器和取消。16/32 位仍以算法和模拟宿主测试为主；不将模拟检查等同于全部实机范围。

## 依赖与边界

Dear ImGui 源码固定为官方 v1.91.9b：https://github.com/ocornut/imgui/tree/v1.91.9b 。MIT 许可随包包含，也嵌入每个 `.8bf` 的 IMGUI_LICENSE 资源。包含源码及许可，不含 Adobe SDK、AE 二进制或 Windows 字体。

设备创建优先硬件 Direct3D11，失败时尝试 WARP 软件渲染。当前中文/英文标签与键盘数值输入已接入，不提供完整 UI Automation 控件语义；复杂辅助功能需求需要单独评估。此结构消除了旧版原生按钮独立擦除导致的白闪路径，仍须对新效果、驱动和 PS 版本做回归，不承诺所有环境永远没有任何绘制问题。
