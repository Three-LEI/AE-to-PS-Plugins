# 智能滤镜参数保存

从0.36-rc1起，十九款效果都将完整效果参数交给Photoshop的智能滤镜描述符保存。保存PSD、关闭并重新打开Photoshop后，各滤镜实例可分别重新编辑。Tint仍只支持RGB8，其余十八款支持RGB8／16／32。

使用时先将目标图层转为智能对象，再应用效果并保存PSD。普通像素层直接应用滤镜只得到像素结果，PSD不会因此增加可编辑的智能滤镜。旧文件中从未写入的参数不能追溯恢复；旧版已保存参数的Glass、Blobbylize、湍流置换、扫光、万花筒和毛边继续读取原字段。

参考图层保存为Unicode名称，不把临时sheetID当作永久图层ID。请保持名称唯一；改名、删除或出现重名后需重新打开滤镜选择。移动或修改外部参考层仍需手动重新进入效果并确认更新，PS不会自动追踪这些跨图层依赖。没有额外刷新脚本。

## 共用实现

- `native/common/descriptor_parameters.hpp`管理Action Descriptor v2和Unicode套件v2，提供类型化读写、临时对象释放和完整写入后的提交。失败时保留原描述符。
- `native/common/parameter_fields.json`列出前十三款的每个逻辑参数和固定四字符ID，包含颜色分量、通道记录、布尔值、种子和参考层名称。`ZpSv`是保存版本，`ZpId`是效果身份。
- `scripts/generate-parameter-schema.py`按清单生成`build/parameter_schema.hpp`，并校验源PiPL中的传统aete术语。普通构建只校验，不改写源文件。维护者有意增加字段时才使用`--update-metadata`。
- 共用运行时读取描述符后再打开参数面板；确认后写入完整描述符。后六款保留原字段、版本和事件ID，共用资源释放及提交逻辑。

参数结构、magic和组件身份与0.35-rc2保持一致。后续增加效果或字段时，应选择未使用过的固定ID，补齐类型访问器、有效性校验及术语，并明确旧文件迁移策略。

## 验证

完整构建后可运行：

```powershell
python qa/check-parameter-fields.py
./qa/run-parameter-store.ps1
./qa/run-font-reset.ps1
./qa/run-smart-persistence.ps1 -Phase create -ExpectedVersion '21.2.9'
# 退出空白测试Photoshop，确认进程结束，再执行重开阶段。
./qa/run-smart-persistence.ps1 -Phase reopen -ExpectedVersion '21.2.9'
```

参数测试生成本地非默认夹具；宿主测试要求已安装对应构建、可用的Photoshop COM和空会话，只建立与关闭自身测试文档。输出默认为被忽略的`evidence/persistence-0.36`，不提交测试PSD或截图。重开测试比较完整类型化参数，临时替换后恢复一个实例来触发重算，再核对其它实例及导出像素。公开验证范围见[VALIDATION.md](VALIDATION.md)。
