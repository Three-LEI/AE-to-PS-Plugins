# 效果列表

当前包含十九款 Windows x64 原生滤镜。0.34更新共用关于链接与开源构建，效果版本、参数结构和生成内核保持0.33。

| 效果 | 内核版本 | 位深 | 模块 |
| --- | --- | --- | --- |
| Tint 色调 | 0.23 | RGB8 | [`native`](../native/) |
| Fractal Noise 分形杂色 | 0.18 | RGB8/16/32 | [`fractal`](../fractal/) |
| Displacement Map 置换图 | 0.10 | RGB8/16/32 | [`displacement`](../displacement/) |
| Glow 发光 | 0.9 | RGB8/16/32 | [`glow`](../glow/) |
| Deep Glow 深度发光 | 0.10 | RGB8/16/32 | [`glow`](../glow/) |
| Chromatic Dispersion 色差色散 | 0.8 | RGB8/16/32 | [`chromatic`](../chromatic/) |
| Vector Blur 矢量模糊 | 0.7 | RGB8/16/32 | [`vector`](../vector/) |
| Grid 网格 | 0.6 | RGB8/16/32 | [`patterns`](../patterns/) |
| Checkerboard 棋盘 | 0.6 | RGB8/16/32 | [`patterns`](../patterns/) |
| Cell Pattern 单元格图案 | 0.3 | RGB8/16/32 | [`cell`](../cell/) |
| Optics Compensation 光学补偿 | 0.2 | RGB8/16/32 | [`lens`](../lens/) |
| Lens 镜头 | 0.3 | RGB8/16/32 | [`lens`](../lens/) |
| JPEG Damage JPEG 损坏 | 0.1 | RGB8/16/32 | [`jpeg`](../jpeg/) |
| Blobbylize 胶状化 | 0.2 | RGB8/16/32 | [`surface`](../surface/) |
| Glass 玻璃 | 0.2 | RGB8/16/32 | [`surface`](../surface/) |
| Turbulent Displace 湍流置换 | 0.1 | RGB8/16/32 | [`creative`](../creative/) |
| Light Sweep 扫光 | 0.1 | RGB8/16/32 | [`creative`](../creative/) |
| Kaleida 万花筒 | 0.1 | RGB8/16/32 | [`creative`](../creative/) |
| Roughen Edges 毛边 | 0.1 | RGB8/16/32 | [`roughen`](../roughen/) |

Lens 对外显示为“Lens 镜头”；为兼容既有安装，二进制文件名保留 ZaYuCCLens.8bf。

单个模块的 README 保留开发阶段的参数与近似范围说明，其中历史 evidence/dist 路径指本地研究工作区，原始截图、PSD、AE工程不随源码仓库上传。当前验证概览见 [VALIDATION.md](VALIDATION.md)。
