# 效果图集 / Effect gallery

23 款效果均由本项目安装在 **Photoshop 2020 21.2.9** 中的原生 `.8bf` 实际渲染。每张左侧为输入，右侧为输出；图片缩放和 JPEG 编码仅用于网页展示，棋盘只表示透明度。

These are actual outputs from this project's native Photoshop plugins, not original AE screenshots. Left: input. Right: output. Resizing, JPEG encoding and checkerboards are only used for presentation.

[返回首页](../README.md) · [下载插件包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.37-rc1/AE-to-PS-Plugins-0.37-rc1-Windows-x64.zip) · [完整参数数据](gallery-parameters.json) · [演示素材许可](assets/README.md)

## 输入与复现

演示照片由项目维护者提供，公开输入已缩小到 768×768 并移除非必要元数据：[demo-source.png](assets/demo-source.png)。不同效果使用不同参数；毛边使用椭圆 Alpha 输入，Starglow 输入添加少量高光点，置换图使用照片的隐藏副本，Saber 使用明确的椭圆路径。这些条件均在对应例子注明。

普通使用直接在插件界面调整即可。开发者完整构建并安装插件后，可使用仓库中的参数数据重新渲染：

```powershell
./scripts/render-gallery.ps1 -Source docs/assets/demo-source.png -Parameters docs/gallery-parameters.json -Output gallery-output
./scripts/package-gallery.ps1 -Renders gallery-output -Parameters docs/gallery-parameters.json -Assets gallery-assets
```

脚本默认连接 PS2020 21.2.9；其他版本需显式传入正确的 `-PhotoshopProgId` 和 `-ExpectedVersion`。只修改自身创建的文档。重新生成类型化参数数据时，先完成正常构建，再运行 `scripts/build-gallery-parameters.ps1`。

All examples use non-default demonstration settings. They describe this project's behavior and do not claim equivalence to proprietary originals.

## 目录 / Index

