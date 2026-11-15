# claims/：每个并行会话一份工作日志

命名：`<分区>.md`，例如 `Source_Plugins_Star.md`、`Python.md`。

格式（追加式，最新在最上面）：

```
claim: Source_Plugins_Star F10,F12 @session-a 2026-11-15T18:30
F12 fixed by a87437f; 验证: ctest -R fluid_solar_column → 通过；反证: 把符号写反 → FAILED
F10 分析: 多层势逐层独立求解属设计选择，改为 ...（结论）
```

规则：
- 只有该会话写这个文件；
- 每条结论都要有**提交号**与**可复现的验证命令**；
- 不确定的就写"不确定 + 缺什么证据"，不要写成结论。
