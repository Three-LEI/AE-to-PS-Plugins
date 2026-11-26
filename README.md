<p align="center"><img src="docs/assets/social-preview.jpg" alt="AE to PS · 41 款 Photoshop 原生滤镜与七种实渲示例" width="1280"></p>

# AE-to-PS-Plugins

**41 款免费开源的 Photoshop 原生滤镜，灵感来自 After Effects。支持 Windows x64。**

[English](README.en.md)

**[下载 0.41 安装包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.41/AE-to-PS-Plugins-0.41-Windows-x64.zip)** · [效果清单](docs/EFFECTS.md) · [参数窗口图集](docs/GALLERY.md)

## 0.41 更新

- 41 款滤镜的右侧参数统一分组和对齐。窄窗中的长标签会自动换行。
- 复选框直接切换效果开关。「全部重置」可恢复整套效果参数。

[发行说明](docs/RELEASE_0.41.md) · [预览操作](docs/ADAPTIVE_PREVIEW.md)

## 安装与使用

1. 下载 Windows x64 安装包，退出 Photoshop 并备份同名插件。
2. 将包内所需的 `.8bf` 复制到 Photoshop 的 `Plug-ins` 目录，然后重启。
3. 在「滤镜」菜单的 `ZaYu-` 分组中打开。需要再次编辑参数时，先把图层转为智能对象。

Windows 与 Photoshop 均需为 64 位。已在 PS2020 和 PS2026 验证，Win7／CC2018 待实机复测。[验证范围](docs/VALIDATION.md)

## 开发与许可

使用 C++20、LLVM-MinGW、Win32、Dear ImGui 和 D3D11。[构建指南](docs/BUILDING.md) · [贡献说明](CONTRIBUTING.md) · [反馈问题](https://github.com/Three-LEI/AE-to-PS-Plugins/issues/new/choose)

代码采用 [MIT](LICENSE)。演示照片与含照片的截图遵循[素材许可](docs/assets/README.md)。[第三方声明](THIRD_PARTY.md)

by：淚泪泪 / Three-LEI
