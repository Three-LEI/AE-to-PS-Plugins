# AE-to-PS-Plugins

将 After Effects 常用效果独立复刻为 Photoshop 原生滤镜。当前提供 **十九款 Windows x64 插件**，共用参数界面、蓝色单项重置、预览和中英文支持。

[下载0.36-rc1候选包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/tag/v0.36-rc1) · [历史版本](https://github.com/Three-LEI/AE-to-PS-Plugins/releases) · [效果列表](docs/EFFECTS.md) · [构建说明](docs/BUILDING.md) · [验证与限制](docs/VALIDATION.md)

## 使用

1. 从 Releases 下载插件整包并解压。
2. 退出 Photoshop，备份已有同名插件，将所需 `.8bf` 放入 Photoshop 的 `Plug-ins` 目录。
3. 启动 Photoshop，在 **滤镜 → ZaYu-AE 效果复刻** 中选择效果。

普通使用无需 AE、原版效果插件、SDK 或 Python。当前候选版为 **0.36-rc1**，保留0.35-rc2的传统PiPL注册、旧系统DPI回退及预编译界面着色器。用户已确认0.35-rc2在Win7＋PS2018（19.1.6）和Win10＋PS2020（21.2.3）可加载使用；本次新候选版仍需在这两台机器复测。

多数效果支持 RGB8／16／32，Tint 当前仅 RGB8。十九款包含色调、分形杂色、置换图、发光／深度发光、色差色散、矢量模糊、网格／棋盘、单元格图案、光学补偿、镜头、JPEG损坏、胶状化、玻璃、湍流置换、扫光、万花筒和毛边。参数与限制见[完整清单](docs/EFFECTS.md)及各模块 README。

参数前的蓝色箭头重置对应参数，底部“全部重置”恢复默认值。右上角语言可选择跟随PS、中文或English；PS2020的自动模式通过界面首选项读取语言，手动选择优先。“关于”中的蓝色**开源地址**可在默认浏览器打开本仓库。

0.36-rc1为十九款统一补齐智能滤镜完整参数保存：先把目标层转为智能对象，应用效果后保存PSD，退出PS再重开可继续编辑各实例参数。参考图层按Unicode唯一名称关联；旧文件中此前没有保存的参数无法追溯恢复。普通像素层直接应用滤镜后只有栅格结果，建议需要反复调整时使用智能对象。详见[参数保存说明](docs/PARAMETER_PERSISTENCE.md)。

## 开发

项目以 C++20、LLVM-MinGW、Win32 和 D3D11 构建。Dear ImGui 1.91.9b 已附源码及许可；Photoshop C++ SDK 需要开发者另行获取。

```powershell
./build.ps1 -Toolchain 'C:/Dev/llvm-mingw/bin' -PhotoshopSdk 'C:/Dev/photoshop-cpp-sdk/pluginsdk'
```

只运行纯 CPU 核心测试时可加 `-CoreOnly`，无需 SDK 或 Photoshop。完整构建依次建立共享库及十三个模块，运行各自必要检查，不会安装插件或修改宿主。详见 [BUILDING.md](docs/BUILDING.md) 和 [ARCHITECTURE.md](docs/ARCHITECTURE.md)。

## 兼容与近似

这是独立实现，与原版效果不保证逐像素一致，随机纹理的同一种子也不保证形态相同。部分效果使用固定源图范围，不能像 AE 自动扩展图层或画布。完整源图尺寸上限和智能滤镜保存能力以各模块说明为准。

Glass／Blobbylize 的外部参考层修改后需要重新打开效果确认更新，未附带自动刷新脚本。建议在透明文字或图形的智能对象上进行小图验证，再处理正式作品。

0.36-rc1按文档实际图层名补充字体字形，修复“鎏金”等少见汉字显示问号；单项重置改为蓝色无边框圆弧箭头。组件身份、参数结构和生成内核保持不变。请在关闭Photoshop后替换同名插件，避免多个目录留有重复副本；Windows和Photoshop都需要64位。验证记录见[VALIDATION.md](docs/VALIDATION.md)。

## 许可与贡献

项目自有代码采用 [MIT](LICENSE)。第三方许可和外部依赖见 [THIRD_PARTY.md](THIRD_PARTY.md)。仓库不包含 Adobe SDK、原版效果二进制、Windows字体、个人PSD或本地研究历史截图。

欢迎提交最小复现和改进，见 [CONTRIBUTING.md](CONTRIBUTING.md)。本项目与 Adobe 或相关原版效果厂商无隶属或背书关系。

by：淚泪泪
