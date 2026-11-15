# lead-root 工作日志（BACKLOG 尾部窗口）

claim: Source_Plugins_ParticleCore_A F5; Source_Plugins_ParticleCore_B F1,F2,F5-F10; Source_Plugins_PicEm_A F1,F2,F5-F10,F12 @lead-root 2026-11-15T18:26

范围说明：BACKLOG 189 条中「倒数第 40～第 21 位」的未认领窗口（条目 71-90）。
- 已跳过 `@lead-tools` 认领的尾部 20 条（Tools_Tests F9,F15-F31；Star F11；PicEm_B F4）；
- 已跳过 `@lead` 认领的 ParticleCore_B F3/F4 与 Runtime_B F6。
- 本窗口 20 条中，ParticleCore_B F3/F4 属 @lead，故实际认领 18 条。

约定：每条修完在此追加一行（提交号 + 验证命令 + 结果 + 反证方式）。构建目录与 @lead 共用
`audit_build/fork`，故**先确认没有别的会话正在跑 ctest/构建**再增量构建（见下方"资源互斥"）。

## 资源互斥记录

- 2026-11-15T18:24 观测到 `ctest --test-dir audit_build/fork -R implicit|mass_matrices -j 6`（@lead）在跑，
  本会话先做只读分析，等其结束后再构建。
F5 (ParticleCore_A) invalid: 守卫**已存在**于 `PhysicalParticleContainer::BackwardCompatibility()`
（`Source/Plugins/ParticleCore/Particles/PhysicalParticleContainer.cpp:476-492`：momentum_distribution_type=maxwell_juttner 时对
theta / theta_distribution_type / theta_function(x,y,z) 全部 abort 并给出 temperature_in_eV 迁移提示），且由初始快照 7bcb44a 带入。
证据：`audit_build/fork/bin/warpx.3d audit_runs/lead-scratch/theta_legacy.inp`（maxwell_juttner + theta=1.0）→
`<species>.theta is no longer supported. Maxwell-Juttner thermal spread is now specified with temperature_in_eV...`。
上游 26.10 的 theta 也只在 maxwell_juttner 分支解析（`_upstream/warpX-26.10/Source/Initialization/TemperatureProperties.cpp:34-62`），覆盖完整。
