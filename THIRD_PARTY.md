# 第三方组件与依赖

本项目自有源码按根目录 [MIT LICENSE](LICENSE) 发布。第三方代码保留各自版权和许可。

| 项目 | 使用方式 | 许可／位置 |
| --- | --- | --- |
| Dear ImGui 1.91.9b | 共用面板、Win32/D3D11 后端 | MIT，见 `third_party/imgui/LICENSE.txt` |
| ImGui DragScalar 的修改实现 | `native/ui/src/numeric_scrub.cpp` 保留作者及来源注释 | MIT，同上 |
| Adobe Photoshop C++ SDK | 开发者另行获取，仅用于构建原生宿主接口 | SDK 自身许可；仓库不附带 SDK |
| LLVM-MinGW | 开发者另行安装的构建工具 | 工具链自身许可；仓库不附带工具链 |
| Windows 系统字体及系统库 | 从运行系统加载 | 未作为源码资源或发行资源分发字体文件 |

不包含 Adobe、Boris FX、Video Copilot 等原版效果实现或二进制。效果是按公开参数和可观察行为独立近似重建，不承诺与原版逐像素一致。产品及效果名称用于说明兼容目标，与相关厂商无隶属或背书关系。

本地研究目录中的原版参考截图、AE 工程、PSD、SDK 下载、工具链和历史安装备份不进入源码导出范围。
