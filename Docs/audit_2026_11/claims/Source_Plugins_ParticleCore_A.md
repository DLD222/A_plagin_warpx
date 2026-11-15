# Source_Plugins_ParticleCore_A 工作日志

claim: Source_Plugins_ParticleCore_A F1,F2,F3,F4,F5 @lead-mid 2026-11-15T18:30

- F1 `GetExternalEBField` 缺 `RuntimeContext`：**已修复（此前提交，不在本轮）**。
  `Source/Plugins/ParticleCore/Particles/ElementaryProcess/{Ionization,QEDPairGeneration,QEDPhotonEmission}.cpp`
  的构造已带 `a_ctx` 并传给 `GetExternalEBField(a_pti, a_offset, a_ctx)`；
  `GetExternalFields.cpp` 的 parser 分支也已加 `WARPX_ALWAYS_ASSERT_WITH_MESSAGE(ps != nullptr, ...)`
  （提交 `5d71c45`）。本条目复核为已闭合。
- F2 `Filter::ApplyStencil` 把 load-balance cost 恒置 `nullptr`：确认成立。上游是
  `WarpX::getCosts(lev)`；fork 里 `Filter.cpp` 已经 include 了 `Runtime/LegacyServices.H`，
  而 `warpx::runtime::legacy_costs(lev)` 正是内核注册的 `WarpX::getCosts(lev)` 代理，
  未注册（单测）时返回 nullptr。两处（GPU / OMP 分支）都改成
  `amrex::LayoutData<amrex::Real>* cost = warpx::runtime::legacy_costs(lev);`
  ⇒ 计时统计恢复，"没有内核"的路径仍是 nullptr（不再有死分支）。
- F3 `read_fields_from_path` 由 `query` 变 `get`：**invalid**。26.11 的
  `Source/Initialization/ExternalField.cpp:210` 同样是 `pp_warpx.get("read_fields_from_path", ...)`
  ⇒ 上游演化。
- F4 eV 温度的 `mass`/`species_type` 解析绕开 `source_name`：**invalid**。26.11
  `Source/Initialization/TemperatureProperties.cpp:36-52` 是同一段代码
  （`pp.query("species_type", ...)`、`utils::parser::queryWithParser(pp, "mass", mass)`）。
- F5 `theta` 输入删除无兼容保护：**invalid**。26.11 的 TemperatureProperties 已经没有
  `theta`/`theta_distribution_type`，只认 `temperature_in_eV`（`parameters.rst` 同步改了）
  ⇒ 上游演化。

验证：见"验证记录"。
