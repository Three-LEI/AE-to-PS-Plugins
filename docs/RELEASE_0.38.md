# 0.38 · 28款滤镜、耀斑设计器与蚀刻／抖动效果

**安装下载：[Windows-x64.zip 插件包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.38/AE-to-PS-Plugins-0.38-Windows-x64.zip)**。source.zip和“Code → Download ZIP”是开发源码，需要自行编译。

这是正式版，设为Latest；保留0.37历史版本。内部组件编号沿用各自开发编号，整包正式版本以本Release为准。

## 新增五款效果

- **Halftone 网点印刷**：圆点、方点、菱形和线条，双色／透明输出。
- **ASCII Dither 字符抖动**：四种字符集、系统字体、颜色和抖动控制。
- **Dither 像素抖动**：规则阈值、Floyd–Steinberg、Atkinson及复古限色。
- **Lens Flare 耀斑设计器**：十二类独立元素、可视添加、行内删除、拖动排序、快捷键、撤销／重做、自动中英文名称、十二套自有预设和导入／导出。640×400后台缓存预览支持悬停放大。
- **Etching 蚀刻**：双线、单线、暗部交叉排线，波形和亮度扭曲、轮廓、墨色／纸色／纸纹，六套货币雕版与版画预设。

## 性能与界面

星芒旋转光线并行计算，Starglow／Shine增加光线缓存，调整不透明度和混合时可复用结果。1024×768自有合成测试中，旋转17°星芒完整计算由273.8ms降至134.1ms；已提取高光后的不透明度调整，星芒129.5ms降至10.5ms，Shine42.8ms降至13.5ms。这些测试阶段不同，不能混为同一提速比例；收益随素材与机器变化。

新增效果均接入公共快速预览、精细补全、中英文、蓝色重置和智能滤镜保存。耀斑列表每行控件居中。[28款效果与参数窗口图集](https://github.com/Three-LEI/AE-to-PS-Plugins/blob/main/docs/GALLERY.md)已更新。

## 验证和边界

- 本机Windows11、PS2020 21.2.9：新增五款各18组预设／位深测试、36个智能滤镜实例，覆盖新进程重开、重算、撤销和源变化；合计241项像素核对，Lens Flare／Etching另有36项独立内核对照。耀斑最后一次行布局修改单独验证中英文／缩放和交互，内核及保存源码保持。
- 星芒／Shine通过266组缓存／直接／冻结内核对照、20组完整浮点、48项PS像素对照及12个旧智能实例参数检查。其余21个原有程序与0.37逐字节一致。
- 保留x64、OS/Subsystem6.0、传统PiPL/aete、旧DPI回退和预编译界面着色器。
- PS2026本轮仅安装。Win7／CC2018保留旧系统路径，**0.38仍需外机复测**。

CPU生成，GPU用于界面／预览显示。独立近似实现，不承诺与原版逐像素一致。ASCII字形受系统字体影响。Lens Flare正式版使用版本2元素快照，早期未发布试验文件不承诺回读。外部参考层改变后仍需手动进入效果刷新。

## 安装

退出Photoshop，备份同名插件，把包内`Plug-ins`中所需`.8bf`复制到64位Photoshop的`Plug-ins`目录后重启。需要可编辑参数时先转智能对象，避免多个目录保留重复副本。普通使用无需AE、原版插件、SDK或Python。

自有代码MIT；演示照片与含照片图集使用独立授权，不随程序包分发。源码包不含Adobe SDK、字体或个人PSD。

## English

Stable 0.38 includes 28 native Windows x64 Photoshop filters. Five additions: Halftone, ASCII Dither, Pixel Dither, Lens Flare Designer and Etching. Also includes appearance-preserving Starglow/Shine performance changes and an updated 28-effect gallery.

Download **Windows-x64.zip** for installation. Source archives require compilation. PS2020 21.2.9 validation covers the additions and changed effects; 21 existing binaries are unchanged from 0.37. PS2026 is installed-only for this round. Win7/CC2018 retains legacy paths and requires external retesting. Previous stable versions remain available.
