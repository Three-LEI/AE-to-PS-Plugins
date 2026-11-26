# 参数窗口图集 / Parameter-window gallery

0.41 收录 41 款滤镜。下图均为实际参数窗口，显示效果预览和当前参数。

These are actual production parameter-window captures, including the full-quality effect preview and the configured controls. The gallery includes 41 lossless PNG captures.

[返回首页](../README.md) · [下载插件包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.41/AE-to-PS-Plugins-0.41-Windows-x64.zip) · [完整参数数据](gallery-parameters.json) · [演示素材许可](assets/README.md)

## 输入与复现

照片由维护者提供，公开输入为已清理非必要元数据的 768×768 图像：[demo-source.png](assets/demo-source.png)。毛边使用椭圆 Alpha 输入，Starglow 输入添加少量高光点，置换图使用参考副本，Saber 使用明确的椭圆路径；各例在下方注明。

球面和圆柱使用同一张自制标签展示两种局部贴图。渲染图集时自动生成标签。只截取这两款的参数窗口可直接准备[合成输入](../scripts/new-texturewrap-gallery-inputs.ps1)。

截图工具复用插件的生产界面和预览代码，加载对应素材与参数，等待精细预览完成后截取窗口。它独立运行，不打开或修改个人 Photoshop 文档；不是原版 AE 参数面板。参数较多的效果保留窗口正常的分组折叠与滚动布局。

开发者先完成正常构建，再准备示例输入并截取参数窗口：

```powershell
./scripts/render-gallery.ps1 -Source docs/assets/demo-source.png -Parameters docs/gallery-parameters.json -Output gallery-inputs
./scripts/capture-gallery-ui.ps1 -Renders gallery-inputs -Output gallery-ui
./scripts/package-gallery-ui.ps1 -Captures gallery-ui/captures -Assets gallery-assets
```

