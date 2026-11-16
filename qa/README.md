# 共享验证

模块 `tests` 中的核心与宿主适配检查由根目录 `build.ps1` 运行。这里保存可移植的共享参数、预览、界面检查，以及模块测试所需的自有参数夹具。

- `run-cache-tests.ps1`：Glow／Deep Glow 缓存与直接计算对照、参数依赖、取消、浮点环境和内存上限；无需 Photoshop 或 SDK。
- `run-parameter-store.ps1`：共享参数字段、描述符和失败清理检查；需要 SDK 头文件。
- `run-preview-quality-tests.ps1`、`run-preview-quality-shared.ps1`：平衡预览分辨率、透明采样、坐标和停止后补全。
- `run-adaptive-panels.ps1`：生产界面的连续拖动、重置、取消和布局检查；会打开测试窗口。

本地研究中依赖升级前二进制、原版 AE 参考、历史 PSD 或开发机安装路径的批处理不随仓库分发。历史文档中的 `evidence` 路径指开发记录，不是构建依赖。本次公开验证摘要见 [VALIDATION.md](../docs/VALIDATION.md)。
