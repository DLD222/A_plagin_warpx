# Source_Plugins_Star 工作日志

claim: Source_Plugins_Star F11 @lead-tools 2026-11-15T18:30
F11 fixed by HASH(F11); 默认 axis_dir 改为按维度取 (AMREX_SPACEDIM>1?1:0)（三处默认值共用 angular_momentum_defaults::axis_dir），assert 增加下界检查；1D 下 applyCentrifugalSource/balanced_gravity 明确 no-op 返回（1D 无垂直于轴的径向，半径无定义）。验证: 3D fluid_angular_momentum_3 / fluid_stellar_model_3 均 Passed；1D 用构建自带的 flags 做 -fsyntax-only 编译通过，并加 static_assert(default axis_dir < AMREX_SPACEDIM)；反证: 把默认值改回 1 → 1D 静态断言失败（正是原缺陷），已还原。未做: 力仍只加在单一分量（改它会改动已验证的积分输出，需单独回归），已在 docstring 注明
