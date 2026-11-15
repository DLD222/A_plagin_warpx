# Python 侧审查（`Python/` + `Examples/**/*.py`）

审计对象：fork `$HOME/WarpX_plugin`，参照 `_upstream/warpX-26.10`。
只读审计；所有结论均可用文中命令复现。运行时复现用的是本机已安装的 pywarpx
（`$HOME/miniconda3/lib/python3.14/site-packages/pywarpx/`，
经 `diff -q` 确认与本仓 `Python/pywarpx/*.py` 逐字节相同）＋ fork 构建的
`audit_build/fork/lib`（`LD_LIBRARY_PATH`）。

## 覆盖

差异清单：
`diff -rq Python _upstream/warpX-26.10/Python` → 4 个文件差异 + 1 个新增目录；
`diff -rq Examples _upstream/warpX-26.10/Examples | grep '\.py'` → 121 个 `.py` 差异 + 3 个新增 `.py`。

| 文件 | 行数 | 类型 | 结论 |
|---|---|---|---|
| `Python/pywarpx/picmi.py` | 5625 | modified（pydantic 全量重写，适配 picmistandard 0.34→0.35） | 可疑（见 F6/F7/复核项） |
| `Python/pywarpx/particle_containers.py` | 840 | modified（删除 `deposit_charge_density`；3 处旧类名残留） | **缺陷 F1/F2** |
| `Python/pywarpx/WarpX.py` | 304 | modified（`_amrex_argv`、`FILE` 正则） | OK（已实测，见 F8/复核项） |
| `Python/setup.py` | 84 | modified（`python_requires>=3.11`） | OK（策略变更，见 F9） |
| `Python/pywarpx.egg-info/` | – | new（构建产物入库） | 可疑（卫生，F9） |
| `Examples/Tests/*/analysis_default_regression.py` ×103 | – | modified（纯机械：`_restart` + `_segmented` 后缀剥离） | OK（但见复核项 R1） |
| `Examples/Physics_applications/capacitive_discharge/inputs_base_1d_picmi.py` | 521 | modified（自定义 solver 迁移到 pydantic） | OK |
| `Examples/Physics_applications/capacitive_discharge/inputs_test_2d_background_mcc_picmi.py` | 362 | modified（同上） | OK |
| `Examples/Tests/collision/analysis_collision_3d_pulsed_decay.py` | 86 | modified（新增 3 组断言） | **缺陷 F4** |
| `Examples/Tests/collision/analysis_test_2d_collisions_split_momentum_push.py` | 148 | modified（容差放宽） | **缺陷 F3** |
| `Examples/Tests/electrostatic_sphere_eb/analysis.py` | 57 | modified（新增 `--last-potential`） | OK（CMake 传 `--last-potential 2`，一致） |
| `Examples/Tests/electrostatic_sphere_eb/inputs_test_3d_electrostatic_sphere_eb_picmi.py` | 125 | modified（EB 势改为 `if(t>0.,2.,0.)`） | OK |
| `Examples/Tests/embedded_boundary_cube/analysis_fields.py` | 113 | modified（仅打印/结构） | OK |
| `Examples/Tests/field_probe/analysis.py` | 61 | modified（容差放宽 2.5%→5%） | **缺陷 F3** |
| `Examples/Tests/implicit/analysis_1d.py`, `analysis_2d_psatd.py` | 35/30 | modified（argparse） | OK |
| `Examples/Tests/implicit/analysis_implicit_jfnk.py` | 106 | modified（容差改命令行，默认值不变 5/10；CMake 传 5/3 更紧） | OK |
| `Examples/Tests/implicit/analysis_pinch.py` | 118 | modified（容差放宽 + 半隐式跳过能量守恒） | **缺陷 F3** |
| `Examples/Tests/implicit/analysis_planar_pinch_solvers.py` | 72 | **new** | 可疑（F3，容差 5e-6） |
| `Examples/Tests/implicit/inputs_test_2d_theta_implicit_jfnk_vandb_picmi.py` | 182 | modified（`ReducedDiagnostic(path=...)`） | OK |
| `Examples/Tests/initial_distribution/analysis.py` | 606 | modified（新增 h12/h13/h14 组，复用同一 tolerance） | OK |
| `Examples/Tests/initial_distribution/inputs_test_3d_initial_distribution_prepare.py` | 119 | modified（写 `temperature_in_eV` 标量 mesh） | OK |
| `Examples/Tests/magnetized_plasma_modes/inputs_test_em_modes_picmi.py` | 576 | modified（`DummyES_Solver` 改 pydantic） | OK |
| `Examples/Tests/ohm_solver_restart/analysis_adaptive_substeps.py` | 53 | **new** | OK |
| `Examples/Tests/ohm_solver_restart/inputs_test_2d_ohm_solver_external_field_segmented_picmi.py` | 109 | **new** | OK |
| `Examples/Tests/projection_div_cleaner/inputs_test_3d_projection_div_cleaner_picmi.py` | 137 | modified（容差放宽） | **缺陷 F3** |
| `Examples/Tests/projection_div_cleaner/inputs_test_3d_projection_div_cleaner_callback_picmi.py` | 247 | modified（容差放宽） | **缺陷 F3** |