- [Tint 色调](#tint)
- [Fractal Noise 分形杂色](#fractal-noise)
- [Displacement Map 置换图](#displacement-map)
- [Glow 发光](#glow)
- [Deep Glow 深度发光](#deep-glow)
- [Chromatic Dispersion 色差色散](#chromatic-dispersion)
- [Vector Blur 矢量模糊](#vector-blur)
- [Grid 网格](#grid)
- [Checkerboard 棋盘](#checkerboard)
- [Cell Pattern 单元格图案](#cell-pattern)
- [Optics Compensation 光学补偿](#optics-compensation)
- [Lens 镜头](#lens)
- [JPEG Damage JPEG 损坏](#jpeg-damage)
- [Blobbylize 胶状化](#blobbylize)
- [Glass 玻璃](#glass)
- [Turbulent Displace 湍流置换](#turbulent-displace)
- [Light Sweep 扫光](#light-sweep)
- [Kaleida 万花筒](#kaleida)
- [Roughen Edges 毛边](#roughen-edges)
- [Starglow 星芒](#starglow)
- [Shine 放射光束](#shine)
- [Saber 能量光束](#saber)
- [Venetian Blinds 百叶窗](#venetian-blinds)

<a id="tint"></a>

## 01. Tint 色调

![Tint 色调 — before and after native Photoshop plugin output](assets/effects/tint.jpg)

暗部蓝色、亮部暖色 / Navy shadows and warm highlights.

<a id="fractal-noise"></a>

## 02. Fractal Noise 分形杂色

![Fractal Noise 分形杂色 — before and after native Photoshop plugin output](assets/effects/fractal-noise.jpg)

对比度 165，比例 145，复杂度 5；本例为独立纹理生成 / Contrast 165, scale 145, complexity 5; generated texture.

<a id="displacement-map"></a>

## 03. Displacement Map 置换图

![Displacement Map 置换图 — before and after native Photoshop plugin output](assets/effects/displacement-map.jpg)

同一照片的隐藏副本作为置换图；水平 38、垂直 17 / A hidden photo duplicate supplies the map; X 38, Y 17.

<a id="glow"></a>

## 04. Glow 发光

![Glow 发光 — before and after native Photoshop plugin output](assets/effects/glow.jpg)

半径 32，强度 0.9，阈值 56 / Radius 32, intensity 0.9, threshold 56.

<a id="deep-glow"></a>

## 05. Deep Glow 深度发光

![Deep Glow 深度发光 — before and after native Photoshop plugin output](assets/effects/deep-glow.jpg)

半径 100，曝光 0.8，阈值 25，平滑 25 / Radius 100, exposure 0.8, threshold 25, smoothness 25.

<a id="chromatic-dispersion"></a>

## 06. Chromatic Dispersion 色差色散

![Chromatic Dispersion 色差色散 — before and after native Photoshop plugin output](assets/effects/chromatic-dispersion.jpg)

光谱扭曲模式；红蓝缩放 103%／97%，旋转 ±0.7° / Spectral warp; red/blue scales 103%/97%, rotations ±0.7°.

<a id="vector-blur"></a>

## 07. Vector Blur 矢量模糊

![Vector Blur 矢量模糊 — before and after native Photoshop plugin output](assets/effects/vector-blur.jpg)

流动预设；数量 18，参考柔和 8，混合 75% / Flow preset; amount 18, map softness 8, mix 75%.

<a id="grid"></a>

## 08. Grid 网格

![Grid 网格 — before and after native Photoshop plugin output](assets/effects/grid.jpg)

64×64 网格，边宽 2，青色，不透明度 80%，正常叠加 / 64×64 grid, border 2, cyan, opacity 80%, Normal.

<a id="checkerboard"></a>

## 09. Checkerboard 棋盘

![Checkerboard 棋盘 — before and after native Photoshop plugin output](assets/effects/checkerboard.jpg)

96×96 棋盘，青色，不透明度 55%，正常叠加 / 96×96 checkerboard, cyan, opacity 55%, Normal.

<a id="cell-pattern"></a>

## 10. Cell Pattern 单元格图案

![Cell Pattern 单元格图案 — before and after native Photoshop plugin output](assets/effects/cell-pattern.jpg)

单元格纹理生成，尺寸 35 / Generated cell texture, size 35.

<a id="optics-compensation"></a>

## 11. Optics Compensation 光学补偿

![Optics Compensation 光学补偿 — before and after native Photoshop plugin output](assets/effects/optics-compensation.jpg)

视场 78，开启反转 / FOV 78, reverse enabled.

<a id="lens"></a>

## 12. Lens 镜头

![Lens 镜头 — before and after native Photoshop plugin output](assets/effects/lens.jpg)

尺寸 66，汇聚 55 / Size 66, convergence 55.

<a id="jpeg-damage"></a>

## 13. JPEG Damage JPEG 损坏

![JPEG Damage JPEG 损坏 — before and after native Photoshop plugin output](assets/effects/jpeg-damage.jpg)

质量 10，错误率 0.08，错误密度 20，混合 90%，固定种子 327 / Quality 10, error rate 0.08, density 20, mix 90%, seed 327.

<a id="blobbylize"></a>

## 14. Blobbylize 胶状化

![Blobbylize 胶状化 — before and after native Photoshop plugin output](assets/effects/blobbylize.jpg)

明度表面，柔和 4，切除 21，效果灯光 / Lightness surface, softness 4, cut away 21, effect light.

<a id="glass"></a>

## 15. Glass 玻璃

![Glass 玻璃 — before and after native Photoshop plugin output](assets/effects/glass.jpg)

明度凹凸，柔和 4，高度 35，置换 55，效果灯光 / Lightness bump, softness 4, height 35, displacement 55, effect light.

<a id="turbulent-displace"></a>

## 16. Turbulent Displace 湍流置换

![Turbulent Displace 湍流置换 — before and after native Photoshop plugin output](assets/effects/turbulent-displace.jpg)

数量 85，尺寸 70，复杂度 2 / Amount 85, size 70, complexity 2.

<a id="light-sweep"></a>

## 17. Light Sweep 扫光

![Light Sweep 扫光 — before and after native Photoshop plugin output](assets/effects/light-sweep.jpg)

中心 (384,330)，宽度 100，强度 85 / Center (384,330), width 100, intensity 85.

<a id="kaleida"></a>

## 18. Kaleida 万花筒

![Kaleida 万花筒 — before and after native Photoshop plugin output](assets/effects/kaleida.jpg)

中心 (384,330)，镜像模式 5 / Center (384,330), mirroring mode 5.

<a id="roughen-edges"></a>

## 19. Roughen Edges 毛边

![Roughen Edges 毛边 — before and after native Photoshop plugin output](assets/effects/roughen-edges.jpg)

先给照片建立椭圆 Alpha 边缘，再应用毛边；边界 26、比例 64。左图显示相同的透明输入 / Elliptical Alpha input, border 26, scale 64; the left image uses the same prepared input.

<a id="starglow"></a>

## 20. Starglow 星芒

![Starglow 星芒 — before and after native Photoshop plugin output](assets/effects/starglow.jpg)

为清楚展示高光响应，输入在眼睛和指尖增加四个 3×3 白色亮点；阈值 0.97、长度 180、增强 1.8。左图也包含这些亮点 / Four 3×3 white glints were added to the input; threshold 0.97, length 180, boost 1.8. The left image includes the same glints.

<a id="shine"></a>

## 21. Shine 放射光束

![Shine 放射光束 — before and after native Photoshop plugin output](assets/effects/shine.jpg)

中心 (280,150)，阈值 0.48，长度 1.6，增强 1.5，滤色、混合 80% / Center (280,150), threshold 0.48, length 1.6, boost 1.5, Screen blend, mix 80%.

<a id="saber"></a>

## 22. Saber 能量光束

![Saber 能量光束 — before and after native Photoshop plugin output](assets/effects/saber.jpg)

使用四段贝塞尔椭圆路径与电流公共参数预设，叠加在照片上；不是自动识别照片轮廓 / A four-segment Bezier ellipse and the Electric common-parameter preset, composited over the photo; no automatic silhouette detection.

<a id="venetian-blinds"></a>

## 23. Venetian Blinds 百叶窗

![Venetian Blinds 百叶窗 — before and after native Photoshop plugin output](assets/effects/venetian-blinds.jpg)

完成 45%，方向 28°，宽度 48，羽化 1.5；棋盘表示输出透明区域 / Completion 45%, direction 28°, width 48, feather 1.5; checkerboard shows output transparency.
