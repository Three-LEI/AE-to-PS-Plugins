# 开发与发布约定

从 **0.37** 开始，GitHub 上发布正式版本，不再直接上传预发布版本。

1. 默认在本机开发、测试和安装，不自动提交或同步到 GitHub。
2. 只有维护者明确要求“更新到 GitHub”或“发布”时，才整理当次源码、检查和安装包。
3. 使用当次正式版本号发布，设置为 **Latest**。测试范围和已知限制按实际结果书写；正式版不表示所有旧系统均经过新一轮实测。
4. 源码检查 CI 只验证源码、参数字段和图集，不执行发布。发布工具默认仅检查；必须显式传入 `--publish` 才会上传并发布。
5. 本次将 Releases 整理为只保留 0.37，旧发布记录与附件已在维护者本地备份。Git 提交历史保留；后续正常保留正式版历史，清理旧发布需要再次明确要求。

一次公开更新授权不延续到后续本地开发。本地调整完成后，应先交给维护者试用，等待下一次明确的同步指令。

Development stays local by default. Only an explicit maintainer request authorizes a GitHub update. Each authorized publication uses the current stable version and is marked Latest; no automatic prereleases. The validation CI never publishes. Compatibility claims remain limited to actual testing.
