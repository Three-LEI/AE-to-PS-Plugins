0.21 图标微调：按用户参考图重绘空心吸管图标，使用圆润胶头、圆角护圈、细长管身和渐窄尖端。新增可复用 ui/include/psui/icons.hpp，统一跟随主题文字色与 DPI 缩放。按钮无边框、无背景，仅在悬停/按下时将图标变为蓝色。此次只修改按钮图形及显示版本，保持原有全屏取色、拖动操作与 PS 原生吸管鼠标指针。100%/200% 共用面板已检查。

# 共用颜色控件与全屏吸管（0.20）

色块仍然调用 Photoshop 首选拾色器。旁边改为紧凑的矢量吸管图标，鼠标指针通过官方 UI Hooks 使用 PS 原生吸管；宿主不支持时用系统十字。取色功能与原生拾色器独立：

1. 点击吸管图标，再点击目标；也可按住吸管，拖到目标后松开。
2. 目标可以是缩览图、PS 画布、其它软件、桌面或界面，支持整个可见屏幕。
3. 成功后更新当前颜色和滤镜预览；Esc 或右键取消。无法读取的位置保持原色，可继续重试。

缩览图采用当前显示的可见结果，包括“显示背景”打开时的合成颜色。需要取原图时可以先关闭“预览”。缩览图是 8 位代理图；半透明像素与透明棋盘混合后的显示颜色也属于可见结果，若不希望包含底纹，请点击不透明图像或 PS 画布。缩览图之外统一读取屏幕显示 RGB，包括 PS 画布、界面、其它软件和桌面，不进入宿主的交互式画布采样。屏幕取色是 8 位显示值，不是 HDR 原始像素；显示器与文档配置文件不同时，两种取色的数值可能不同。

底层原理：

- `LibraryPanel::color_swatch` 统一管理色块、PS 拾色器按钮、取色按钮和结果交付。所有颜色控件均使用此入口；`generator_controls::color_control` 仅提供表格排版。
- `PreviewColorSampler` 保存与最后成功上传纹理对应的显示色彩管理前 RGB，并使用相同的线性纹理插值、缩放、平移和裁切范围；黑边和图像边界之外不取色。不会对截图/显示器颜色再次应用文档色彩管理。
- 低层 `host_sample_rgb` 保留供已验证的图像点调用，共用吸管不使用它：PS 在非图像区域可能进入自己的交互式采样，干扰插件操作。该辅助函数使用 Adobe `ColorServicesInfo / plugIncolorServicesSamplePoint / globalSamplePoint`，要求 RGB 返回值。物理屏幕坐标保留负数多显示器位置，超出旧接口有符号 16 位范围时拒绝请求。调用失败或空间不符时保持原色。
- `screen_sample_rgb` 使用只读桌面 DC 的 GetPixel，取一个可见屏幕像素后立即 ReleaseDC；支持有符号的全屏坐标。始终读取最上方实际可见颜色，不会取到其它软件背后的 PS 画布。CLR_INVALID 时不更改原色。
- `host_eyedropper_cursor` 通过 UI Hooks Suite 的 `kPICursorEyedropper` 取得 PS 原生指针；获取后释放 suite，借用的 cursor 不自行销毁。按钮图标使用 ImGui 抗锯齿矢量图形随 DPI 缩放，不绑定 Adobe 内部工具栏资源。
- 鼠标捕获仅属于滤镜窗口；点击不传给 PS 的编辑工具。取色期间其它参数控件暂停交互，缩览图不会平移或缩放。取消、失焦、捕获丢失和窗口退出时都释放捕获。
- 使用稳定的 ImGui 控件 ID 交付结果，不保存跨帧颜色数组指针，不修改前景色、图层、选区或历史。只有屏幕采样读取一个像素，不绘制到屏幕 DC，不新增 HWND 控件或绘制表面，因此参数面板仍由同一 ImGui/DX11 swap chain 绘制。

新效果调用：

```cpp
// 自己安排文字和色块位置：
changed |= ui.color_swatch("color-a", host, color_a);
// 或者使用紧凑的标签/颜色表格：
changed |= psui::color_control(ui, host, "颜色 / Color", color);
```

添加原生颜色选项时不要单独写 `ImGui::ColorButton`。`verify-contract.ps1` 会拒绝绕开共享入口的效果面板，以便后续效果自动继承拾色和吸管。

验证：`native/tests/run-color-tests.ps1`，包含 70 项采样/宿主接口/原生光标检查，以及 100% 英文、200% 中文的实际 DX11 共用面板检查；覆盖预览/画布/屏幕路由、拖动松开取色、原生拾色器保留、不同颜色目标隔离、无效位置重试、Esc/右键/捕获丢失、视点不改变和退出清理。`-Desktop` 还运行真实桌面纯色窗口检查；需要普通交互桌面，受限执行桌面的 DC 不代表用户显示输出。真实桌面已核对 RGB 24/180/216 与 256 次取色无 GDI 句柄增长。实机结果另存于 `evidence/color-picker-0.20`。

Adobe 接口定义：[PIGeneral.h](https://github.com/AdobeDocs/photoshop-cpp-sdk/blob/main/pluginsdk/photoshopapi/photoshop/PIGeneral.h)。
原生吸管指针：[PIUIHooksSuite.h](https://github.com/AdobeDocs/photoshop-cpp-sdk/blob/main/pluginsdk/photoshopapi/photoshop/PIUIHooksSuite.h)。
