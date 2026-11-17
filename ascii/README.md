# ASCII Dither 字符抖动（0.38收录）

**已收录0.38正式版。** 内部组件编号沿用开发编号，发行渠道以整包版本为准。[安装包](https://github.com/Three-LEI/AE-to-PS-Plugins/releases/download/v0.38/AE-to-PS-Plugins-0.38-Windows-x64.zip) · [验证范围](../docs/VALIDATION.md)

把图像分成字符单元，以字形的实际疏密表现亮度，再用抖动分配单元之间的量化误差。已纳入0.38正式版。

## 使用

- 四个字符集：经典 ASCII、精细字符、二进制、数字。
- 关闭抖动、4×4 有序抖动、蛇形 Floyd–Steinberg 误差扩散；可调抖动强度。
- 字符宽度、单元高宽比、字符填充、粗体、网格偏移、明暗／对比／伽马、反向映射。
- 单色或原图彩色字符、字符色与底色、字符亮度、实色／透明背景、效果混合。
- 六组预设：绿色终端、黑字纸面、彩色字符、二进制屏幕、密集文本、透明字符。预设只设置公共参数。

支持 RGB8／16／32、中文／English、蓝色单项／全部重置、快速预览及停止后的精细补全。智能滤镜保存全部 24 个字段及实例身份。

输出为栅格图像。透明背景将源 Alpha 乘以字符覆盖率；背景层和锁定透明度会限制透明输出。缩小时细字符趋近其平均覆盖率，检查字符形状可放大到 100%。字符密度本身会限制亮度范围，可用伽马、明亮度和字符亮度调整观感。

## 字体与实现

使用本机安装的 Consolas、Courier New 或 Lucida Console。运行时通过 Windows GDI 生成字形掩码，按实际覆盖率排序；插件及源码不附带字体文件。字体选择不可用时尝试 Courier New，窗口显示实际使用的字体。不同系统上的字体版本或替代字体可能导致字形略有区别。

字符采用面积积分采样。单元取 Alpha 加权平均颜色，避免透明区隐藏颜色污染；单色着色或对源色相归一化。字形积分缓存仅存在于预览窗口，不写入 PSD，关闭窗口释放。最终输出重新读取源图及系统字形生成。

完整处理上限为 16,777,216 像素、4,194,304 个字符单元。亮度映射使用源 RGB 的 0–1 范围，原图彩色模式保留 HDR 通道比例；效果混合为 0 时返回原始源值。图案使用文档坐标锚定，选区只限制最终输出。

Windows API 参考：[CreateFontW](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfontw)、[GetTextFaceW](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gettextfacew)、[CreateDIBSection](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection)。

## 构建

在项目根目录运行 `./build.ps1 -Module ascii`，生成 `ascii/build/ZaYuASCIIDither.8bf`。`-CoreOnly` 也会在 Windows 上验证系统字形与资源回收，链接系统 GDI，无需另装字体库。沿用传统 PiPL/aete、共享界面和旧描述符接口。

测试与试用说明见 [ASCII 本地记录](../docs/ASCII_LOCAL.md)。原有 24 款滤镜不因构建本模块而重新生成。代码采用根目录 MIT 许可，系统字体保持各自授权范围。
