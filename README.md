<p align="center"><img src="docs/assets/social-preview.jpg" alt="AE to PS · Photoshop 原生滤镜" width="1280"></p>

# AE-to-PS-Plugins

**41 款免费开源的 Photoshop 原生滤镜，灵感来自 After Effects。支持 Windows x64。**

[English](README.en.md)

**[下载 0.40 安装包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.40/AE-to-PS-Plugins-0.40-Windows-x64.zip)** · [效果清单](docs/EFFECTS.md) · [参数窗口图集](docs/GALLERY.md)

## 0.40 更新

- 新增综合杂色、球面与圆柱贴图、雨雪模拟、复合模糊等 10 款滤镜。
- 统一 360 × 360 默认预览区。有手柄的效果支持用左键直接调整位置和方向。
- 滤镜归入九个 `ZaYu-` 分组，支持中英文参数界面、快速预览和智能滤镜保存。

[发行说明](docs/RELEASE_0.40.md) · [预览操作](docs/ADAPTIVE_PREVIEW.md)

## 安装与使用

1. 下载 Windows x64 安装包，退出 Photoshop 并备份同名插件。
2. 将包内所需的 `.8bf` 复制到 Photoshop 的 `Plug-ins` 目录，然后重启。
3. 在「滤镜」菜单的 `ZaYu-` 分组中打开。需要再次编辑参数时，先把图层转为智能对象。

Windows 与 Photoshop 均需为 64 位。已在 PS2020 和 PS2026 验证，Win7／CC2018 待实机复测。[验证范围](docs/VALIDATION.md)

## 开发与许可

使用 C++20、LLVM-MinGW、Win32、Dear ImGui 和 D3D11。[构建指南](docs/BUILDING.md) · [贡献说明](CONTRIBUTING.md) · [反馈问题](https://github.com/Three-LEI/AE-to-PS-Plugins/issues/new/choose)

代码采用 [MIT](LICENSE)。演示照片与含照片的截图遵循[素材许可](docs/assets/README.md)。[第三方声明](THIRD_PARTY.md)

by：淚泪泪 / Three-LEI
