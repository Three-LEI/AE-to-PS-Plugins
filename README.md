<p align="center"><img src="docs/assets/social-preview.jpg" alt="AE to PS — native Photoshop filters" width="1280"></p>

# AE-to-PS-Plugins

**把 After Effects 常见效果带到 Photoshop：28 款免费、开源的 Windows 原生滤镜。**

[English](README.en.md) · 简体中文

![Release](https://img.shields.io/badge/release-0.38-blue) ![Platform](https://img.shields.io/badge/Windows-x64-0078D4) ![Code license](https://img.shields.io/badge/code-MIT-green)

发光、色散、能量光束、玻璃、程序纹理与画面变形，都能在 Photoshop 内完成。支持智能滤镜参数保存、中英文界面、蓝色单项重置，以及拖动时快速预览、停止后自动补全质量。

**0.38 正式版**新增网点印刷、ASCII 字符抖动、像素抖动、镜头耀斑设计器和蚀刻，支持照片印刷、货币雕版、像素艺术与可编辑镜头光效。收录星芒／Shine 性能改进；保留历史正式版供回溯。

**[下载 Windows 插件包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.38/AE-to-PS-Plugins-0.38-Windows-x64.zip)** · [本次更新](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/tag/v0.38) · **[28 款参数窗口图集](docs/GALLERY.md)** · [完整清单](docs/EFFECTS.md)

## 参数窗口示例

### Etching 蚀刻 · 绿钞雕版

![Etching engraving effect and parameter window](docs/assets/effects/etching.png)

### Lens Flare · 耀斑设计器

![Lens Flare designer and parameter window](docs/assets/effects/lens-flare.png)

下图为插件实际生产参数窗口，展示效果预览和当前调整数值。全部 28 款窗口、输入条件与参数见[完整图集](docs/GALLERY.md)。首页分享封面保持不变。

### Deep Glow 深度发光

![Deep Glow parameter window and effect preview](docs/assets/effects/deep-glow.png)

### Saber 能量光束

![Saber parameter window and effect preview](docs/assets/effects/saber.png)

**[查看全部 28 款参数窗口 →](docs/GALLERY.md)**

## 安装与使用

1. 下载上方 **Windows-x64.zip 插件包**。“Code → Download ZIP” 是源码，需要自行编译。
2. 退出 Photoshop，备份旧版同名插件，将包内 `Plug-ins` 中所需的 `.8bf` 放入 Photoshop 的 `Plug-ins` 文件夹。避免在多个目录保留重复副本。
3. 启动 Photoshop，在 **滤镜 → ZaYu-AE 效果复刻** 中使用。
4. 需要反复修改时，先把目标层转换为**智能对象**，再添加效果并保存 PSD；重新打开后可继续编辑各实例参数。

普通使用无需 AE、原版效果插件、SDK 或 Python。Windows 与 Photoshop 均需为 64 位。

## 包含哪些效果

| 类别 | 效果 |
| --- | --- |
| 发光与光线 | Glow 发光、Deep Glow 深度发光、Starglow 星芒、Shine 放射光束、Saber 能量光束、Light Sweep 扫光 |
| 色彩与镜头 | Tint 色调、Chromatic Dispersion 色差色散、Optics Compensation 光学补偿、Lens 镜头、Lens Flare 镜头耀斑／设计器 |
| 印刷与风格化 | Halftone 网点印刷、ASCII Dither 字符抖动、Dither 像素抖动、Etching 蚀刻 |
| 纹理与图案 | Fractal Noise 分形杂色、Cell Pattern 单元格图案、Grid 网格、Checkerboard 棋盘、JPEG Damage JPEG 损坏 |
| 变形与表面 | Displacement Map 置换图、Vector Blur 矢量模糊、Turbulent Displace 湍流置换、Blobbylize 胶状化、Glass 玻璃、Kaleida 万花筒 |
| 边缘与遮罩 | Roughen Edges 毛边、Venetian Blinds 百叶窗 |

色差色散包含光谱扭曲与棱镜散焦模式；Saber 支持两点、路径、Alpha 输入及 16 款自有预设。多数效果支持 RGB8／16／32，Tint 当前仅 RGB8。[参数与模块列表](docs/EFFECTS.md)

## 本次更新：0.38 正式版

- **[Halftone 网点印刷](docs/HALFTONE_LOCAL.md)**：圆点、方点、菱形和线条，双色／透明输出。
- **[ASCII Dither 字符抖动](docs/ASCII_LOCAL.md)**：四种字符集、系统字体、颜色和抖动控制。
- **[Dither 像素抖动](docs/DITHER_LOCAL.md)**：规则阈值、Floyd–Steinberg／Atkinson及复古限色模式。
- **[Lens Flare 耀斑设计器](docs/FLARE_DESIGNER_LOCAL.md)**：十二类独立元素、可视添加、行内删除、拖动排序、快捷键、撤销／重做、十二套自有预设及文件导入／导出；640×400缓存预览支持悬停放大。
- **[Etching 蚀刻](docs/ETCHING_LOCAL.md)**：两组变粗细曲线、波形／明暗扭曲、轮廓、墨色／纸色／纸纹和六套版画预设。
- **Starglow／Shine性能改进**：旋转星芒并行计算，调整光效不透明度与混合时可复用光线缓存，保持生成效果。[测量与边界](docs/LIGHTFX_PERFORMANCE_LOCAL.md)

[完整0.38更新说明](docs/RELEASE_0.38.md)。程序安装请选择Windows-x64.zip；source.zip及“Code → Download ZIP”是开发源码。

## 兼容与限制

| 环境 | 已知情况 |
| --- | --- |
| PS2020 21.2.9，本机 Windows 11 | 新增五款的输出、撤销、PSD保存／新进程重开和重算；星芒／Shine像素对照通过，其余21款程序与0.37一致 |
| PS2026 27.7.0 | 已安装本次包；本轮未增加完整实机回归声明 |
| Win7＋CC2018 | 此前整包已有用户反馈可运行；0.38保留传统加载与旧DPI路径，仍需外机复测 |
| 其他 Windows／PS 版本 | 未覆盖所有组合，请反馈具体系统、PS 版本与位深 |

这是依据公开行为独立实现的效果集合，与原版不保证逐像素一致；随机纹理也不保证同一种子形态相同。效果生成使用 **CPU**，GPU 用于界面与预览显示。部分效果不能自动扩展 Photoshop 画布。

Glass／Blobbylize 的外部参考层修改后，需要重新进入效果确认更新。Lens Flare从本次正式版起使用版本2元素快照，早期未发布试验文件不承诺回读。此前没有保存的参数无法追溯恢复。[完整验证范围](docs/VALIDATION.md) · [智能滤镜参数保存](docs/PARAMETER_PERSISTENCE.md)

## 开发与贡献

默认只在本机开发和测试；只有维护者明确要求更新到 GitHub 时才发布正式版。[开发与发布约定](docs/RELEASE_POLICY.md)


实现采用 C++20、LLVM-MinGW、Win32、Dear ImGui 与 D3D11，共 21 个模块。纯 CPU 核心测试无需 Photoshop 或 SDK：

```powershell
./build.ps1 -CoreOnly -Toolchain 'C:/Dev/llvm-mingw/bin'
```

完整构建需要开发者单独获取 Photoshop C++ SDK。[构建说明](docs/BUILDING.md) · [项目结构](docs/ARCHITECTURE.md) · [参与贡献](CONTRIBUTING.md)

欢迎[反馈问题和建议](https://github.com/Three-LEI/AE-to-PS-Plugins/issues/new/choose)，并提供有权公开的最小案例。如果这些插件对你的创作有帮助，欢迎 Star，方便以后找到更新。

## 许可

自有代码采用 [MIT](LICENSE)，第三方依赖见 [THIRD_PARTY.md](THIRD_PARTY.md)。[演示照片与含照片的展示图](docs/assets/README.md)不属于代码 MIT 授权范围。仓库不提供 Adobe SDK、原版效果二进制或 Windows 字体。

本项目与 Adobe、原版效果及相关厂商无隶属或背书关系。

by：淚泪泪 / Three-LEI