合计覆盖：4 个 `Python/` 文件 + 121 个修改的 `Examples/*.py` + 3 个新增 `Examples/*.py` = **128 个文件**。

---

## 缺陷清单

### F1 `pywarpx` 仍调用已改名的旧迭代器类 `WarpXParIter` → 运行时 `AttributeError`
- 文件:行
  - `Python/pywarpx/particle_containers.py:166`（`get_particle_real_arrays`）
  - `Python/pywarpx/particle_containers.py:211`（`get_particle_int_arrays`）
  - `Python/pywarpx/particle_containers.py:250`（`get_particle_idcpu_arrays`）
- 严重度: **critical**
- 置信度: confirmed
- 现象与依据:
  - C++ 侧只注册了新名字：`Source/Plugins/PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp:13-19`
    `py::class_<ParticleTileIter, ...>(m, "ParticleTileIter")`，声明在
    `Source/Plugins/PicEm/Modules/Python/pyWarpX.cpp:43,70`（`init_ParticleTileIter`）；
    上游注册名是 `"WarpXParIter"`（`_upstream/warpX-26.10/Source/Python/Particles/WarpXParticleContainer.cpp`）。
  - Python 侧三处仍写 `libwarpx.libwarpx_so.WarpXParIter(...)`，`grep -rn ParticleTileIter Python/` 为 0 命中。
  - 编译产物字符串核对：`warpx_pybind_3d*.so` 内 `ParticleTileIter` 出现 745 次、`WarpXParIter` 0 次。
  - 实测复现（1D 输入，`sim.initialize_warpx()` 之后）：
    ```
    hasattr(libwarpx.libwarpx_so, "WarpXParIter")      -> False
    hasattr(libwarpx.libwarpx_so, "ParticleTileIter")  -> True
    ParticleContainerWrapper("electrons").get_particle_real_arrays("w", 0)
      AttributeError: module 'pywarpx.warpx_pybind_1d' has no attribute 'WarpXParIter'
      (particle_containers.py:166)
    ParticleContainerWrapper("electrons").get_particle_int_arrays("i1", 0)
      AttributeError: ... (particle_containers.py:211)
    ```
  - 影响面：`get_particle_x/y/z/r/theta/w/ux/uy/uz/id/cpu`（`particle_containers.py:337`–`:559`）全部经由这三个函数，
    即 `ParticleContainerWrapper` 的粒子数据访问 API 整体失效；`get_particle_boundary_buffer`
    走 `BoundaryBufferParIter`（该名未改：`particle_containers.py:723,731` + `Particles/ParticleBoundaryBuffer.cpp:26-28`）所以仍可用。
  - 同一 C++ 分区（`reviews/Source_Plugins_PicEm_B.md` F3）也从绑定侧报过同一问题；这里是
    **可修复的 Python 侧 3 行**，且仓库内 `Examples/` 没有用例覆盖它（`grep` 无
    `get_particle_x`/`get_particle_real_arrays` 调用），CI 不会发现。
- 建议修法（最小改动，二选一）:
  - (a) `py::class_(..., "ParticleTileIter")` 改为 `"WarpXParIter"`（保留 C++ 类型名），零 Python 改动、向后兼容最好；
  - (b) Python 侧 3 处改为 `ParticleTileIter`（并同步 `Tools/Modularization/check_core_purity.py:24` 的字符串与文档；若选 (b)，建议旧名保留一个别名类以便用户脚本过渡）。