第一步使用已安装的插件生成演示输入，默认连接 PS2020 21.2.9，其他版本需指定 `-PhotoshopProgId` 和 `-ExpectedVersion`；后两步为独立界面截取与无损打包。打包不修改首页分享封面。截图仅展示本项目实际行为，不表示与商业原版逐像素一致。

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
- [Halftone 网点印刷](#halftone)
- [ASCII Dither 字符抖动](#ascii-dither)
- [Dither 像素抖动](#pixel-dither)
- [Lens Flare 镜头耀斑／耀斑设计器](#lens-flare)
- [Etching 蚀刻](#etching)
- [Caustics 焦散](#caustics)
- [Film Damage 胶片损伤](#film-damage)
- [Long Shadow 长投影](#long-shadow)
- [Toner 多色调](#toner)
- [Mirror 镜像](#mirror)
- [RepeTile 重复拼贴](#repetile)
- [Noise 综合杂色](#noise)
- [Sphere 球面贴图](#sphere)
- [Cylinder 圆柱贴图](#cylinder)
- [Rain & Snow 雨雪模拟](#rain-snow)
- [Channel Blur 通道模糊](#channel-blur)
- [Compound Blur 复合模糊](#compound-blur)
- [Pixel Sort 像素排序](#pixel-sort)

<a id="tint"></a>

## 01. Tint 色调

![Tint 色调 — production parameter window and full effect preview](assets/effects/tint.png)

暗部蓝色、亮部暖色 / Navy shadows and warm highlights.

<a id="fractal-noise"></a>

## 02. Fractal Noise 分形杂色

![Fractal Noise 分形杂色 — production parameter window and full effect preview](assets/effects/fractal-noise.png)

对比度 165，比例 145，复杂度 5；本例为独立纹理生成 / Contrast 165, scale 145, complexity 5; generated texture.

<a id="displacement-map"></a>

## 03. Displacement Map 置换图

![Displacement Map 置换图 — production parameter window and full effect preview](assets/effects/displacement-map.png)

同一照片的隐藏副本作为置换图；水平 38、垂直 17 / A hidden photo duplicate supplies the map; X 38, Y 17.

<a id="glow"></a>

## 04. Glow 发光

![Glow 发光 — production parameter window and full effect preview](assets/effects/glow.png)

半径 32，强度 0.9，阈值 56 / Radius 32, intensity 0.9, threshold 56.

<a id="deep-glow"></a>

## 05. Deep Glow 深度发光

![Deep Glow 深度发光 — production parameter window and full effect preview](assets/effects/deep-glow.png)

半径 100，曝光 0.8，阈值 25，平滑 25 / Radius 100, exposure 0.8, threshold 25, smoothness 25.

<a id="chromatic-dispersion"></a>

## 06. Chromatic Dispersion 色差色散

![Chromatic Dispersion 色差色散 — production parameter window and full effect preview](assets/effects/chromatic-dispersion.png)

光谱扭曲模式；红蓝缩放 103%／97%，旋转 ±0.7° / Spectral warp; red/blue scales 103%/97%, rotations ±0.7°.

<a id="vector-blur"></a>

## 07. Vector Blur 矢量模糊

![Vector Blur 矢量模糊 — production parameter window and full effect preview](assets/effects/vector-blur.png)

流动预设；数量 18，参考柔和 8，混合 75% / Flow preset; amount 18, map softness 8, mix 75%.

<a id="grid"></a>

## 08. Grid 网格

![Grid 网格 — production parameter window and full effect preview](assets/effects/grid.png)

64×64 网格，边宽 2，青色，不透明度 80%，正常叠加 / 64×64 grid, border 2, cyan, opacity 80%, Normal.

<a id="checkerboard"></a>

## 09. Checkerboard 棋盘

![Checkerboard 棋盘 — production parameter window and full effect preview](assets/effects/checkerboard.png)

96×96 棋盘，青色，不透明度 55%，正常叠加 / 96×96 checkerboard, cyan, opacity 55%, Normal.

<a id="cell-pattern"></a>

## 10. Cell Pattern 单元格图案

![Cell Pattern 单元格图案 — production parameter window and full effect preview](assets/effects/cell-pattern.png)

单元格纹理生成，尺寸 35 / Generated cell texture, size 35.

<a id="optics-compensation"></a>

## 11. Optics Compensation 光学补偿

![Optics Compensation 光学补偿 — production parameter window and full effect preview](assets/effects/optics-compensation.png)

视场 78，开启反转 / FOV 78, reverse enabled.

<a id="lens"></a>

## 12. Lens 镜头

![Lens 镜头 — production parameter window and full effect preview](assets/effects/lens.png)

尺寸 66，汇聚 55 / Size 66, convergence 55.

<a id="jpeg-damage"></a>

## 13. JPEG Damage JPEG 损坏

![JPEG Damage JPEG 损坏 — production parameter window and full effect preview](assets/effects/jpeg-damage.png)

质量 10，错误率 0.08，错误密度 20，混合 90%，固定种子 327 / Quality 10, error rate 0.08, density 20, mix 90%, seed 327.

<a id="blobbylize"></a>

## 14. Blobbylize 胶状化

![Blobbylize 胶状化 — production parameter window and full effect preview](assets/effects/blobbylize.png)

明度表面，柔和 4，切除 21，效果灯光 / Lightness surface, softness 4, cut away 21, effect light.

<a id="glass"></a>

## 15. Glass 玻璃

![Glass 玻璃 — production parameter window and full effect preview](assets/effects/glass.png)

明度凹凸，柔和 4，高度 35，置换 55，效果灯光 / Lightness bump, softness 4, height 35, displacement 55, effect light.

<a id="turbulent-displace"></a>

## 16. Turbulent Displace 湍流置换

![Turbulent Displace 湍流置换 — production parameter window and full effect preview](assets/effects/turbulent-displace.png)

数量 85，尺寸 70，复杂度 2 / Amount 85, size 70, complexity 2.

<a id="light-sweep"></a>

## 17. Light Sweep 扫光

![Light Sweep 扫光 — production parameter window and full effect preview](assets/effects/light-sweep.png)

中心 (384,330)，宽度 100，强度 85 / Center (384,330), width 100, intensity 85.

<a id="kaleida"></a>

## 18. Kaleida 万花筒

![Kaleida 万花筒 — production parameter window and full effect preview](assets/effects/kaleida.png)

中心 (384,330)，镜像模式 5 / Center (384,330), mirroring mode 5.

<a id="roughen-edges"></a>

## 19. Roughen Edges 毛边

![Roughen Edges 毛边 — production parameter window and full effect preview](assets/effects/roughen-edges.png)

先给照片建立椭圆 Alpha 边缘，再应用毛边；边界 26、比例 64。窗口预览使用相同的透明输入 / Elliptical Alpha input, border 26, scale 64; the window preview uses this prepared input.

<a id="starglow"></a>

## 20. Starglow 星芒

![Starglow 星芒 — production parameter window and full effect preview](assets/effects/starglow.png)

为清楚展示高光响应，输入在眼睛和指尖增加四个 3×3 白色亮点；阈值 0.97、长度 180、增强 1.8。窗口预览也包含这些亮点 / Four 3×3 white glints were added to the input; threshold 0.97, length 180, boost 1.8. The preview includes the same glints.

<a id="shine"></a>

## 21. Shine 放射光束

![Shine 放射光束 — production parameter window and full effect preview](assets/effects/shine.png)

中心 (280,150)，阈值 0.48，长度 1.6，增强 1.5，滤色、混合 80% / Center (280,150), threshold 0.48, length 1.6, boost 1.5, Screen blend, mix 80%.

<a id="saber"></a>

## 22. Saber 能量光束

![Saber 能量光束 — production parameter window and full effect preview](assets/effects/saber.png)

使用四段贝塞尔椭圆路径与电流公共参数预设，叠加在照片上；不是自动识别照片轮廓 / A four-segment Bezier ellipse and the Electric common-parameter preset, composited over the photo; no automatic silhouette detection.

<a id="venetian-blinds"></a>

## 23. Venetian Blinds 百叶窗

![Venetian Blinds 百叶窗 — production parameter window and full effect preview](assets/effects/venetian-blinds.png)

完成 45%，方向 28°，宽度 48，羽化 1.5；棋盘表示输出透明区域 / Completion 45%, direction 28°, width 48, feather 1.5; checkerboard shows output transparency.

<a id="halftone"></a>

## 24. Halftone 网点印刷

![Halftone 网点印刷 — production parameter window and full effect preview](assets/effects/halftone.png)

圆点，间距 18px，角度 25°，平滑 4px，明亮度 4，对比度 125%，伽马 0.95，深紫／浅黄双色。示例参数 / Round dots, 18px spacing, 25°, source smoothing 4px, brightness 4, contrast 125%, gamma 0.95, purple/yellow duotone. Included in 0.38.

<a id="ascii-dither"></a>

## 25. ASCII Dither 字符抖动

![ASCII Dither 字符抖动 — production parameter window and full effect preview](assets/effects/ascii-dither.png)

经典 ASCII，Consolas 粗体，宽 14px，高宽比 1.6，填充 95%，有序抖动 85%，明亮度 6，对比度 125%，伽马 1.35，绿色终端配色。示例参数 / Classic ASCII, Consolas Bold, 14px width, height ratio 1.6, fill 95%, ordered dither 85%, brightness 6, contrast 125%, gamma 1.35, green terminal palette. Included in 0.38.

<a id="pixel-dither"></a>

## 26. Dither 像素抖动

![Dither 像素抖动 — production parameter window and full effect preview](assets/effects/pixel-dither.png)

Bayer 8×8，黑白双色，像素尺寸 4，强度 100%，明亮度 3，对比度 115%，伽马 1.1。示例参数 / Bayer 8×8, black/white duotone, pixel size 4, amount 100%, brightness 3, contrast 115%, gamma 1.1. Included in 0.38.

<a id="lens-flare"></a>

## 27. Lens Flare 镜头耀斑／耀斑设计器

![Lens Flare 镜头耀斑 — production parameter window and full effect preview](assets/effects/lens-flare.png)

0.3设计器将电影蓝光公共参数转换为独立元素，选中“鬼影 4”，默认名称跟随界面语言；支持十二种元素；光源 (610,145)、光学中心 (384,384)、亮度1.15。全局及转换前参数见示例记录，元素编辑和预设格式见[本地说明](FLARE_DESIGNER_LOCAL.md)。示例参数 / Designer component 0.3 with twelve element types: Cinematic Blue converted into independent objects, Ghost 4 selected; hotspot (610,145), pivot (384,384), brightness 1.15. Included in 0.38.

<a id="etching"></a>

## 28. Etching 蚀刻

![Etching 蚀刻 — production parameter window and full effect preview](assets/effects/etching.png)

绿钞雕版预设，频率85，暗部交叉排线，两组角度30°／−20°，波形幅度0.045，亮度扭曲0.035，墨绿／浅米纸色，颗粒12%，源平滑0.8像素，伽马0.97。完整值见参数数据；[本地说明](ETCHING_LOCAL.md)。示例参数 / Green Engraving, frequency85, tonal crosshatch, angles30/-20, wave0.045, warp0.035, green ink and cream paper, grain12%, source smooth0.8px, gamma0.97. Included in 0.38.

<a id="caustics"></a>

## 29. Caustics 焦散

![Caustics production parameter window and full effect preview](assets/effects/caustics.png)

使用维护者授权照片。0.2新实例默认值：内置自然水波，尺寸160、波高18、水深120、聚焦1.5、柔化1.5、光纹强度100%、折射扭曲100%；全部保存字段见参数数据。0.39 正式收录。[使用说明](CAUSTICS_LOCAL.md)

<a id="film-damage"></a>

## 30. Film Damage 胶片损伤

![Film Damage 胶片损伤 — production parameter window and full effect preview](assets/effects/film-damage.png)

使用维护者授权照片，放映机损伤风格：颗粒18%、灰尘220处、划痕16条、毛发5条、污渍6处，随机种子413、样式变化12。0.39 正式收录。[使用说明](FILMDAMAGE_LOCAL.md)

<a id="long-shadow"></a>

## 31. Long Shadow 长投影

![Long Shadow 长投影 — production parameter window and full effect preview](assets/effects/long-shadow.png)

透明文字与圆形演示：方向45°，长度320像素，不透明度82%，扩张2像素，尾部渐隐35%，远端模糊14像素，纯色深蓝阴影。示例参数 / Transparent text and circle, angle 45°, length 320 px, opacity 82%, expansion 2 px, tail fade 35%, far blur 14 px, deep blue solid shadow. Included in 0.39.

<a id="toner"></a>

## 32. Toner 多色调

![Toner 多色调 — production parameter window and full effect preview](assets/effects/toner.png)

五色调暖调示例：高光米白、亮部浅桃、中间调橙棕、暗部蓝灰、阴影深海军蓝，混合原图 0%。按亮度线性映射，支持双色调、三色调、五色调和纯色。[使用说明](TONER_LOCAL.md)

<a id="mirror"></a>

## 33. Mirror 镜像

![Mirror 镜像 — production parameter window and full effect preview](assets/effects/mirror.png)

反射中心 384、384，角度 90°，把上侧镜像到下侧；预览缩放/平移时读取可见区及其镜像区并按文档坐标换算，镜像线不跟随视图移动，也不会把裁剪边缘拉伸进画面。[使用说明](../mirror/README.md)

<a id="repetile"></a>

## 34. RepeTile 重复拼贴

![RepeTile 重复拼贴 — production parameter window and full effect preview](assets/effects/repetile.png)

透明边距内四向扩展 192 像素，水平翻转模式，接缝混合 12 像素。演示照片被重复拼贴到透明区域。[使用说明](../repetile/README.md)

<a id="noise"></a>

## 35. Noise 综合杂色 0.2.1

![Noise 综合杂色 — production parameter window and full effect preview](assets/effects/noise.png)

高斯杂色 8%，颗粒强度 16%，颗粒尺寸 2.2 像素，粗糙度 60%，中间调位置 44%。叠加雪花 12%、横向干扰 25%、色度拖尾 10 像素和局部掉讯 28%，效果混合为 80%。 [使用说明](../noise/README.md)

<a id="sphere"></a>

## 36. Sphere 球面贴图 0.5

![Sphere 球面贴图 — production parameter window and full effect preview](assets/effects/sphere.png)

当前图层同时覆盖整球并作为局部贴花。贴花长边张角 84°，中心经度 26°、纬度 15°。 / The current layer wraps the sphere and supplies a local decal with an 84° long edge at 26° longitude and 15° latitude. [使用说明](../texturewrap/README.md)

<a id="cylinder"></a>

## 37. Cylinder 圆柱贴图 0.6

![Cylinder 圆柱贴图 — production parameter window and full effect preview](assets/effects/cylinder.png)

当前图层用于完整柱面和局部贴花。贴花覆盖 120° 弧段，周向位置 18°，上下位置 42%，高度为柱体的 62%。 / The current layer wraps the cylinder and supplies a decal over a 120° arc, 18° around the cylinder and 42% down its axis, with a height of 62%. [使用说明](../texturewrap/README.md)

<a id="rain-snow"></a>

## 38. Rain & Snow 雨雪模拟 0.2

![Rain & Snow 雨雪模拟 — production parameter window and full effect preview](assets/effects/rain-snow.png)

浅蓝色雨线合成到原图，风向 −27°，雨线长度 46 像素。雨和雪可分别选色。 / Custom light blue rain over the source with −27° wind and 46 px streaks. Rain and snow have independent colors. [使用说明](../weather/README.md)

<a id="channel-blur"></a>

## 39. Channel Blur 通道模糊 0.1.1

![Channel Blur 通道模糊 — production parameter window and full effect preview](assets/effects/channel-blur.png)

亮度与色度模式仅柔化色度，半径 26 像素。 / Luminance and Chroma mode with a 26 px chroma radius and zero luminance radius. [使用说明](../channelblur/README.md)

<a id="compound-blur"></a>

## 40. Compound Blur 复合模糊 0.1.1

![Compound Blur 复合模糊 — production parameter window and full effect preview](assets/effects/compound-blur.png)

四个色标组成径向灰度模糊图，中心保持清晰，最大半径 34 像素。 / Four stops define a radial blur map with a sharp center and a 34 px maximum radius. [使用说明](../compoundblur/README.md)

<a id="pixel-sort"></a>

## 41. Pixel Sort 像素排序 0.1

![Pixel Sort 像素排序 — production parameter window and full effect preview](assets/effects/pixel-sort.png)

亮度 16–68% 的像素沿水平条带从亮到暗排列，最大长度 124 像素。 / Horizontal stripes sort pixels in the 16–68% luminance range from light to dark, with a 124 px length cap. [使用说明](../pixelsort/README.md)
