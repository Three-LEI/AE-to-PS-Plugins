# AE-to-PS-Plugins

将 After Effects 常用效果独立复刻为 Photoshop 原生滤镜。当前提供 **十九款 Windows x64 插件**，共用参数界面、蓝色单项重置、预览和中英文支持。

[下载发行包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases) · [效果列表](docs/EFFECTS.md) · [构建说明](docs/BUILDING.md) · [验证与限制](docs/VALIDATION.md)

## 使用

1. 从 Releases 下载插件整包并解压。
2. 退出 Photoshop，备份已有同名插件，将所需 `.8bf` 放入 Photoshop 的 `Plug-ins` 目录。
3. 启动 Photoshop，在 **滤镜 → ZaYu-AE 效果复刻** 中选择效果。

普通使用无需 AE、原版效果插件、SDK 或 Python。当前兼容修复候选版为 **0.35-rc2**，使用传统 PiPL 注册，并移除强制的 Win10 DPI／D3DCompiler 依赖。已验证环境为 Windows x64、Photoshop 2026（27.7.0）；PS2020（21.2.9）已在本机验证加载和使用；Windows 7＋PS2018及其它机器组合仍需实机复测。

多数效果支持 RGB8／16／32，Tint 当前仅 RGB8。十九款包含色调、分形杂色、置换图、发光／深度发光、色差色散、矢量模糊、网格／棋盘、单元格图案、光学补偿、镜头、JPEG损坏、胶状化、玻璃、湍流置换、扫光、万花筒和毛边。参数与限制见[完整清单](docs/EFFECTS.md)及各模块 README。

参数前的蓝色箭头重置对应参数，底部“全部重置”恢复默认值。右上角语言可选择跟随PS、中文或English；PS2020的自动模式通过界面首选项读取语言，手动选择优先。“关于”中的蓝色**开源地址**可在默认浏览器打开本仓库。

## 开发

项目以 C++20、LLVM-MinGW、Win32 和 D3D11 构建。Dear ImGui 1.91.9b 已附源码及许可；Photoshop C++ SDK 需要开发者另行获取。

```powershell
./build.ps1 -Toolchain 'C:/Dev/llvm-mingw/bin' -PhotoshopSdk 'C:/Dev/photoshop-cpp-sdk/pluginsdk'
```

只运行纯 CPU 核心测试时可加 `-CoreOnly`，无需 SDK 或 Photoshop。完整构建依次建立共享库及十三个模块，运行各自必要检查，不会安装插件或修改宿主。详见 [BUILDING.md](docs/BUILDING.md) 和 [ARCHITECTURE.md](docs/ARCHITECTURE.md)。

## 兼容与近似

这是独立实现，与原版效果不保证逐像素一致，随机纹理的同一种子也不保证形态相同。部分效果使用固定源图范围，不能像 AE 自动扩展图层或画布。完整源图尺寸上限和智能滤镜保存能力以各模块说明为准。

Glass／Blobbylize 的外部参考层修改后需要重新打开效果确认更新，未附带自动刷新脚本。建议在透明文字或图形的智能对象上进行小图验证，再处理正式作品。

0.35-rc2 修复加载兼容及PS2020自动语言识别问题，保留0.34的效果参数、组件身份和生成内核。请在关闭 Photoshop 后用候选包替换同名插件，避免不同目录中留有重复副本；Windows 和 Photoshop 都需要64位。验证记录见 [VALIDATION.md](docs/VALIDATION.md)。

## 许可与贡献

项目自有代码采用 [MIT](LICENSE)。第三方许可和外部依赖见 [THIRD_PARTY.md](THIRD_PARTY.md)。仓库不包含 Adobe SDK、原版效果二进制、Windows字体、个人PSD或本地研究历史截图。

欢迎提交最小复现和改进，见 [CONTRIBUTING.md](CONTRIBUTING.md)。本项目与 Adobe 或相关原版效果厂商无隶属或背书关系。

by：淚泪泪