- 验证方法:
  ```bash
  python -c "import pywarpx,libwarpx; from pywarpx.Geometry import geometry; geometry.dims='1'; print(hasattr(libwarpx.libwarpx_so,'WarpXParIter'))"
  # 或直接跑上面的 1D 复现脚本调用 get_particle_real_arrays('w',0)
  ```

### F2 公开 API `ParticleContainerWrapper.deposit_charge_density` 被删（连同 C++ 绑定）
- 文件:行 `Python/pywarpx/particle_containers.py:638`（上游方法在该位置；fork 删除后 `:638` 直接就是
  `class ParticleBoundaryBufferWrapper`）
  对应 C++ 绑定缺失：上游 `_upstream/warpX-26.10/Source/Python/WarpX.cpp:223`
  `.def("apply_inverse_volume_scaling_to_charge_density", ...)`，fork 中 `grep -rn apply_inverse_volume_scaling_to_charge_density Source/` 为 0 命中。
- 严重度: major（用户可见 API 静默消失，无 deprecation 过渡、无 Docs/CHANGELOG 说明）
- 置信度: confirmed
- 现象与依据:
  - 上游 26.10 的 `deposit_charge_density(level, clear_rho, sync_rho)` 会 `fields.get("rho_fp")` →
    `particle_container.deposit_charge` → RZ 下 `apply_inverse_volume_scaling_to_charge_density` → `sync_rho`。
    fork 保留了 `sync_rho`（`warpx_pybind_1d.so` 内含该符号，`WarpX.cpp:213`）和 `deposit_charge`
    （`Particles/WarpXParticleContainer.cpp:113`），只删了反体积缩放这一环，于是整个 Python 方法被删。
  - 实测：`hasattr(libwarpx.warpx, 'apply_inverse_volume_scaling_to_charge_density') == False`；
    `'deposit_charge_density' in dir(ParticleContainerWrapper) == False`。
  - 任何调用 `sim.particles.get("x")`/wrapper 的 `deposit_charge_density(...)` 的用户脚本会在升级后直接
    `AttributeError`（不是 `TypeError`，不会被 try/except 宽泛捕获以外的逻辑发现）。
  - 语义上这不是"改名兼容"——RZ 下若要复现"把电荷沉积到 rho_fp 并按 2πr 体积反缩放"，用户已无公开路径。
- 建议修法: 恢复 C++ `.def("apply_inverse_volume_scaling_to_charge_density", ...)` 与 Python 方法；
  若确要移除，至少在 `Docs/source/usage/` 与 CHANGELOG 标注并提供一个 `DeprecationWarning` 兼容实现
  （内部可用 `mf.multFabs()` / `warpx.apply_inverse_volume_scaling_...` 的替代实现）。
- 验证方法: `python -c "from pywarpx.particle_containers import ParticleContainerWrapper as P; print(hasattr(P,'deposit_charge_density'))"`（fork=False，上游=True）。

### F3 测试容差被系统性放宽（6 处）

> **复核结论（round 25）：不成立（以 26.11 为基准）**。逐个与
> `_upstream/warpX-26.11/Examples/**/analysis*.py` 比对，列出的 6 处**全部与上游 26.11 完全一致**
> （例如 `inputs_test_3d_projection_div_cleaner_picmi.py:133` 上游也是 `tolerance = 2e-12`，
> `analysis_pinch.py:72/115` 上游也是 `3.0e-12 / 2.2e-12`，`field_probe/analysis.py:61` 上游也是
> `averror < 5.0`）。这些是上游自己在 26.10→26.11 之间调整的数值。
>
> 系统性复查的全部结果：`Examples` 下只有 **2 个** `analysis*.py` 与上游 26.11 不同，都是本项目
> 修改的（`analysis_collision_3d_pulsed_decay.py`、`analysis_planar_pinch_solvers.py`）。
> 复审中真正找到的"凑出来的容差"是后者：`tolerance_rel_charge = 5.0e-6` 而实测残差 1.08e-15
> （姊妹脚本对同一量要求 2.2e-12），已收紧到 `1.0e-12`。

- 严重度: major（涉及投影散度清理器这一"机器精度"级测试，是该类物理正确性的核心门限）
- 置信度: confirmed（数值改动本身）；对"是否掩盖了精度回归"为 likely（未跑 ctest）
- 清单（左=上游，右=fork）:

| 文件:行 | 量 | 上游 | fork | 倍数 |
|---|---|---|---|---|
| `Examples/Tests/projection_div_cleaner/inputs_test_3d_projection_div_cleaner_picmi.py:133` | `tolerance`（‖divB‖₂） | 1e-12 | **2e-12** | ×2 |
| `Examples/Tests/projection_div_cleaner/inputs_test_3d_projection_div_cleaner_callback_picmi.py:243` | `tolerance` | 1e-12 | **3e-12** | ×3 |
| `Examples/Tests/implicit/analysis_pinch.py:72` | `rel_net_energy_tol`（能量守恒） | 1.0e-12 | **3.0e-12** | ×3 |
| `Examples/Tests/implicit/analysis_pinch.py:115` | `tolerance_rel_charge`（高斯定律/电荷守恒） | 1.0e-12 | **2.2e-12** | ×2.2 |
| `Examples/Tests/field_probe/analysis.py:61` | `averror`（场探针平均误差 %） | 2.5 | **5.0** | ×2 |
| `Examples/Tests/collision/analysis_test_2d_collisions_split_momentum_push.py:121` | `relative_tolerance`（静电分支） | 1.1e-1 | **2e-1** | ×1.8 |

  另有 1 处"新增文件即用松容差"：`Examples/Tests/implicit/analysis_planar_pinch_solvers.py:69`
  `tolerance_rel_charge = 5.0e-6`——同一物理量在姊妹脚本 `analysis_pinch.py` 要求 2.2e-12，
  相差 2×10⁶ 倍，等于新测试对电荷守恒几乎不做检查。
- 现象与依据:
  - 就 `projection_div_cleaner` 而言，fork 的 C++ 实现只是模块化搬移，**没有数值改动**：
    `diff -u _upstream/.../Initialization/DivCleaner/ProjectionDivCleaner.cpp
    Source/Plugins/PicEm/Modules/Initialization/DivCleaner/ProjectionDivCleaner.cpp` 的语义差异仅
    `WarpX::GetInstance()` → `warpx::runtime::RuntimeData`（`data()`），以及 include 路径，
    MLMG 容差/迭代设置未变。因此"容差 1e-12→2e-12/3e-12"没有任何算法层面的正当理由，
    只能解释为"新实现残差比上游大 2–3 倍"后的凑数式放宽。
  - `analysis_pinch.py` 同时新增 `if "semi_implicit" not in test_name:` 整段跳过能量守恒断言
    （`:70-73`），以及 `newton_iters_tol` 对 `*_substeps` 用例从 6.0 提到 9.0（`:88`，可辩护，
    因为 substeps 把所有子步迭代累加），但前者是直接把守恒性检查关掉。
  - 未发现 CMake 层 checksum `--rtol` 被放宽：`diff` 全部 CMakeLists 后只有
    `--gmres-iters-tol 4/5`、`--newton-iters-tol 4/3` 两处，且都**比**原默认 5.0/6.0 更严。
- 建议修法: 恢复上游数值；若新实现确实残差变大，应作为独立缺陷定位（例如比较
  projection cleaner 的 MLMG 相对/绝对容差、`num_final_sweeps`、以及 cleaner 作用前后的
  ‖divB‖），而不是放宽判据。`analysis_pinch.py` 的 semi-implicit 分支应改为单独（且明确的）容差，
  不要整体 `if` 跳过。
- 验证方法:
  ```bash
  ctest --test-dir audit_build/fork -R projection_div_cleaner --output-on-failure
  # 观察报告里的 error 值：若落在 1e-12..3e-12 区间，说明确实发生了精度回归
  git -C ... diff -- 无意义（本仓非 git），直接 diff 上述文件
  ```

### F4 新增碰撞分析断言有复制粘贴错误 + 精确浮点相等
- 文件:行 `Examples/Tests/collision/analysis_collision_3d_pulsed_decay.py:58,66,76,80`
- 严重度: minor
- 置信度: confirmed（代码事实）/ likely（是否引起假通过）
- 现象与依据:
  - `:66` 中性粒子权重误差用 `nI_arr[-1]`（0D **电离份额**，≈0.78·n10）作分母，而分子是
    `n1_arr[-1]`（0D **中性**密度，≈0.22·n10），与打印出来的 `neu weight (0D) = n1_arr[-1]` 自相矛盾；
    与 `:50`（离子）用 `nI_arr` 作分母的写法不一致。数值上把判据放宽了 1/0.78≈1.3 倍。
  - `:58` 电子权重以 `nI_arr[-1]` 为 0D 参考（物理上电子权重=离子权重，尚可接受），但同一行既作参考
    又作分母，若 `ele_weight` 与 `nI` 的量纲/归一化不同则误差量纲不成立。
  - `:76`/`:80` 用 `==` 精确比较浮点：`assert ion_number_end == weight_ratio * neu_number_change`
    （`weight_ratio = 2` 硬编码），依赖 `particle_number.txt` 的文本浮点解析恰好精确；不同平台/精度下
    易假失败。
