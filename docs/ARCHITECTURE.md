# 结构与边界

十九款效果分布在十三个模块中，清单见 [EFFECTS.md](EFFECTS.md)。

- 每个模块的 `include`／`src` 保存参数、独立 CPU 核心、Photoshop 入口、PiPL 资源和生产面板。
- `native/common` 提供分块输出、完整输入缓存、像素混合、并行范围工具及共用描述符参数保存层。
- `native/ui` 提供共用 Dear ImGui／D3D11 面板、参数拖动、单项重置、颜色拾取、背景预览、语言／DPI和关于窗口。
- 模块 `tests/*.cpp` 为核心及 DLL／模拟宿主测试；`displacement/tests/mock_host.hpp` 是多模块共用的宿主夹具。
- `qa` 保存可重现的共用关于链接检查、Photoshop 前后对照和发布审查。宿主测试只创建／关闭自身测试文档。
- `scripts` 保存路径解析、仓库链接配置、干净源码导出及发布工具。

新旧滤镜运行时并存，避免为了目录整理而改动已有像素算法。参数块 magic、组件身份和智能滤镜描述符 schema 属于兼容性边界，修改时需要迁移和 PSD 重开测试。

`native/common/descriptor_parameters.hpp`统一管理旧版Action Descriptor／Unicode套件、资源释放和原子提交。前十三款由`parameter_fields.json`定义稳定字段ID，构建时生成类型化访问器与校验传统aete术语；后六款沿用既有描述符字段，通过同一资源和提交层保存。字段ID与组件身份不能随意复用。实现及扩展流程见[PARAMETER_PERSISTENCE.md](PARAMETER_PERSISTENCE.md)。

“开源地址”由 `source_repository.hpp` 统一提供，在滤镜内的关于弹窗和 Photoshop 的关于增效工具入口显示为蓝色无边框链接。点击时使用 Windows 默认浏览器；不会随预览计算或打开参数窗口自动访问网络。测试钩子可以截获 URL，验证点击而不启动浏览器。

完整分辨率异步预览将渲染与显示分离；缩放和平移用于查看结果。部分模块可复用不可变源图的预处理缓存。像素范围分区相互独立，取消回调留在调用线程，工作线程退出后才释放源图和缓存。

实际位深、透明度、图层参考和尺寸限制随效果变化。Glass／Blobbylize 的外部参考层不会被 PS 自动追踪，用户需重新进入滤镜确认更新；退休的外部刷新脚本不属于当前源码和发行包。
