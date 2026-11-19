# Caustics 焦散

仅本机开发的 0.2 版本，尚未收录公开 0.38。独立实现焦散光纹和可选水底折射，参考 Sapphire S_Caustics 与 AE 原生 Caustics 的公开功能，不包含商业插件代码、预设或资源。

使用及边界见 [本机说明](../docs/CAUSTICS_LOCAL.md)。纯 CPU 构建可运行根目录 `./build.ps1 -Module caustics -CoreOnly`；完整构建需先建立公共 native 库，详见 [构建说明](../docs/BUILDING.md)。
