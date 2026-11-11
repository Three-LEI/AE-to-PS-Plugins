# Numeric controls / 数值控件（公共组件 0.18）

九款效果统一使用蓝色数值。深浅主题分别选用浅蓝/深蓝；四种 PS 主题中的普通、悬停和活动背景均验证了至少 4.5:1 的文本对比度。禁用值跟随 ImGui 的禁用透明度。

- 左右拖动数值区域：向右增加，向左减少。超过拖动阈值后才调值。
- Shift：10 倍速度；Ctrl：0.1 倍速度；同时按下：正常速度。可在拖动途中切换，不会跳值。
- 轻点松开或双击：进入键盘输入；Enter 确认；Esc 恢复本次数值并留在滤镜中。输入时 Ctrl+A/C/V 保持正常。
- 普通滑块仍保留。整数、上下限、禁用状态由同一个组件处理。Ctrl 拖动整数时会累积小数步长。
- 普通数值速度根据可视滑块跨度设置：每个逻辑像素为 `clamp(span/200, .005, 1)` 个单位；整数为 1。坐标和预览百分比为 1。按 DPI 换算物理像素，使 100%/150%/200% 下手感一致。

新效果优先使用 `LibraryPanel::number`，坐标使用 `point_control`。需要自定义排版时可调用 `numeric_value`，最后一个速度参数为每个逻辑像素对应的数值单位：

```cpp
ui.number("强度 / Intensity", strength, 0, 100, 0, 100);
psui::point_control(ui, "中心 / Center", center_x, center_y);
ImGui::SetNextItemWidth(80 * ui.scale());
ui.numeric_value("##custom", gamma, .01, 10, .01);
```

不要在效果面板中直接使用 `InputDouble` / `DragScalar`，否则会绕过统一颜色、修饰键和 Esc 处理。`ui/verify-contract.ps1` 会检查这种回退。

`scrub_control.hpp` / `numeric_scrub.cpp` 是不依赖 Photoshop 宿主的数值实现。它基于固定的 Dear ImGui 1.91.9b 内部接口；版本断言要求升级 ImGui 时复查激活逻辑。没有改写全局 IO 修饰键，也没有新增 HWND 控件或独立绘制表面。`library_panel.cpp` 负责本地化提示和阻止编辑用的 Esc 关闭外层面板。预览缩放只修改 PreviewView，效果参数不随之改变。

主工程验证：`native/tests/build-scrub-tests.ps1`（76 项检查）；`native/tests/run-scrub-panel.ps1`（实际 DX11 共用面板，100% 英文和 200% 中文、深浅主题）。九款生产面板另有 smoke 检查。本次只更改共用 UI 与显示版本，效果内核和保存参数布局未修改。
