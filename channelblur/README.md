# Channel Blur 通道模糊 0.1.1

在「滤镜 → ZaYu-模糊和锐化」打开。「RGB 通道」可分别调整红、绿、蓝的模糊半径。「亮度与色度」将明暗与色彩分开处理，亮度半径为 0 时可单独柔化颜色。两种模式均可独立柔化透明度，并选择双向、仅水平或仅垂直。

- 只调「透明度」可柔化抠图边界。羽化带内，邻近的可见颜色在原轮廓两侧连续过渡，消除随 Alpha 羽化留下的硬线。
- 半径以文档像素计，支持小数。RGB 半径设为 0 时，主体内部远离羽化带的像素保留自身细节。「效果混合」为 0 时输出原图。
- RGB32 保留浮点 HDR 范围。亮度与色度依据输入 RGB 的 Rec.709 权重分离，RGB8／16 写出时仍受 Photoshop 位深范围限制。

滤镜支持 RGB8／16／32、智能滤镜和公共自适应预览。CPU 使用可分离滑动窗口近似高斯。100% 及以上缩放的可见区读取完整的邻域，选区仅限制输出范围。

功能参考 AE 的 Channel Blur 与 Sapphire BlurChroma。 [Adobe Channel Blur](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/blur-sharpen-effects.html) · [Boris FX BlurChroma](https://borisfx.com/documentation/sapphire/ae/blurchroma/)

从工作区根目录运行 `./build.ps1 -Module channelblur` 构建。 [参数窗口](../docs/GALLERY.md#channel-blur)
