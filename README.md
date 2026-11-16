<p align="center"><img src="docs/assets/social-preview.jpg" alt="AE to PS — 23 native Photoshop filters" width="1280"></p>

# AE-to-PS-Plugins

**把 After Effects 常见效果带到 Photoshop：23 款免费、开源的 Windows 原生滤镜。**

[English](README.en.md) · 简体中文

![Release](https://img.shields.io/badge/release-0.37-blue) ![Platform](https://img.shields.io/badge/Windows-x64-0078D4) ![Code license](https://img.shields.io/badge/code-MIT-green)

发光、色散、能量光束、玻璃、程序纹理与画面变形，都能在 Photoshop 内完成。支持智能滤镜参数保存、中英文界面、蓝色单项重置，以及拖动时快速预览、停止后自动补全质量。

**[下载 Windows 插件包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.37/AE-to-PS-Plugins-0.37-Windows-x64.zip)** · [本次更新](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/tag/v0.37) · **[23 款参数窗口图集](docs/GALLERY.md)** · [完整清单](docs/EFFECTS.md)

## 参数窗口示例

下图为插件实际生产参数窗口，展示效果预览和当前调整数值。全部 23 款窗口、输入条件与参数见[完整图集](docs/GALLERY.md)。首页分享封面保持不变。

### Deep Glow 深度发光

![Deep Glow parameter window and effect preview](docs/assets/effects/deep-glow.png)

### Saber 能量光束

![Saber parameter window and effect preview](docs/assets/effects/saber.png)

**[查看全部 23 款参数窗口 →](docs/GALLERY.md)**

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
| 色彩与镜头 | Tint 色调、Chromatic Dispersion 色差色散、Optics Compensation 光学补偿、Lens 镜头 |
| 纹理与图案 | Fractal Noise 分形杂色、Cell Pattern 单元格图案、Grid 网格、Checkerboard 棋盘、JPEG Damage JPEG 损坏 |
| 变形与表面 | Displacement Map 置换图、Vector Blur 矢量模糊、Turbulent Displace 湍流置换、Blobbylize 胶状化、Glass 玻璃、Kaleida 万花筒 |
| 边缘与遮罩 | Roughen Edges 毛边、Venetian Blinds 百叶窗 |

色差色散包含光谱扭曲与棱镜散焦模式；Saber 支持两点、路径、Alpha 输入及 16 款自有预设。多数效果支持 RGB8／16／32，Tint 当前仅 RGB8。[参数与模块列表](docs/EFFECTS.md)

## 本次更新：0.37 正式版

- 相对 0.36-rc1 增加 Starglow、Shine、Saber 和百叶窗，整包达到 23 款。
- 平衡预览画质：按实际耗时调整草图分辨率，停止约 180ms 后恢复精细预览；改进透明采样与百叶窗遮罩预览。
- 公共线程复用、发光临时缓冲复用；Glow／Deep Glow 调整合成参数时可复用前置模糊结果。
- Deep Glow 高光校准、色差色散模式扩展、Saber 公共参数与预设，以及加高按钮和预览内浮动计算提示。

本机合成测试中，Deep Glow 768×512 的曝光调整约从 351ms 降至缓存命中后的 19.5ms。首次计算及改变半径仍需重新生成，收益随素材和机器变化。[测量与范围](docs/PERFORMANCE.md)

## 兼容与限制

| 环境 | 已知情况 |
| --- | --- |
| PS2020 21.2.9，本机 Windows 11 | 本次 23 款／67 个效果位深组合、撤销与旧 PSD 回放通过 |
| PS2026 27.7.0 | 已安装本次包；本轮未增加完整实机回归声明 |
| Win7＋CC2018 | 用户已确认此前 0.7 本地整包可运行；本版本保留兼容路径，仍需外机复测 |
| 其他 Windows／PS 版本 | 未覆盖所有组合，请反馈具体系统、PS 版本与位深 |

这是依据公开行为独立实现的效果集合，与原版不保证逐像素一致；随机纹理也不保证同一种子形态相同。效果生成使用 **CPU**，GPU 用于界面与预览显示。部分效果不能自动扩展 Photoshop 画布。

Glass／Blobbylize 的外部参考层修改后，需要重新进入效果确认更新。旧 PSD 中此前没有保存的参数无法追溯恢复。[完整验证范围](docs/VALIDATION.md) · [智能滤镜参数保存](docs/PARAMETER_PERSISTENCE.md)

## 开发与贡献

默认只在本机开发和测试；只有维护者明确要求更新到 GitHub 时才发布正式版。[开发与发布约定](docs/RELEASE_POLICY.md)


实现采用 C++20、LLVM-MinGW、Win32、Dear ImGui 与 D3D11，共 16 个模块。纯 CPU 核心测试无需 Photoshop 或 SDK：

```powershell
./build.ps1 -CoreOnly -Toolchain 'C:/Dev/llvm-mingw/bin'
```

完整构建需要开发者单独获取 Photoshop C++ SDK。[构建说明](docs/BUILDING.md) · [项目结构](docs/ARCHITECTURE.md) · [参与贡献](CONTRIBUTING.md)

欢迎[反馈问题和建议](https://github.com/Three-LEI/AE-to-PS-Plugins/issues/new/choose)，并提供有权公开的最小案例。如果这些插件对你的创作有帮助，欢迎 Star，方便以后找到更新。

## 许可

自有代码采用 [MIT](LICENSE)，第三方依赖见 [THIRD_PARTY.md](THIRD_PARTY.md)。[演示照片与含照片的展示图](docs/assets/README.md)不属于代码 MIT 授权范围。仓库不提供 Adobe SDK、原版效果二进制或 Windows 字体。

本项目与 Adobe、原版效果及相关厂商无隶属或背书关系。

by：淚泪泪 / Three-LEI
