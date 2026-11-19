# 共享验证

模块 `tests` 中的核心与宿主适配检查由根目录 `build.ps1` 运行。这里保存可移植的共享参数、预览、界面检查，以及模块测试所需的自有参数夹具。

- `run-cache-tests.ps1`：Glow／Deep Glow 缓存与直接计算对照、参数依赖、取消、浮点环境和内存上限；无需 Photoshop 或 SDK。
- `run-parameter-store.ps1`：共享参数字段、描述符和失败清理检查；需要 SDK 头文件。
- `run-preview-quality-tests.ps1`、`run-preview-quality-shared.ps1`：平衡预览分辨率、透明采样、坐标和停止后补全。
- `run-adaptive-panels.ps1`：生产界面的连续拖动、重置、取消和布局检查；会打开测试窗口。

本地研究中依赖升级前二进制、原版 AE 参考、历史 PSD 或开发机安装路径的批处理不随仓库分发。历史文档中的 `evidence` 路径指开发记录，不是构建依赖。本次公开验证摘要见 [VALIDATION.md](../docs/VALIDATION.md)。

## 图层越界读取

完整构建后运行 `./qa/run-offcanvas-adapters.ps1`，直接加载全部实际DLL，验证画布与图层边界不同时的RGB8／16／32、四向移动、选区与旧／新坐标；Tint仅RGB8，Cell补测晶格化及锁定Alpha。模拟真实PS的FilterRecord重定位与Channel Ports文档坐标，包含选区重置floatCoord的情况；Optics Compensation零FOV逐像素核对完整恒等输出，防止只通过“不报错”而遗漏透明截断或位移。只使用模拟宿主，不打开Photoshop或个人文档。
