0.35-rc1 · 旧版 Windows／Photoshop 加载兼容修复候选版

针对0.34在Win7＋PS2018、Win10＋PS2020没有滤镜分组的反馈，重建十九款Windows x64原生滤镜：

- 改用传统二进制PiPL／aete注册，保留组件ID、菜单名称、透明度／选区处理和智能滤镜参数术语。
- 较新DPI接口改为按需检测，旧系统使用Win32回退；不修改Photoshop进程的全局DPI设置。
- 内置固定ImGui HLSL对应的SM4字节码，移除运行时D3DCompiler DLL依赖。
- 效果生成内核、默认值、参数结构及各滤镜自身版本保持0.34。

本机已通过十三模块构建、十九款DLL适配、32组界面检查（含16组旧DPI回退），以及PS2026的55个效果／位深组合；113份导出与0.34像素一致。

**Win7＋PS2018和Win10＋PS2020仍待用户实机复测。本版本作为Pre-release提供，不宣称已通过这些旧环境验证。**

退出Photoshop并备份旧插件，替换同名`.8bf`后重启。系统和Photoshop均需64位；避免多个插件目录中有重复版本。普通使用无需AE、SDK、Python或单独下载D3DCompiler DLL。Tint仅RGB8，其余位深及限制见模块说明。