- 建议修法: 中性项分母改 `n1_arr[-1]`（或统一用一个明确参考量）；精确相等改
  `np.allclose(..., rtol=1e-12)`；`weight_ratio` 从输入文件读取而不是硬编码。
- 验证方法: `ctest -R collision_3d_pulsed_decay`；或直接看 `neu weight error` 的打印值，
  它应比真实相对误差小约 22%。

### F5 `read_dims_from_file` 的 `FILE` 正则仍会把 `#` 吞进路径
- 文件:行 `Python/pywarpx/WarpX.py:113-115`、`:128`
- 严重度: minor
- 置信度: confirmed
- 现象与依据: 新增正则 `r'^\s*FILE\s*=\s*(?:"([^"\n\r]+)"|([^\s"#]\S*))'` 的第二分支首字符排除
  `#`，但随后的 `\S*` 又把 `#` 收回来。实测：
  ```
  a.in:  FILE = inc/base.in   # comment      -> dims 3   （OK）
  b.in:  FILE = inc/base.in#comment           -> 打开 "inc/base.in#comment" 失败，返回 None
  c.in:  FILE = "inc/base.in"                 -> dims 3   （OK）
  ```
  `b` 情况下 `read_dims_from_file` 打印 "Could not open file ..." 并返回 `None`，随后
  `WarpX.load_inputs_file` 抛 "Could not find the geometry.dims in the input file"。注释里写
  "a comment can follow"，但只在 `#` 前有空白时成立。上游正则同样有此问题（`([^"\n\r]+)` + strip），
  属**未修好**而非新引入；注释与实现不符。
- 建议修法: `r'^\s*FILE\s*=\s*(?:"([^"\n\r]+)"|([^\s"#]+))'`（第二分支不允许 `#`），或先按
  `#` 截断注释再匹配。
- 验证方法: 见上面三条 `a.in/b.in/c.in`。

### F6 `Simulation` 的若干公开属性/扩展点被改为私有（无 property 兼容）
- 文件:行 `Python/pywarpx/picmi.py`：`_species_type`/`_element`/`_species_number` 声明在
  `:310-312`（赋值 `:379`）；`_laser_number` 声明 `:2651`、`:2689`；`_mangle_dict` 多处（首处 `:776`）；
  `_inputs_initialized`/`_warpx_initialized`/`_finalized` 声明 `:3916-3918`
- 严重度: minor
- 置信度: confirmed（属性确实不存在了）/ needs-verification（外部用户依赖程度）
- 现象与依据: 上游 `Species` 暴露 `species_number`、`species_type`、`element`；`GaussianLaser`/
  `AnalyticLaser` 暴露 `laser_number`；各 mixin 暴露 `mangle_dict`；`Simulation` 暴露
  `inputs_initialized`/`warpx_initialized`/`finalized`。fork 改成 `PrivateAttr`，其中
  `species`、`laser`、`diagnostic` 保留了同名 `@property`（`picmi.py:316`、`:2654`/`:2693`、`:4239`），但
  `species_number`/`species_type`/`element`/`laser_number`/`mangle_dict`/`*_initialized`/`finalized`
  **没有**兼容 property；pydantic 对未声明属性抛 `AttributeError`，不是静默降级。
  仓库内自查：`grep -rn "\.species_number\|\.species_type\|\.laser_number\|\.mangle_dict\|\.inputs_initialized\|\.warpx_initialized\|\.finalized" Examples/ Tools/ Docs/` 只命中输入文件里的
  `<species>.species_type` 文本（非 Python 属性），所以本仓 CI 不会暴露，但第三方脚本会。
- 建议修法: 为上述 5–7 个名字加只读 `@property`（对 `mangle_dict` 提供 setter 转发到 `_mangle_dict`）。
- 验证方法: `python -c "from pywarpx import picmi; s=picmi.Species(particle_type='proton'); print(s.species_type)"`（fork→AttributeError）。

