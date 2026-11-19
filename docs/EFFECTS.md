# 效果列表

0.39包含 **31款Windows x64原生滤镜**，分布在24个模块。整包版本以Release标签为准，内部组件保留各自编号；0.38及更早版本保留。

本版本新增 **[Caustics 焦散](CAUSTICS_LOCAL.md)**、**[Film Damage 胶片损伤](FILMDAMAGE_LOCAL.md)** 和 **[Long Shadow 长投影](LONGSHADOW_LOCAL.md)** 三款效果。

[全部参数窗口](GALLERY.md) · [安装](../README.md#安装与使用) · [验证范围](VALIDATION.md)

| 效果 | 位深 | 模块 | 展示 |
| --- | --- | --- | --- |
| Tint 色调 | RGB8 | [`native`](../native/) | [参数窗口](GALLERY.md#tint) |
| Fractal Noise 分形杂色 | RGB8/16/32 | [`fractal`](../fractal/) | [参数窗口](GALLERY.md#fractal-noise) |
| Displacement Map 置换图 | RGB8/16/32 | [`displacement`](../displacement/) | [参数窗口](GALLERY.md#displacement-map) |
| Glow 发光 | RGB8/16/32 | [`glow`](../glow/) | [参数窗口](GALLERY.md#glow) |
| Deep Glow 深度发光 | RGB8/16/32 | [`glow`](../glow/) | [参数窗口](GALLERY.md#deep-glow) |
| Chromatic Dispersion 色差色散 | RGB8/16/32 | [`chromatic`](../chromatic/) | [参数窗口](GALLERY.md#chromatic-dispersion) |
| Vector Blur 矢量模糊 | RGB8/16/32 | [`vector`](../vector/) | [参数窗口](GALLERY.md#vector-blur) |
| Grid 网格 | RGB8/16/32 | [`patterns`](../patterns/) | [参数窗口](GALLERY.md#grid) |
| Checkerboard 棋盘 | RGB8/16/32 | [`patterns`](../patterns/) | [参数窗口](GALLERY.md#checkerboard) |
| Cell Pattern 单元格图案 | RGB8/16/32 | [`cell`](../cell/) | [参数窗口](GALLERY.md#cell-pattern) |
| Optics Compensation 光学补偿 | RGB8/16/32 | [`lens`](../lens/) | [参数窗口](GALLERY.md#optics-compensation) |
| Lens 镜头 | RGB8/16/32 | [`lens`](../lens/) | [参数窗口](GALLERY.md#lens) |
| JPEG Damage JPEG 损坏 | RGB8/16/32 | [`jpeg`](../jpeg/) | [参数窗口](GALLERY.md#jpeg-damage) |
| Blobbylize 胶状化 | RGB8/16/32 | [`surface`](../surface/) | [参数窗口](GALLERY.md#blobbylize) |
| Glass 玻璃 | RGB8/16/32 | [`surface`](../surface/) | [参数窗口](GALLERY.md#glass) |
| Turbulent Displace 湍流置换 | RGB8/16/32 | [`creative`](../creative/) | [参数窗口](GALLERY.md#turbulent-displace) |
| Light Sweep 扫光 | RGB8/16/32 | [`creative`](../creative/) | [参数窗口](GALLERY.md#light-sweep) |
| Kaleida 万花筒 | RGB8/16/32 | [`creative`](../creative/) | [参数窗口](GALLERY.md#kaleida) |
| Roughen Edges 毛边 | RGB8/16/32 | [`roughen`](../roughen/) | [参数窗口](GALLERY.md#roughen-edges) |
| Starglow 星芒 | RGB8/16/32 | [`lightfx`](../lightfx/) | [参数窗口](GALLERY.md#starglow) |
| Shine 放射光束 | RGB8/16/32 | [`lightfx`](../lightfx/) | [参数窗口](GALLERY.md#shine) |
| Saber 能量光束 | RGB8/16/32 | [`energy`](../energy/) | [参数窗口](GALLERY.md#saber) |
| Venetian Blinds 百叶窗 | RGB8/16/32 | [`blinds`](../blinds/) | [参数窗口](GALLERY.md#venetian-blinds) |
| Halftone 网点印刷 | RGB8/16/32 | [`halftone`](../halftone/) | [参数窗口](GALLERY.md#halftone) |
| ASCII Dither 字符抖动 | RGB8/16/32 | [`ascii`](../ascii/) | [参数窗口](GALLERY.md#ascii-dither) |
| Dither 像素抖动 | RGB8/16/32 | [`dither`](../dither/) | [参数窗口](GALLERY.md#pixel-dither) |
| Lens Flare 镜头耀斑 | RGB8/16/32 | [`flare`](../flare/) | [参数窗口](GALLERY.md#lens-flare) |
| Etching 蚀刻 | RGB8/16/32 | [`etching`](../etching/) | [参数窗口](GALLERY.md#etching) |
| Caustics 焦散 | RGB8/16/32 | [`caustics`](../caustics/) | [参数窗口](GALLERY.md#caustics) |
| Film Damage 胶片损伤 | RGB8/16/32 | [`filmdamage`](../filmdamage/) | [参数窗口](GALLERY.md#film-damage) |
| Long Shadow 长投影 | RGB8/16/32 | [`longshadow`](../longshadow/) | [参数窗口](GALLERY.md#long-shadow) |

Lens 的界面名称为“Lens 镜头”；为兼容旧安装，文件名保留 `ZaYuCCLens.8bf`。Saber 支持两点、路径、Alpha 输入和 16 款自有参数预设；色差色散包含光谱扭曲与棱镜散焦模式。

模块 README 保留参数与开发近似范围；历史 evidence/dist 路径指本地研究记录。SDK、原版参考、个人 PSD 和安装备份不随源码上传。

0.39已收录全部31款，包括焦散、胶片损伤与长投影；三款均支持 RGB8／16／32、智能滤镜保存和公共快速预览。
