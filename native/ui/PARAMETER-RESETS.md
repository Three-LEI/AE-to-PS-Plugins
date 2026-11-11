# 参数重置 / Parameter resets（0.27）

十二款效果统一使用参数名前的蓝色回转箭头。图标无背景和边框，颜色取自数值控件的 `numeric_color()`，随主题和 DPI 调整。单击仅恢复对应参数；坐标行同时恢复 X/Y，颜色行恢复 RGB。禁用参数沿用禁用状态；已在默认值的参数不触发重复计算。底部按钮明确显示“全部重置 / Reset All”，恢复全部效果参数，包括当前未展开或禁用的参数。预览、缩放、背景和语言属于界面状态。

每帧在绘制前调用 `ui.parameter_defaults(parameters, factory_defaults)`，传入与“全部重置”相同的默认结构。默认值立即复制；参数块地址仅用于当前帧的字段定位。`number`、`choice`、`check` 自动获取直接成员的默认值。`point_control` 和 `color_control` 共用该绑定。默认坐标通过插件已有的文档尺寸计算，不使用上次保存的参数作为默认值。

转换为临时 double/int 的控件需先调用 `inherit_number_default(label, field)` / `inherit_choice_default(label, enum_field)`；百分比等单位转换需显式调用 `number_default(label, converted_factory_value)`。额外的颜色、图层选择器用 `parameter_label(label, field)`；RGB 指针需明确传入 `3*sizeof(double)`。单项控件返回 changed，继续走原有预览更新与宿主图层读取路径。图层重置必须同步选中项、稳定标识和输入快照；置换图恢复与首次调用相同的默认采样图层。

测试定位键为原参数标签加 `.reset`；原值、滑条、下拉选项及底部 `reset` 键保持不变。`native/tests/run-reset-tests.ps1` 在真实共享面板中验证字段隔离、数值/枚举适配、坐标、颜色、图层、布尔值、禁用/重复点击、全量恢复和两种 DPI/主题。`tests/run-parameter-reset-ui.ps1` 检查十二款生产界面。

增加了共享 C++ 面板接口，需重新编译 `library_ui.lib` 及使用该库的插件；不改变 Photoshop 宿主接口、参数结构、保存参数 magic、ComponentName 或渲染内核。常规构建入口仍为各模块的 `build.ps1`；完整验证见主工程 `evidence/parameter-reset-0.27/qa.json`。示例和测试路径基于主工程布局及其 SDK/工具链。