### F7 所有 PICMI 对象改为 pydantic 严格校验，未使用的多余关键字从"忽略"变"报错"
- 文件:行 `Python/pywarpx/picmi.py:5089-5215`（`ReducedDiagnostic._type_inputs` 表）、
  `:5359-5367`（`_parameter_names` 覆写）、`:5372-5411`（`_check_parameters_of_type`）、
  `:19-26`（`ConfigDict`/`Field` 引入）
- 严重度: minor
- 置信度: confirmed
- 现象与依据: `ParticleHistogram` 等表达式诊断历史上允许把任意关键字当"表达式常量"传入，未被引用的
  会被忽略；fork 下未被引用即触发 `pydantic_core.ValidationError: Extra inputs are not permitted`。
  实测 `picmi.ReducedDiagnostic(diag_type="ParticleHistogram", ..., histogram_function="ux",
  warpx_custom_const=3.0)` → `ValidationError: warpx_custom_const Extra inputs are not permitted`。
  这一"更严格"本身是好事（拼错参数不再静默失效），但属用户可见的兼容性变化，且
  `_parameter_names` 覆写（`:5357-5367`）依赖 `picmistandard` 0.35 的内部钩子
  （`PICMI_ExpressionParameters._parameter_names`，0.35 新增，已确认存在），与上游锁定的 0.34 不兼容。
- 建议修法: 在升级说明里明确"未使用关键字现在会报错"；`ReducedDiagnostic` 建议对
  `_expression_fields` 之外的关键字给出指向具体字段名的错误信息。
- 验证方法: 上面一行构造即触发。

### F8 `_amrex_argv`：脚本自身的 `--flag` 若出现在第一个覆盖参数之后会破坏 ParmParse
- 文件:行 `Python/pywarpx/WarpX.py:147-189`，拼接返回在 `:189`
- 严重度: minor
- 置信度: confirmed（行为已实测）/ needs-verification（真实调用方式是否触发）
- 现象与依据: 设计是"第一个 `name=value` 之前的脚本参数全部丢弃"，实测符合预期：
  ```
  ['run.py','my.in']                 -> ['FILE = "my.in"']
  ['run.py','-f','kernel.json']      -> ['FILE = "my.in"']          # Jupyter kernel
  ['run.py','max_step','= 10']       -> ['FILE = "my.in"', 'max_step', '= 10']
  ['run.py','--','my.in','max_step=5'] -> ['FILE = "my.in"','--','my.in','max_step=5']
  ```
  我也用 AMReX 26.10 实际跑通了 `FILE = "..."` 这条包含路径（`from pywarpx import warpx; warpx.load_inputs_file("cf1d.in")`
  得到 `dt = 2.0827e-16`，正是 `(1e-6/16)/c`，说明 inputs 文件确实生效）。但若用户把脚本自己的开关
  写在覆盖参数之后（`python run.py max_step=10 --verbose`），该 flag 会被当成 AMReX 定义，
  粘到上一个定义的取值里，触发 `has multiple values` 类中止。docstring 有说明，但错误信息会出现在
  C++ 层、与 Python 参数位置难对应。
- 建议修法: 在 `_amrex_argv` 中对 `args[first:]` 里以 `-` 开头且不含 `=` 的 token 直接
  `raise SystemExit("script options must come before overrides or after '--'")`，把问题前移到 Python 层。
- 验证方法: 上面的 `cases` 脚本 + 一个真实的 `python inputs.py max_step=10 --verbose`。

### F9 打包/卫生问题
- 文件:行 `Python/setup.py:82`；`Python/pywarpx.egg-info/`（新增于 `Python/`）
- 严重度: minor
- 置信度: confirmed
- 现象与依据:
  - `python_requires` 从上游 `">=3.8"`（注释：CI 用，实际 `>=3.9`）改为 `">=3.11"`；与代码实际需求一致
    （用到 `typing.Self`，3.11+），但这是对下游发行版（conda/spack 仍可能用 3.9/3.10）的硬性门槛变更，
    应在 release note 中说明，而不是跟随一次 picmi 重构静默提高。
  - `dependencies.json` 的 `version_picmi` 0.34.0 → 0.35.0，且 `commit_picmi` 从发布号改成一个
    git SHA（`89b27cee...`）。`setup.py` 仍按 `picmistandard==0.35.0` 安装，即"按 commit 开发、按发布号
    安装"，两者可能不是同一份代码；建议改回 tag 或把 SHA 同时写进 `install_requires`。
  - `Python/pywarpx.egg-info/`（`PKG-INFO`/`SOURCES.txt` 等）被提交进源码树，属构建产物污染；
    `MANIFEST.in`/`.gitignore` 应排除。
