claim: Source_Plugins_ParticleCore_B F3,F4 + Source_Core_Runtime_B F6 @lead 2026-11-15T18:30

F3 fixed (frame() 不再用一次性 static 快照，改走 live 提供者 warpx::runtime::legacy_frame())；
   同时删掉 WarpXParticleContainer.cpp 里重复的 LegacyServices.H include。
   验证: 构建通过；ctest -R 'implicit|mass_matrices' 51/51、ctest -R 'runtime_snapshot|plugin_|fluid_' 42/42。
F4 fixed (full_mass_matrices 判据恢复为 Szz->nComp() > 1，与上游一致)。
   可达性: use_mass_matrices_pc=true 且 jacobian=false 时 nComp>1，旧判据会走窄 stencil。
   验证: ctest -R 'implicit|mass_matrices' 51/51。

附带发现（构建期暴露的潜伏缺陷，均非 F3/F4 引起，但在重建这些 TU 时才显形）：
- ParticleCreationFunc.H 使用 ParticleCreation::DefaultInitializeRuntimeAttributes 却没 include
  DefaultInitialization.H（靠其它头文件的传递包含才编过）。
- MomentFluidSolver.cpp 使用 WARPX_ABORT_WITH_MESSAGE 却没 include Utils/TextMsg.H。
两者已补 include。

Runtime_B F6: SpeciesHandle::numberOfParticles 改名为 registeredNumberOfParticles，注释写明
它是"注册时刻的快照、不是实时数量"（保留它是因为 tests/runtime/test_runtime_snapshot.cpp 在用）。
