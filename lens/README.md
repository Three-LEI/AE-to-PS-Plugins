# Optics Compensation 光学补偿 0.1 / Lens 镜头 0.2

0.2界面修正：名称统一为“Lens 镜头”，删除截图参数按钮。生成内核、默认值、原有组件ID及参数存储结构不变。

2026-11-05，新增两款独立Windows x64 Photoshop滤镜，随0.26整包交付。菜单“滤镜 → ZaYu-AE 效果复刻”。使用时无需AE或原版Cycore插件；已有十款二进制保持0.24原样。

## 参数及行为

光学补偿：FOV默认0（范围0至接近180度）、反转镜头扭曲、水平/垂直/对角线FOV方向、文档坐标视图中心、最佳像素。FOV0保持源RGBA；普通与反转采用成对径向投影。AE的最佳像素会改变缩放并破坏同FOV往返的可逆性，本版普通方向的缩放使用同源参考拟合。

Lens：中心、Size默认50（0–500）、Convergence默认100（-200–100）。圆形镜头边界及其透明渐变独立于收敛度；Convergence0仍有圆形裁切，不是全图恒等。按图像对角线确定镜头半径，正负收敛度分别产生相反的径向变化。Size61/Convergence11仅为原版研究对照值，不作为默认值，也不提供额外快捷按钮。

中心默认文档中心；右键缩览图可设置中心。复用共用蓝色数值拖动、中英切换、主题/DPI、关于、后台预览、显示背景、缩放和平移；视图操作不改变参数。RGB8、PS16（32768白色标尺）及float32/HDR，采样按预乘颜色插值后恢复直通RGBA，避免透明边缘色晕。输入为当前目标图层，不读取其它层作为图像；“显示背景”只影响预览。

## 原版对照与准确边界

在本机AE26.5x89中正常脚本采集69组640×480原生PNG，源图完全相同：光学补偿51组，覆盖FOV30/60/90/120、三方向、正反转、偏移中心及最佳像素；CC Lens18组，覆盖Size25/50/100、正负/零收敛度、61/11和偏移中心。

固定案例的最大单案例平均误差（0–255标尺）：CC Lens预乘RGB约0.448、Alpha约0.361；光学补偿RGB约0.950、Alpha约0.546。这些是该组案例的平均量，不是单像素最大误差或任意图像保证。高对比线条、图像边缘、重采样/抗锯齿与极端参数仍可能不同。最佳像素普通方向为经验拟合，部分拟合案例也计入69组，不能当作独立的普适精度证明。原始参数与比较明细见`evidence/lens-ae-reference/latest.json`和`evidence/lens-v01/ae-parity.json`（均相对主工程）。

**调整大小限制：** AE的Resize能扩展效果图层边界。Photoshop原生滤镜受PS提供的作用区域及画布限制，本版仅实现Resize Off；面板明确显示固定画布说明，没有无效的Max2X/4X/Unlimited控件。需保留边缘时先扩展PS画布。不会自动改文档尺寸。

其它限制：完整原生RGBA快照上限16,777,216像素；没有GPU内核、主画布实时预览、动作参数描述/智能滤镜参数重编辑。色值使用文档数值RGB，不自动解析ICC/ACES；HDR预览限幅显示。空白层不会凭空生成内容。结果限制于PS给出的输出范围。

## 验证及复现

- 内核：FOV0/HDR/Alpha、两种模型全部控件、并行确定性、零半径、取消与参数保护。
- 两个真实DLL各4,739,492项模拟宿主断言：非默认畸变、RGB8/PS16/32、平面图、保护透明度、任意分块、新旧坐标、选区/行填充和取消/读取失败清理。
- 八组生产面板：两款各中英100%/200%，参数编辑、后台预览、背景、缩放、关于、确认与取消。
- PS27.7实际默认参数：Optics FOV0、CC Size50/Convergence100，RGB8/16/32、选区外保留、锁定Alpha及一条滤镜历史记录回退；光学补偿非零畸变由AE参考和真实DLL模拟宿主覆盖，不将其当作PS非零实机测试。

主工程中先准备`native/build.ps1`公共库，再执行`lens/build.ps1 -BuildTarget CCLens`；`-CoreOnly`只运行内核。然后运行`python tests/compare_lens_reference.py`与两款`*-dialog_preview.exe --qa-zh/--qa-en/--qa-zh-200/--qa-en-200`。`python tests/verify_lens_rename_release.py --preinstall`绑定安装前结果，`tests/install_lens_name_release.ps1`安装；通过官方PS脚本接口运行`lens/tests/host_smoke.jsx`，再运行`python tests/verify_lens_rename_release.py`及`python tests/package_lens_name_release.py`。开发工具依赖Pillow/NumPy及本地参考，成品不依赖这些工具。

本地证据：主工程`evidence/lens-v02/qa.json`与`installed.json`。MIT许可，by：淚泪泪，开源免费，非Adobe/Cycore官方移植，没有使用原版二进制代码。

官方行为资料：[Adobe Optics Compensation](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/distort-effects.html)、[Cycore手册CC Lens第21页](https://cycorefx.com/downloads/cfx_hd_std/CycoreFX%201.6%20Manual.pdf)。默认值/范围以本机正常AE参数查询为准。