- 建议修法: 上述三点分别处理；`python_requires` 建议先与 CI 矩阵对齐再改。
- 验证方法: `ls Python/pywarpx.egg-info`；`grep -n python_requires Python/setup.py`。

---

## 不确定但值得复核的点

- **R1（`_segmented` 机械改动）**：103 个 `analysis_default_regression.py` 把 `test_name` 里的
  `_segmented` 剥掉，即新的分段运行用例直接复用**基用例**的 benchmark JSON。方向上是加严（分段必须
  逐位复现连续运行），但 `test_name.replace(suffix, "")` 是子串替换：若将来出现
  `foo_segmented_bar` 这类目录名会误映射到 `foo_bar`，建议改成 `removesuffix`。
- **R2（`Species.charge/mass` 放宽为 `float | str`）**：`picmi.py:133-141`。WarpX 侧确实支持
  `"q_e"`、`"2*m_e"` 这类表达式，但 PICMI 标准之外的字符串会让 `mass`/`charge` 变成需要 parser
  求值的输入；若表达式里出现未定义常量，错误只在 C++ 初始化期才出现。值得确认 `warpx.used_inputs_file`
  里这些字段的引号/转义是否正确（我实测生成的 deck 中 `electrons.reflection_model_xlo(E) = "0.5*v"`
  正常，但未覆盖 charge/mass 表达式）。
- **R3（网格类把 `prob_lo/prob_hi` 从 `model_post_init` 推迟到 `grid_initialize_inputs`）**：
  `picmi.py:1117`、`:1230`、`:1349`、`:1487`（各 Grid 的 `grid_initialize_inputs`）。注释理由是"bounds 在 validation 之后才填好、且之后还会变"。风险：
  任何在 `initialize_inputs()` 之前读取 `geometry.prob_lo` 的代码（包括用户脚本和
  `Diagnostic` 的 `lower_bound` 缺省推导）会看到 `None`。仓库内用例没暴露，建议加一条
  "构造 Grid 后立刻读 `pywarpx.geometry.prob_lo`"的断言。
- **R4（`CylindricalGrid`/`Cartesian*DGrid` 里的 RZ 断言被删）**：fork 删掉了
  `assert lower_bound[0] >= 0` 与"radial 边界不能 periodic"（上游 `picmi.py:1080-1085`）。
  我实测这两条现在由 **picmistandard 0.35 自己**的 validator 提供
  （"The lower radial bound must be >= 0"、"The radial boundaries cannot be periodic"），
  所以**不是缺陷**，但删除理由应写进注释，否则后续升级 picmistandard 时会失去保护。
- **R5（`picmi.Simulation.collisions` 变为 `list[CollisionBase]`）**：与上游
  `for collision in self.collisions` 的用法一致（上游也要求 list），实测评测 `warpx_collisions=[...]`
  正常生成 `collisions.collision_names = c1`。但**单对象**传入现在给出 pydantic `ValidationError`
  而不是"不可迭代"的 `TypeError`，报错信息可读性变差。
- **R6（`BinomialSmoother` 默认补偿开关）**：`compensation=None` → `np.all(None) is False` →
  `warpx.use_filter_compensation=0`。与上游一致（picmistandard 默认也是 None），**不是 fork 回归**，
  但"默认不开补偿"值得与 C++ 的 `use_filter_compensation` 缺省值核对（若 C++ 缺省是 1，则 PICMI
  路径与输入文件路径行为不同）。实测 deck：`warpx.use_filter = 1`、`use_filter_compensation = 0`。
- **R7（`FieldProbe` 的 `probe_geometry` 校验从 `AttributeError`/`RuntimeError` 变为 `ValueError`）**：
  用户脚本里 `except AttributeError` 的旧写法会漏接；属错误类型变化，需在升级说明中列出。
- **R8（CI 覆盖面）**：本分区最严重的 F1 在 `Examples/` 中**无任何用例调用**，F2 亦无。
  建议加一个最小 PICMI 用例，用 `sim.particles.get(name)` + `get_particle_real_arrays` 读回
  一个自定义 real attribute，作为 pywarpx↔C++ 绑定名的回归测试。
