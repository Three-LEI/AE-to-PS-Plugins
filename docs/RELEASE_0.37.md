# 0.37 · 23 款滤镜、性能优化与效果图集

> 历史正式版。当前正式版见[0.38](RELEASE_0.38.md)，0.37继续保留供回溯。

**安装请选择 [Windows-x64.zip 插件包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.37/AE-to-PS-Plugins-0.37-Windows-x64.zip)**。源码 ZIP 面向开发者。0.37 为新的正式发布起点；此前 Releases 已清理并在本地备份，Git 提交历史保留。

## 新增与改进

- 在 0.36-rc1 的 19 款基础上新增 **Starglow 星芒、Shine 放射光束、Saber 能量光束、Venetian Blinds 百叶窗**，共 23 款。
- Saber 支持两点、路径、Alpha 输入和公共参数，提供 16 款自有预设；色差色散增加光谱扭曲与棱镜散焦模式，Deep Glow 完成高光表现校准。
- 所有效果使用统一的交互快速预览。草图从 512 起步，按耗时调整分辨率，停止后自动补全质量；改善透明采样和百叶窗预览细节。
- 公共 CPU 线程复用、模糊缓冲复用；Glow／Deep Glow 的合成参数调整可复用前置缓存。768×512 合成测试的曝光调整约从 351ms 降至缓存命中后的 19.5ms；首次计算、改变半径仍需重算，收益因素材和机器而异。
- 操作按钮加高，Fit 对齐，计算提示浮动在预览内，减少布局抖动；保持蓝色单项重置、中英文和智能滤镜参数保存。
- 提供[全部 23 款参数窗口图集](https://github.com/Three-LEI/AE-to-PS-Plugins/blob/main/docs/GALLERY.md)，可同时查看精细效果预览和示例参数；另有英文首页、复现参数、构建和性能说明。

## 验证范围

本机 PS2020 **21.2.9**：23 款共 67 个效果／位深组合及撤销通过，137 次 TIFF 样本字节比较与性能优化前候选一致；旧 PSD 的 194 个独立滤镜实例恢复及 194 次重开／强制重算比较通过。16 模块完整构建，36 组完整浮点输出、396 组缓存对照、46 组界面检查通过。

保留 x64、OS／Subsystem 6.0、传统 PiPL／aete、旧 DPI 回退及预编译界面着色器；相对性能优化前候选没有新增强制导入 API。

**兼容范围**：用户确认此前 0.7 本地整包在 Win7＋CC2018 可运行，本版本保留兼容路径，仍需外机复测。PS2026 本轮仅安装，未新增完整实机回归声明。效果是独立近似实现，不保证与原版逐像素相同。效果生成使用 CPU。

## 安装

退出 Photoshop，备份同名插件，把 Windows 插件包内 `Plug-ins` 中所需 `.8bf` 放入 Photoshop 的 `Plug-ins` 目录，再启动。在“滤镜 → ZaYu-AE 效果复刻”中使用。需要可编辑参数时先转智能对象。避免多个插件目录保留重复副本。Windows 和 Photoshop 均需 64 位，普通使用无需 AE、原版插件、SDK 或 Python。

外部 Glass／Blobbylize 参考层修改后仍需手动重开效果确认更新。演示照片不属于代码 MIT 授权范围，见仓库素材说明。

## English

23 native Windows x64 Photoshop filters, with four additions since 0.36-rc1: Starglow, Shine, Saber and Venetian Blinds. Includes adaptive previews, CPU thread reuse, Glow/Deep Glow preparation caches, chromatic mode extensions, 16 Saber presets and a gallery of actual production parameter windows with effect previews.

Download **Windows-x64.zip** for installation. End users do not need AE or the SDK. This release was tested in PS2020 21.2.9; PS2026 is installed-only for this round, and Win7/CC2018 still requires external retesting. Earlier Releases were archived locally and removed as a one-time cleanup. See the repository for precise validation and compatibility limits.

## 后续更新约定

默认仅本机开发和测试，收到维护者明确的“更新到 GitHub”指令后才同步源码和发布当次正式版，不再自动发布预发布版本。详见 [发布约定](RELEASE_POLICY.md)。

## 介绍与图集更新

图集已改为 23 款生产参数窗口截图，包含精细效果预览和调整数值；首图保持不变。色调与分形杂色的截图取图适配已修正。本次只更新说明、图片及展示工具，0.37 的插件程序与三个 Release 附件均保持。
