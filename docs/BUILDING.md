# 构建

目标为 Windows x64 Photoshop 原生 `.8bf`，实现语言 C++20。完整构建会运行各模块的核心与宿主适配检查，但不会安装插件或修改 Photoshop。

## 依赖

- PowerShell 7（也可在支持 UTF-8 源码和代码页936的 Windows PowerShell 环境运行）。
- LLVM-MinGW x86_64 工具链，`bin` 包含 `x86_64-w64-mingw32-clang++.exe`、`x86_64-w64-mingw32-windres.exe` 和 `llvm-ar.exe`。开发使用 MSVCRT 版；其它工具链组合未经验证。
- [Adobe Photoshop C++ SDK](https://github.com/AdobeDocs/photoshop-cpp-sdk)，由开发者单独获取；指向其中的 `pluginsdk` 目录。SDK 不包含在本仓库和 MIT 授权范围内，使用时遵守其自身许可。
- Dear ImGui 1.91.9b，所需源码及原 MIT 许可已固定在 `third_party/imgui`；指纹记录在 `dependencies.lock.json`。不需要下载 Windows 字体或原版 AE 效果。
- Python 3 用于发布审查、改仓库地址和源码导出；插件构建和运行不依赖 Python。

## 完整构建

在项目根目录运行，路径替换为自己的依赖位置：

```powershell
./build.ps1 -Toolchain 'C:/Dev/llvm-mingw/bin' -PhotoshopSdk 'C:/Dev/photoshop-cpp-sdk/pluginsdk'
```

依次构建十三个模块，输出十九款 `.8bf` 到各模块的 `build` 目录。`native` 首先建立两份共享静态库，其余模块随后链接。构建失败会停止；具体模块日志和测试结果保存在相应 `build` 目录。

也可设置环境变量 `AEPS_TOOLCHAIN`、`AEPS_PHOTOSHOP_SDK` 和可选的 `AEPS_IMGUI_ROOT`。本机路径还可写入未跟踪的 `build.local.json`：

```json
{
  "toolchain": "C:/Dev/llvm-mingw/bin",
  "photoshop_sdk": "C:/Dev/photoshop-cpp-sdk/pluginsdk"
}
```

优先顺序：构建参数（本次环境）→环境变量→本机配置→工具链 PATH／仓库内默认依赖位置。`build.local.json` 不应提交。

单独构建某模块前，先建立共享库：

```powershell
./build.ps1 -Module native
./build.ps1 -Module roughen
```

仅测试纯 CPU 核心，不需要 Photoshop SDK 或 Photoshop：

```powershell
./build.ps1 -CoreOnly -Toolchain 'C:/Dev/llvm-mingw/bin'
```

核心测试仍使用已固定的 ImGui 头文件路径，但不链接 Photoshop。全部核心测试完成不代表真实宿主行为已经验证。

## 更换开源链接

派生仓库可运行：

```powershell
python scripts/set-repository.py https://github.com/your-account/your-repository
```

这会同步 `project.json` 与编译期头文件 `source_repository.hpp`。重新构建所有插件后，“开源地址”才会指向新位置。不会改效果参数或生成公式。

## 安装与发布

关闭 Photoshop，备份同名插件，再把所需 `.8bf` 放入 Photoshop 的 `Plug-ins` 目录。不要把 SDK、工具链、`.lib` 或测试程序放入插件目录。

`scripts/export-source.py` 使用明确文件范围生成干净源码目录和 ZIP；`scripts/check-source.py` 检查链接一致性、固定依赖、十九款清单及不应上传的文件。发布程序包放在 GitHub Releases，生成物不提交到源码分支。

`qa/verify-release.py`、`qa/install-release.ps1`及`qa/package-release.py`用于维护者的0.34升级验证，需要本地升级前的baseline和PS导出记录；这些历史输入不随源码提供。普通构建不调用这些迁移脚本。`qa/run-about-link.ps1`可在建立共享库后独立验证关于链接，`qa/run-photoshop.ps1`需要已安装插件及可用的Photoshop。

维护者发布下载包时，`scripts/github-release.py`使用Git已配置的GitHub登录会话，验证账号、目标仓库及上传资产哈希后发布；请仅对准备公开的`dist`文件显式调用该脚本。
