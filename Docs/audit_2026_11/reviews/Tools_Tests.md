# Tools/ 与 tests/ 审查

审查范围：本 fork 新增/修改的 `Tools/**` 与 `tests/**`（对照 `_upstream/warpX-26.10`）。
只读审计，未改动任何源码。除注明外，所有"运行结果"均为本次在本机直接执行的输出。

## 覆盖

| 文件 | 行数 | 类型 | 结论 |
|---|---:|---|---|
| Tools/Modularization/check_core_purity.py | 70 | new | 缺陷（F5） |
| Tools/Modularization/check_plugin_boundaries.py | 171 | new | 缺陷（F1，critical） |
| Tools/Modularization/check_kernel_coupling.py | 82 | new | 缺陷（F3） |
| Tools/Modularization/check_build_boundaries.py | 89 | new | 缺陷（F16） |
| Tools/Modularization/check_legacy_coverage.py | 34 | new | 缺陷（F4） |
| Tools/Modularization/check_field_solver_extraction.py | 30 | new | 缺陷（F26） |
| Tools/Modularization/inventory.py | 24 | new | OK（仅报告） |
| Tools/Modularization/inventory_field_solver.py | 106 | new | 可疑（F33） |
| Tools/Modularization/{core_concrete,kernel_coupling}_allowlist.txt | 2/2 | new | 空清单（见 F3/F5） |
| Tools/Modularization/plugin_boundary_allowlist.txt | 14 | new | 可疑（声称的 10 条已修，见 F5/F1） |
| Tools/Modularization/plugin_dependencies.txt | 11 | new | 缺陷（F1：cycles 不被检测） |
| Tools/verify/check_macroscopic_cavity_mode.py | 19 | new | 缺陷（F8，无断言） |
| Tools/verify/check_macroscopic_cavity_mode_2d.py | 24 | new | 缺陷（F8，无断言） |
| Tools/verify/check_macroscopic_2d_normal.sh | 22 | new | 缺陷（F8） |
| Tools/verify/check_macroscopic_subpixel_step.sh | 28 | new | 缺陷（F8/F23） |
| Tools/verify/check_macroscopic_periodic_interface.sh | 23 | new | 缺陷（F23） |
| Tools/verify/check_macroscopic_vacuum_limit.sh | 52 | new | 可疑（F23） |
| Tools/verify/check_star_runtime_plugin.sh | 33 | new | 缺陷（F10） |
| Tools/verify/check_star_runtime_plugins.sh | 49 | new | 可疑（F23） |
| Tools/verify/parse_tests.py | 152 | new | 缺陷（F2） |
| Tools/verify/classify_ctest_log.py | 143 | new | 缺陷（F25） |
| Tools/verify/compare_particles_openpmd.py | 106 | new | OK（有退出码；归一化见 F31） |
| Tools/verify/ab/compare_fields.py | 73 | new | 缺陷（F9） |
| Tools/verify/ab/run_ab.sh | 41 | new | 缺陷（F9） |
| Tools/verify/ab/run_all.sh | 90 | new | 缺陷（F9） |
| Tools/verify/ab/README.md | 50 | new | 文档与实现不符（F9） |
| Tools/verify/build_and_run_4d.sh | 83 | new | 可疑（F30） |
| Tools/verify/build_parallel_hdf5.sh | 36 | new | 可疑（硬编码/usr/bin/mpicc、联网下载） |
| Tools/verify/fix_mpi4py_for_openmpi.sh | 67 | new | 可疑（硬编码 conda 路径、覆盖 site-packages） |
| Tools/verify/install_git_hooks.sh | 23 | new | 缺陷（F6：hook 不含不变量检查器） |
| Tools/verify/run_gates.sh | 32 | new | 缺陷（F6） |
| Tools/verify/README.md / README_particle_compare.md | 55 / 21 | new | OK（记录环境前提，未夸大） |
| Tools/star/eos.py | 358 | new | 缺陷（F12） |
| Tools/star/stellar_structure.py | 704 | new | 缺陷（F13） |
| Tools/star/opal_opacity.py | 183 | new | 缺陷（F11） |
| Tools/star/ssm_compare.py | 220 | new | 可疑（F20） |
| Tools/star/solar_compare_plot.py | 197 | new | 可疑（F21） |
| Tools/star/sod_reference.py | 141 | new | 缺陷（F19） |
| Tools/star/cf88_reference.py | 247 | new | OK（自检实质有效；Q 值自证，见 F31） |
| Tools/star/gravity_reference.py | 186 | new | OK（自检有效，含 Gauss/shell 定理） |
| Tools/star/parse_fgong.py | 91 | new | 可疑（F31） |
| Tools/star/extract_opal_subset.py | 101 | new | 可疑（无 mkdir，见 F31） |
| Tools/star/README.md | 66 | new | 文档与 ctest 一致；"5% per layer" 与断言 15% 不符（F20） |
| Tools/automation/route_tasks.py | 269 | new | 缺陷（F7） |
| Tools/automation/route_tasks.json | 2988 | new | 数据；153 个任务全部 `done`（F7） |
| Tools/automation/gates/compliance.py | 96 | new | OK（实测 PASS；`git ls-files` 失败会抛异常而非静默） |
| Tools/automation/gates/audit_check.py | 54 | new | 可疑（阈值型，见 F31） |
| Tools/automation/gates/build_2d.sh | 23 | new | OK |
| Tools/automation/gates/full_gate.sh | 10 | new | 缺陷（F7：不含 JSON 声明的 `ab` 门） |
| Tools/automation/gates/smoke_2d.sh | 7 | new | 缺陷（F25） |
| Tools/automation/gates/a5_ab.py | 51 | new | 缺陷（F29：未接入任何门） |
| Tools/A0/parse_headers.py | 90 | new | 缺陷（F24） |
| Tools/A0/build_catalog.py | 106 | new | 缺陷（F24） |
| Tools/machines/karolina-it4i/install_dependencies.sh | 64 | modified | OK（仅 picmistandard 0.34→0.35，与 requirements.txt:7 一致） |
| tests/runtime/CMakeLists.txt | 154 | new | 缺陷（F17） |
| tests/runtime/test_runtime_snapshot.cpp | 614 | new | OK（真正的数值断言，容差 1e-12，返回 failures==0） |
| tests/runtime/test_plugin_lifecycle.cpp | 225 | new | 缺陷（F15） |
| tests/runtime/test_plugin_field.cpp | 100 | new | 可疑（checkpoint 探测只看文件名前缀，F31） |
| tests/runtime/test_bus_access.cpp | 110 | new | OK |
| tests/runtime/test_scalar_state.cpp | 152 | new | OK |
| tests/runtime/test_stage_registry.cpp | 135 | new | OK（含 C ABI ordinal 同步检查） |
| tests/runtime/test_stage_dispatch.cpp | 109 | new | OK |
| tests/runtime/test_service_registry.cpp | 73 | new | 可疑（"no diagnostics" 分支恒真，F31） |
| tests/runtime/test_particle_factory.cpp | 71 | new | 可疑（依赖链接器不拉入粒子插件，F31） |
| tests/runtime/test_nonpic_kernel.cpp | 239 | new | 缺陷（F18） |
| tests/runtime/test_{particle,particle_access,collision}_contracts.cpp | 16/13/13 | new | 缺陷（F17：仅编译检查且未注册 CTest） |
| tests/runtime/test_field_solver{,_engine}_interface.cpp | 3/3 | new | 缺陷（F17） |
| tests/runtime/inputs_2d* | 13–52 | new | OK（与断言一致；btd/es/lb/psatd 仅 smoke） |
| tests/plugins/CMakeLists.txt | 81 | new | 缺陷（F22） |
| tests/plugins/*.c, example_plugin.h | 15–105 | new | OK（纯 C，ABI v6，descriptor 正确） |
| tests/plugins/test_c_abi_header.c | 80 | new | OK（NULL 容错与返回值约定一致） |
| tests/plugins/test_plugin_loader*.cpp, test_plugin_example.cpp | 22–46 | new | OK（返回非零） |
| tests/external_plugin/CMakeLists.txt | 57 | new | 缺陷（F22） |
| tests/external_plugin/third_party_plugin.c | 105 | new | 可疑（未初始化 `desc.sub`? 实为显式赋值；见 F31） |
| tests/external_plugin/inputs_external_plugin | 32 | new | OK |
| tests/unit/run_tests.py | 78 | new | 缺陷（F14） |
| tests/unit/test_load_inputs_file.py | 94 | new | 缺陷（F14） |
| tests/unit/test_read_dims_from_file.py | 25 | new | 缺陷（F14 同类：在仓库根写文件） |
| tests/unit/test_charge_deposition.py | 与上游同（+`finalize=True`） | modified | OK（跟随 binding 新签名） |
| tests/unit/test_mass_matrices.py | 与上游同（+`deposit_mass_matrices(dt)`） | modified | OK（跟随 binding 新签名；属 API 破坏，见 F31） |

未用 `grep` 代替阅读的文件：以上每一行都已完整读过；`Tools/automation/route_tasks.json` 为纯数据，只读了结构与统计（153 任务 / 153 done）。

## 缺陷清单

### F1 `check_plugin_boundaries.py` 的两条检查被无条件丢弃（可复现反例）
- 文件:行：`Tools/Modularization/check_plugin_boundaries.py:151`（追加重复头文件错误）、`:159`（追加依赖环错误）、`:161`（`errors = sorted(...)` 覆盖上述两条）。
- 严重度：critical
- 置信度：confirmed
- 现象与依据：第 140–151 行的"Core 与插件同路径头文件 → ODR 风险"检查把错误 append 进 `errors`；第 158–159 行的依赖环检查同样 append；第 161 行把 `errors` 整体重新赋值为 `violations` 的过滤结果，于是这两条检查的结论永远不参与 `sys.exit(1)`。而 `plugin_dependencies.txt:3` 与 `S_SERIES_STATUS.md` 明确写"the checker fails on a cycle"。
  反例（本次实测，合成树 `/tmp/pbt`：`Source/Core/Modules/mod/dup.H` 与 `Source/Plugins/Foo/Modules/mod/dup.H` 同名不同内容，且 `plugin_dependencies.txt` 写 `Foo -> Bar` / `Bar -> Foo`）：
  ```
  $ python3 Tools/Modularization/check_plugin_boundaries.py
  plugin boundaries OK (0 known, 0 new)
  exit=0
  ```
- 建议修法：把 140–151 与 158–159 的错误并入另一个列表（如 `hard_errors`），与 `violations` 的过滤结果合并后再判断。
- 验证方法：如上合成树；或在本仓库临时加入 `Bar -> Foo` 一行（不改源码目录，只改 `Tools/Modularization/plugin_dependencies.txt`），脚本仍返回 0。

### F2 `parse_tests.py` 把仓库内的上游副本 `_upstream/` 当成"本地树"，使上游一致性门恒过
- 文件:行：`Tools/verify/parse_tests.py:28-31`（`SKIP_DIRS` 无 `_upstream`）、`:53`（`os.walk(root)`）、`:126-148`（比较与退出码）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`_upstream/warpX-26.10` 是本仓库内的上游拷贝（`.gitignore:109` 忽略，但审计环境存在），其中含 408 个 `add_warpx_test()` 定义。实测：
  ```
  $ python3 Tools/verify/parse_tests.py --compare _upstream/warpX-26.10 .
  reference _upstream/warpX-26.10: 408 definitions
  local     .: 832 definitions
  shared names: 408
  only in reference: []
  tests whose full argument list differs: 0
  ... exit=0
  ```
  "local" 恰好是 reference 的两倍（408→832，各维度计数也精确翻倍：RZ 53→106），说明上游定义被当作本地定义参与比较。后果：删掉本地任何 `add_warpx_test()`，`only in reference` 仍为空、退出码仍为 0；`UPSTREAM_PARITY.md` 所依赖的"逐字比对"因此不可证伪。
- 建议修法：`SKIP_DIRS |= {"_upstream", "audit_build", "audit_runs"}`；或要求显式传入"本地根"并拒绝包含 `_upstream` 的树。
- 验证方法：临时把 `Examples/Tests/langmuir/CMakeLists.txt` 的某个 `add_warpx_test` 注释掉（或复制一份树删掉它）后再跑，仍 exit 0。

### F3 `check_kernel_coupling.py` 的 5 个监控目录有 3 个不存在，静默跳过仍报 OK
- 文件:行：`Tools/Modularization/check_kernel_coupling.py:25-33`（TREES）、`:48-49`（`if not base.is_dir(): continue`）、`:35`（TOKENS）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`ls Source/Plugins/PicEm/Modules/FieldSolver` 只有 `Magnetostatic/` 与若干 `WarpX*.cpp`；`FiniteDifference/`、`Electrostatic/`、`PSATD/` 三个目录不存在（已迁到 `Source/Plugins/FieldSolver/`）。脚本对缺失目录直接 `continue`，然后打印 `kernel coupling OK (0 known entries, 0 new)`（本次实测 exit=0）。文档（`FINAL_REPORT.md:83`、`S_SERIES_STATUS.md:32`）据此声明"待拆模块不再耦合内核"，但实际只跑了 `ParticleCore` 与 `Magnetostatic`。
  另外 `TOKENS` 只含 `"WarpX::"` 与 `'#include "WarpX.H"'`：漏 `#include <WarpX.H>`（`check_plugin_boundaries.py:18-20` 自己指出了尖括号问题，这里没同步）、漏命名空间别名（`namespace W = WarpX; W::Evolve()`）、漏 `WarpX::GetInstance()` 的转写（如 `using WX = WarpX;`）。
- 建议修法：目录不存在时报错（而不是跳过）；TREES 改为现存路径或改为"在 Source/Plugins 下 glob 所有层"；TOKENS 改为正则 `#include\s*[<"]WarpX\.H[>"]`，并对 `using`/`namespace` 别名做一次解析。
- 验证方法：`python3 Tools/Modularization/check_kernel_coupling.py`；在 `Source/Plugins/ParticleCore` 某文件里加 `namespace W = WarpX;` + `W::nox;` 后重跑，仍报 0。

### F4 `check_legacy_coverage.py` 候选路径不含 `Core/Modules/` 与其它插件根，554/829 假"缺失"；且与文档"missing: 0"冲突
- 文件:行：`Tools/Modularization/check_legacy_coverage.py:26`（candidates 仅 `rel`、`Core/{rel}`、`Plugins/PicEm/{rel}`、`Plugins/PicEm/Modules/{rel}`）、`:34`（`sys.exit(1 if missing else 0)`）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：本次实测 `legacy files: 829 / missing: 554 / exit=1`。其中 551 个文件确实存在于 `Source` 下，只是路径前缀变了：
  - `AcceleratorLattice/AcceleratorLattice.H` → `Source/Plugins/ParticleCore/Particles/AcceleratorLattice/AcceleratorLattice.H`
  - `AcceleratorLattice/LatticeElementFinder.H` → `Source/Core/Modules/AcceleratorLattice/LatticeElementFinder.H`
  - 分类统计：18 个在 `Core/Modules/`，427 个落在其它插件（`ParticleCore`、`FieldSolver`、`Diagnostics`…）的 `Particles|Modules` 子目录，其余多为已合并的 `CMakeLists.txt`。
  而 `A7_MASTER_PLAN.md:13-17` 写 `missing: 0` 并以本脚本为"校验"，`A7_ROUTE_AUDIT.md:170` 又承认它"只能证明文件计数"。当前状态：门是红的、文档是绿的、没人跑（见 F6）。
- 建议修法：候选集改为"在 `Source` 全树按 basename+相对后缀匹配，或按 `Source/{Core,Plugins/*}{,Modules}{,Particles}/{rel}` 枚举"；对 `CMakeLists.txt` 这类构建文件单独豁免/计数。
- 验证方法：`python3 Tools/Modularization/check_legacy_coverage.py | head`；对比 `find Source -name LatticeElementFinder.H`。

### F5 `check_core_purity.py` 的匹配规则可被命名空间别名绕过，注释判定也不可靠
- 文件:行：`Tools/Modularization/check_core_purity.py:19-26`（TOKENS 为字面量）、`:44-46`（只跳过以 `//`、`*`、`/*` 开头的行）、`:51-53`（`entry = f"{rel}|{token}"`）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：文档称"零容忍、允许清单为空"（`core_concrete_allowlist.txt:1-2`、`FINAL_REPORT.md:81`）。
  1. 别名绕过：`namespace W = WarpX;` 之后 `W::nox` 不含子串 `WarpX::`，`\bWarpXParIter\b` 也匹配不到 `WarpXParticleContainer` 之外的 typedef（如 `using PC = PhysicalParticleContainer;` 中只在 using 行命中一次，后续 `PC` 不命中）。当前 `Source/Core` 为 0 命中，"零容忍"只证明了没有出现字面 token。
  2. 注释：只跳过"行首"注释 → `int x; // 见 WarpX::Evolve` 会被误报；而块注释里不以 `*` 开头的续行（如 `/* 说明\n WarpX::nox */`）会被漏报。
  3. 粒度：允许清单以"文件|token"为键，同一文件同 token 的**第二处**新增引用不会触发失败，与"the number can only go down"的棘轮语义不符。
  4. 目录不存在时（如重命名 `Source/Core`）脚本 rglob 结果为空仍 `exit 0` 并打印 `0 known references`。
- 建议修法：先做一次轻量预处理（去注释/字符串，并把 `namespace X = WarpX;`、`using X = <concrete>;` 展开为 token 集合）；允许清单键改为 `file|token|count`；`CORE` 不存在时 `sys.exit(2)`。
- 验证方法：在 `/tmp` 复制一份 `Source/Core` 并加入 `namespace W = WarpX; int f(){return W::nox;}`，把脚本 `CORE` 指过去后重跑，输出仍为 0 new。

### F6 四个"不变量检查器"没有进入任何自动门（CI / pre-commit / run_gates 都没有）
- 文件:行：`Tools/verify/run_gates.sh:28-30`（只跑 compliance、audit_check、`route_tasks.py next`）、`.github/workflows/route-gates.yml:24-29`（同样三步）、`Tools/verify/install_git_hooks.sh:15-19`（hook 调 `run_gates.sh`）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`HANDOFF.md:8-11` 与 `FINAL_REPORT.md:211-213` 把 `check_core_purity/plugin_boundaries/kernel_coupling/build_boundaries` 列为项目门；`run_gates.sh:1-8` 更声称"Run exactly what the route-gates CI job runs"。实际上 CI 只跑 compliance/audit/route-next 三步，`Tools/Modularization/*` 一个都没跑，`ctest` 也不跑；`check_legacy_coverage.py` 当前是 exit 1，因为没人调用所以无人察觉。`route_tasks.py next` 另外还永真（F7）。
- 建议修法：在 `run_gates.sh` 与 `route-gates.yml` 中加一个 `for s in Tools/Modularization/check_*.py; do python3 "$s"; done` 步骤（先修 F1/F3/F4，否则会红）；`run_gates.sh` 的注释与实现改成一致。
- 验证方法：`bash Tools/verify/run_gates.sh; echo $?`（当前 0），再 `python3 Tools/Modularization/check_legacy_coverage.py; echo $?`（当前 1）。

### F7 `route_tasks.py` 可以在"什么都没验证"的情况下把任务标成 done，且 153/153 已是 done
- 文件:行：`Tools/automation/route_tasks.py:165-178`（无 `automation_command`、无 `--gate`、无 `verify_command` 时 `ok` 保持 True）、`:180-187`（写状态并**改写受版本控制的 `route_tasks.json`**）、`:261-263`（"keep JSON done authoritative" 为死代码，`load_state` 已 setdefault 全部任务）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`python3 Tools/automation/route_tasks.py run A1.4`（不带 `--gate/--accept/--force`，任务 kind 非 refactor）会直接打印 "marked done" 并把 `Tools/automation/route_tasks.json` 的 `status` 改写为 done，没有任何构建/测试/检查被执行。当前 `route_tasks.json` 153 个任务全部 `status: "done"`，`route_tasks.py next` 输出 "no pending task with satisfied dependencies"，而同一树上 `check_legacy_coverage.py` 失败、F1/F3 的检查器形同虚设。另外 JSON 的 `gates.full` 声明含 `"ab"`，但 `gates/full_gate.sh:10` 明确只做 compliance+audit+build+smoke 并打印 "A/B regression gate is added together with the first plugin" —— 也就是说 153 个"已过 full gate"的说法里，A/B 门从未存在。
- 建议修法：`ok` 初值改为 False，只有执行过至少一条 gate/verify 命令且成功才置 True；`mark done` 与实际门解耦记录（`evidence` 字段）；不要在被门评估时改写 spec 文件。
- 验证方法：`python3 Tools/automation/route_tasks.py run <一个 pending 任务>` 观察 "marked done"；`python3 -c "import json;d=json.load(open('Tools/automation/route_tasks.json'));print(len(d['tasks']),sum(t['status']=='done' for t in d['tasks']))"`。

### F8 macroscopic 物理核验脚本根本不做比较：只打印，调用方只 grep 关键词
- 文件:行：`Tools/verify/check_macroscopic_cavity_mode.py:16-19`（打印 ratio，无 assert、无 return 码）、`Tools/verify/check_macroscopic_cavity_mode_2d.py:22-24`（同）、`Tools/verify/check_macroscopic_2d_normal.sh:22`（只调用它）、`Tools/verify/check_macroscopic_subpixel_step.sh:26-28`（`| tee mode.txt`，`grep -q "amplitude ratio"`）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`check_macroscopic_2d_normal.sh:10-13` 声称"a wrong Ez position shows up as a wrong omega, i.e. a wrong amplitude"，但 `check_macroscopic_cavity_mode_2d.py` 只 print `amplitude ratio ... analytic ...`，从不比较；`check_macroscopic_subpixel_step.sh:13-14` 声称"mode decays like the analytic exp(...) within the same tolerance"，实际只 grep 输出里出现 "amplitude ratio" 字样（19 行脚本连 expected/measured 都没放进同一个断言）。因此 Ez 半格偏移、指数步长的 sigma 分支等回归都检不出来——只要跑完且日志里有那行字。
- 建议修法：在 `check_macroscopic_cavity_mode*.py` 里加 `assert abs(measured/expected - 1) < tol` 并 `sys.exit(1)`；shell 侧改为检查退出码而不是 grep 文案。
- 验证方法：把 `check_macroscopic_cavity_mode_2d.py:22` 的 `expected` 乘 2 后重跑 shell 脚本，仍 exit 0。

### F9 A/B 上游对照工具链永远不会失败（run_ab 忽略退出码、run_all 恒 exit 0、compare_fields 只打印）
- 文件:行：`Tools/verify/ab/run_ab.sh:21`（`set -uo pipefail`，无 `-e`）、`:37-39`（`echo "$side exit=$?"` 后继续）；`Tools/verify/ab/run_all.sh:83-90`（不检查 `run_ab.sh` 状态，末尾 `exit 0`）；`Tools/verify/ab/compare_fields.py:62-64`（`scale=...`、`dmax/scale` 只存入 rows，无阈值；`scale==0` 时 `ZeroDivisionError`）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`UPSTREAM_PARITY.md` 的对照表由此脚本产出。若参考侧或被测侧根本没启动（二进制缺、输入路径错），`run_ab.sh` 只打印 `up exit=127`，`run_all.sh` 仍 `ran+=1` 并 `exit 0`；`compare_fields.py` 打印 "missing plotfile on one side, skipped" 后返回 0。零个用例被真正比较也算成功。`ab/README.md:46-47` 的"skipped with a message ... instead of failing"把这种行为写成了设计。
- 建议修法：`run_ab.sh` 加 `set -e` 或显式收集 status；`run_all.sh` 汇总失败并返回非零；`compare_fields.py` 对每个字段给出 `max|diff|/scale` 阈值（例如 1e-12）并 `return 1`，并对 `scale==0` 用绝对容差。
- 验证方法：`UP_BIN=/bin/false MINE_BIN=/bin/false bash Tools/verify/ab/run_ab.sh x warp:Examples/Tests/langmuir/inputs_test_1d_langmuir_multi; echo $?` → 0；再跑 `run_all.sh`。

### F10 `check_star_runtime_plugin.sh` 明确容忍"关机时堆损坏"的非零退出
- 文件:行：`Tools/verify/check_star_runtime_plugin.sh:19-31`。
- 严重度：major
- 置信度：confirmed
- 现象与依据：注释自述"Known defect: with the plugin loaded the run aborts at shutdown (heap corruption in a deque destructor)"，脚本 `set +e` 包住运行、`[ "$status" -ne 0 ]` 只打印 NOTE、最后仍打印 "star runtime plugin: NewtonGravity solved the potential from a loaded plugin" 并 exit 0。即"加载插件会把进程跑挂"这一真实缺陷被检查器吸收为 note。
- 建议修法：已知缺陷要么修，要么让检查器对已知缺陷另设一个会失败的 `xfail` 通道（例如 `--allow-shutdown-abort` 显式开关），默认必须 exit 0。
- 验证方法：`WARPX_PLUGIN_PATH=... warpx.3d ...; echo $?` 应为非零，而脚本仍 exit 0。

### F11 OPAL 不透明度自检的容差（1 dex / 中位数 2 倍、只抽样 1/4 层）宽到无法发现系统性错误
- 文件:行：`Tools/star/opal_opacity.py:153`（`rows[::4]`）、`:166`（`assert 0.5 < median < 2.0`）、`:167`（`assert worst < 1.0`，即 10 倍）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：本次实测 `median ratio 0.986, worst |log10 ratio| 0.733 dex` —— 最坏点已是 5.4 倍偏差却通过（`< 1.0 dex`）；docstring 却称"very nearly a test of the extraction and the interpolation"。也只测了 `kappa()`，模型实际使用的 `kappa_combined()`（`opal_opacity.py:119-140`，含 H⁻ 拟合拼接）没有任何断言。
- 建议修法：worst 阈值改为 `0.1–0.2 dex`（或对 0.2 以上差异逐层报告并失败），全覆盖而非 `[::4]`；补一条 `kappa_combined` 在 T<1.2e4 与 T>1.2e4 拼接连续性的断言。
- 验证方法：`python3 Tools/star/opal_opacity.py --selftest`，改阈值到 0.1 会立即失败（当前 worst 0.733）。

### F12 `eos.py` 的 Debye 屏蔽离子和对与自身文档不符（用 (Z+1)² 代替 Z²），HeⅡ 的连续谱降低也用错倍数
- 文件:行：`Tools/star/eos.py:70`（docstring：`1/R_D^2 = 4πe²/(kT)(n_e + Σ n_i Z_i²)`）、`:144`（`ions = n_hp + n_hep*4.0 + n_hepp*9.0 + n_z*Z_HEAVY**2`）、`:148-151`（H 用 `lowering`，HeⅠ/HeⅡ 都用 `2.0*lowering`，而 `:91` 注释写的是 `(Z+1)e²/R_D`）。
- 严重度：major
- 置信度：confirmed（公式层面；对最终太阳模型的影响量级：needs-verification）
- 现象与依据：Σn_i Z_i² 对 H⁺/He⁺/He²⁺ 应为 `n_hp*1 + n_hep*1 + n_hepp*4 + n_z*49`，代码写的是 `1→4, 1→4, 2→9`，即 `(Z+1)²`。这直接改变 `_debye_radius`，从而改变 `lowering`，再改变 Saha 平衡（`state()` 是 `fluid_*` 系列 ctest 的基准）。HeⅡ→HeⅢ 按自己的注释应为 `3*lowering`，代码仍 `2*lowering`。
- 建议修法：改为 `n_hp + n_hep + 4.0*n_hepp + Z_HEAVY**2*n_z`；HeⅡ 用 `3.0*lowering`（或统一写 `(Z+1)*E²/R_D` 的辅助函数）。
- 验证方法：`python3 Tools/star/eos.py --selftest`（当前仍会通过，说明自检对这两个系数不敏感——自检本身也该补一条与解析 Debye 长度的对比）。

### F13 `stellar_structure.selftest` 的"四方程积分"只检查打印文本里出现 `R/R_sun`，NaN 也能通过（且已被注册为 ctest）
- 文件:行：`Tools/star/stellar_structure.py:677-687`（`assert "surface:" in text` / `assert "R/R_sun" in radius_text`），打印处 `:439`。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`compare_solar_diagnostic()` 以 `f"{last[1]/R_SUN:.3f}"` 打印，NaN 会打印成 `nan`，字符串仍含 `R/R_sun`，两个 assert 都通过；注释却写 "the machinery must produce a finite, positive model rather than a NaN"。而 `Source/Plugins/FluidMoments/CMakeLists.txt:100` 把它注册成 ctest `fluid_lane_emden`，于是"太阳模型结构积分是有限的"这一条在 CI 里没有被检查。补充：该 ctest 本次实测耗时 **123.09 s**（`/usr/bin/time` 实测，单核 100%，因为 `integrate_solar_structure` 默认 `n_steps=2000`，而 `solar_compare_plot.py` 只用 240），且没有 `set_tests_properties(TIMEOUT)`——一个 python 自检占掉 2 分钟 ctest 预算且无超时保护。
- 建议修法：让 `compare_solar_diagnostic()` 返回 `(R, L)` 并在 selftest 里 `assert math.isfinite(R) and R > 0`、`assert 0.3 < R/R_SUN < 1.5`；把 selftest 的 n_steps 降到 240（和 `solar_compare_plot.py` 一致）或加 TIMEOUT。
- 验证方法：把 `solar_compare_diagnostic` 的最后一层 `r` 改为 `float('nan')` 后跑 `--selftest`，仍 exit 0。

### F14 新增 pytest 用例在**仓库根**写文件，`mkdir()` 无 `exist_ok`，并行维度互相覆盖
- 文件:行：`tests/unit/run_tests.py:21-30,56,63-66`（`project_root()` 一路向上找到含 `pyproject.toml` 的目录，即仓库根，并以 `cwd=root` 启动 pytest）；`tests/unit/test_load_inputs_file.py:74-76`（`scan_dir = pathlib.Path("a0=2"); scan_dir.mkdir()`）；`tests/unit/test_read_dims_from_file.py:20-23`（写 `inputs_base`、`inputs_test`）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：CMake 为每个维度建了独立工作目录（`cmake/WarpXFunctions.cmake:329-331` 的 `bin/pytest.WarpX.<SD>`），但 `run_tests.py` 以 `cwd=仓库根` 调 pytest，所有写盘都落在源树根：一次 `ctest -R pytest` 会生成 `./inputs_test`、`./inputs_base`、`./a0=2/`；`a0=2` 用无 `exist_ok` 的 `mkdir()`，**同一工作树第二次运行即 FileExistsError 失败**；`pytest.WarpX.1d/2d/3d/rz` 四个 ctest 并行时写同一批文件名，存在真实竞态。`run_tests.py:10-11` 注释关心的只是"collect 时机"，没有处理工作目录。
- 建议修法：`cwd` 用 `os.getcwd()`（CMake 已给的唯一工作目录），node id 用绝对路径；或每个测试用 `tmp_path` fixture + `monkeypatch.chdir`；`mkdir(exist_ok=True)`。
- 验证方法：`ctest -R pytest.WarpX.2d` 跑两遍（第二遍在 `a0=2` 上失败），`git status` 观察仓库根多出文件。

### F15 `test_plugin_lifecycle.cpp` 的 "satisfied requirement" 用例断言的是"被拒绝"，role 名作为 provider 从未被测
- 文件:行：`tests/runtime/test_plugin_lifecycle.cpp:177-187`。
- 严重度：major
- 置信度：confirmed
- 现象与依据：注释写 "Satisfied requirement: role name counts as a provider"，代码加入 `ProbePlugin`（要求 `"kernel"`）与 `UnmetRequirementPlugin`（要求 `"no_such_plugin"`），随后 `check(thrown, "requirement for an unknown name is still rejected")` 要求**抛异常**。因为 `no_such_plugin` 必然失败，这个用例无法区分 `"kernel"` 是否被 role 名满足——把"role 名不算 provider"的 bug 注入后测试依然通过。
- 建议修法：拆成两个用例；"role counts as provider" 的用例只放 `ProbePlugin`（要求 kernel）与一个 role 为 Kernel 的插件，断言不抛异常。
- 验证方法：把 `PluginHost::validate_plugins` 中的 role-name 匹配分支删掉，只跑这一个用例，仍通过。

### F16 `check_build_boundaries.py` 的分层覆盖不完整，并留有 KeyError 地雷
- 文件:行：`Tools/Modularization/check_build_boundaries.py:42-47`（`PLUGIN_DIRS` 含不存在的 `Source/Plugins/IO`，缺 `FluidMoments`/`NewtonGravity`）、`:57-59`（`LAYERS[layer]` 无 `"io"` 键）、`:68` 注释 "The four object libraries" 与 `LAYERS` 的 6 层不符、`:34/51`（`target_sources\((.*?)\)` 遇嵌套括号会截断）。
- 严重度：major
- 置信度：confirmed
- 现象与依据：`ls Source/Plugins` → `Collision Diagnostics FieldSolver FluidMoments NewtonGravity ParticleCore PicEm`，无 `IO`。当前 `Path.rglob` 在不存在目录上返回空，所以静默通过；一旦有人新建 `Source/Plugins/IO/CMakeLists.txt`，第 57 行 `LAYERS["io"]` 直接 KeyError 崩溃而不是报违规。`FluidMoments`、`NewtonGravity` 两个插件的 CMake 完全不在检查范围内，而脚本文档声称检查"A7 B1-B7"全部层次。第 1 项也只扫 `Source/Core/**/CMakeLists.txt`，顶层 `CMakeLists.txt` 往 Core 目标里塞插件源文件不会被发现。
- 建议修法：`LAYERS` 与 `PLUGIN_DIRS` 由同一张表派生（含全部 7 个插件）；目录不存在时报错；括号匹配改成计数式（像 `parse_tests.py:63-76` 那样）。
- 验证方法：`mkdir -p Source/Plugins/IO && cp Source/Plugins/PicEm/CMakeLists.txt Source/Plugins/IO/` 后重跑脚本观察 KeyError。

### F17 5 个 runtime "contract test" 实际是 `int main(){return 0;}` 且没有注册进 CTest
- 文件:行：`tests/runtime/test_field_solver_interface.cpp:1-3`、`test_field_solver_engine_interface.cpp:1-3`、`test_particle_contracts.cpp:13-16`、`test_particle_access_contracts.cpp:11-13`、`test_collision_contracts.cpp:10-13`；注册侧 `tests/runtime/CMakeLists.txt:19-24` 只 `add_executable`，`:51-56` 起的 `add_test` 没有它们。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：生成的 `audit_build/fork/tests/runtime/CTestTestfile.cmake` 里确实只有 `particle_factory_2 … runtime_lb_2`，上述 5 个可执行文件从不出现在 ctest 里。它们的内容只证明"头文件能编译 + 类型是抽象的"，名称却暗示接口行为测试；另外两个 `static_assert(std::is_abstract_v<...>)` 没有 `#include <type_traits>`，依赖头文件间接包含。
- 建议修法：要么删掉、要么改名 `compile_only_*` 并显式 `add_test`（跑起来至少验证进程能启动），并补 `<type_traits>`。

### F18 `test_nonpic_kernel.cpp` 打印的断言数写死为 8（实际 7）
- 文件:行：`tests/runtime/test_nonpic_kernel.cpp:237`。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`std::printf("non-PIC kernel: %d checks passed\n", 8)`；实际 `check()` 调用 7 次（:208/209/212/222/224/225/226）。属于"自证输出"与实际不符（虽然退出码仍由 `failures` 决定）。
- 建议修法：改为运行计数而非字面量。

### F19 `sod_reference.py` 稀疏波分支的解析导数写错（靠括号保护的 Newton 兜住）
- 文件:行：`Tools/star/sod_reference.py:42`（`return (p / p_k) ** (-expo - 1.0) / (rho_k * c_k)`）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：由 `f_K = 2c_K/(γ-1)((p/p_K)^expo - 1)` 得 `df/dp = c_K/(γ p)·(p/p_K)^expo`；代码给出的 `(p/p_K)^{-expo-1}/(ρ_K c_K)` 只在 `p=p_K` 与正确值相等，偏离时差 `(p/p_K)^{-2expo-1}`（p=0.5 时因子约 1.22）。结果仍正确（`selftest` 的残差断言与 `p*` 数值复现 0.30313018/0.92745262），但导数分支是错的，属"参考实现里写错公式、被稳健算法掩盖"的典型。
- 建议修法：`return c_k / (gamma * p) * (p / p_k) ** expo`。

### F20 `ssm_compare.py` 自检里有一条恒真断言，README 的"5%"与断言的 15% 不符，打印的 eps/kappa 比值在 SSM 为 0 处失真
- 文件:行：`Tools/star/ssm_compare.py:200-201`（`kes_centre=0.2*(1+X)` 后 `assert abs(kes_centre - 0.2*(1+X)) < 1e-14`，恒真）、`:209`（`worst < 2.0` 与 `:213-214` 的 15% 重复且宽）、`:177-178`（`r['eps']/max(...,1e-30)`）；`Tools/star/README.md:61-62`（"5% per layer"）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：实测 `worst |ratio - 1| = 0.050`、`l_ours/l_ssm = 1.006`，即真实精度 5% 与 README 一致，但断言允许 15%，回归到 10% 也不会红。`solar_compare_plot.py:177` 的同款比值在 SSM 的 `eps` 恰为 0 的层会打印 `10932936.300`（除以 1e-30），表格容易被误读为"能量产生差 10⁷ 倍"。
- 建议修法：删掉恒真断言；把 15% 收紧到 0.06；比值列在分母为 0 时打印 `n/a`。

### F21 `solar_compare_plot.py --check` 向源码树写 PNG，且只钉住 L、R 两个数
- 文件:行：`Tools/star/solar_compare_plot.py:165-166`（`fig.savefig(OUT/"solar_comparison.png")`，`OUT` 为 `Docs/research_notes/star_plugins`）、`:184-189`（`assert 1.10 < L < 1.16`、`assert 0.51 < R < 0.57`）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：该脚本被注册为 ctest `fluid_solar_model`（`Source/Plugins/FluidMoments/CMakeLists.txt:97`），每次 ctest 都会改写源码树里的 PNG（对只读源码挂载或并行 ctest 都是问题）。而它自己的输出表里 `eps ratio` 到 m/M=0.992 已到 1e7 量级、`kappa ratio` 0.018，这些都不影响 assert。作为"回归基线"可以接受（注释也说明了），但不应在测试目录里被理解成物理验收。
- 建议修法：PNG 写到 `CMAKE_CURRENT_BINARY_DIR`/环境变量指定目录；把 `--check` 的断言范围与打印表解耦说明。

### F22 插件测试的构建假定 ELF/macOS 以外的编译器与 `.so` 后缀
- 文件:行：`tests/external_plugin/CMakeLists.txt:8-10`（仅判 `CMAKE_C_COMPILER`）、`:29-31`（`-shared -fPIC -std=c11`、`${ep_lib}=...libthird_party_plugin.so`）；`tests/plugins/CMakeLists.txt:30-35`（`SET` 后缀由 CMake 处理，但 `LINKER_LANGUAGE CXX` 与 `-shared` 组合仍限 POSIX）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：仓库的 CI 矩阵含 `windows.yml`、`macos.yml`（`.github/workflows/`）；`-shared -fPIC -std=c11` 在 MSVC 下必然失败、`.so` 在 macOS 应为 `.dylib`。目前 `tests/` 是否被 `add_subdirectory` 取决于 `WarpX_BUILD_TESTS`，所以不会立刻炸，但一旦在这些平台打开测试就是硬失败。
- 建议修法：`if(NOT UNIX OR APPLE) return()` 或使用 `set(CMAKE_SHARED_LIBRARY_SUFFIX)`/生成器表达式，并按编译器选择 `-fPIC`/`/LD`。

### F23 若干 shell 核验脚本的硬编码与静默 no-op
- 文件:行：`Tools/verify/check_macroscopic_subpixel_step.sh:20-23`（`sed -e 's/^macroscopic.sigma   = 0/.../'`，三个空格，若不匹配则静默不改 → 指数分支根本没进，脚本仍通过）、`check_macroscopic_periodic_interface.sh:18-22`（只接受 `$repo/build_multi`，不接收 build 参数；只 grep `[GAUSS]` 不校验 5.66e-13 vs 1.70e-12）、`check_macroscopic_vacuum_limit.sh:26,43`（`WORK` 可配但 python heredoc 里写死 `/tmp/macro_vacuum_limit`；`PY` 默认 `$HOME/miniconda3/bin/python3`；`EXE` 写死全后缀二进制名）、`check_star_runtime_plugins.sh:45`（`grep -E "joule = [1-9]"` 对 `0.5` 这类值会误判为失败）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`set -euo pipefail` 不会捕获"sed 没匹配到"，`grep -q` 只匹配字面量。`check_macroscopic_2d_normal.sh:17` 与 `vacuum_limit.sh:25` 分别写死 `build_2d` / `build_multi_hdf5_fft` 与全后缀可执行名，换构建配置即失效。
- 建议修法：sed 后 `grep -q '^algo.macroscopic_subpixel = 1'` 自检；heredoc 用 `"$WORK"`；统一用一个 `--build` 参数与 `warpx.1d` 软链。

### F24 A0 两段流水线的输入路径对不上，文档化的"重新生成"无法执行
- 文件:行：`Tools/A0/parse_headers.py:90`（写 `Tools/A0/header_catalog.json`）、`Tools/A0/build_catalog.py:5,34`（读 `/tmp/header_catalog.json`）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：生产者写仓库内 `Tools/A0/header_catalog.json`，消费者读 `/tmp/header_catalog.json`，按 build_catalog 的 docstring 顺序执行会直接 `FileNotFoundError`。另外 `parse_headers.py` 用相对路径 `ROOT='Source'` 与 `'Tools/A0/...'`，必须在仓库根执行。
- 建议修法：统一路径（参数化或都用 `ROOT/"Tools/A0/header_catalog.json"`）。

### F25 两个"门"的退出码恒为 0
- 文件:行：`Tools/automation/gates/smoke_2d.sh:7`（`ctest -R '...'` 无 `--no-tests=error`）、`Tools/verify/classify_ctest_log.py:139`（`return 0`）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：CMake ≥3.20 的 ctest 在 `-R` 一个都没匹配到时**退出 0**（这正是 `--no-tests=error` 存在的原因），所以一旦测试改名/挪走，`smoke_2d.sh` 会在"跑了 0 个测试"的情况下成功。`classify_ctest_log.py` 无论发现多少 `candidate real defect` 都返回 0，不能作为门使用（其 docstring 也自认只是初判）。
- 建议修法：`ctest ... --no-tests=error`；分类脚本增加 `--fail-on candidate` 之类的开关。

### F26 `check_field_solver_extraction.py` 永远 exit 0，且统计的是已经搬空的旧树
- 文件:行：`Tools/Modularization/check_field_solver_extraction.py:30`（`sys.exit(0)`）、`:6`（`PICEM = Source/Plugins/PicEm`）。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：名字是 `check_*`，输出 "external FieldSolver includes: 1 / ImplicitSolver: 2 / HybridPICModel: 90" 这样明显的阻塞项，却无条件成功；而 FieldSolver 实测已在 `Source/Plugins/FieldSolver`，脚本仍只统计 `PicEm` 树，输出（`WarpX.cpp: FieldSolver/WarpX_FDTD.H`）不能说明当前抽取状态。
- 建议修法：改成 `report_` 前缀或加 `--fail-on-blockers`；根目录指向 `Source/Plugins`。

### F27 `route_tasks.py` 的"JSON 权威"是死代码
- 文件:行：`Tools/automation/route_tasks.py:260-263`。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`load_state`（`:73-77`）已经对每个任务 `setdefault`，所以 `if t["status"] == "done" and t["id"] not in state` 永远为假，注释所称"keep JSON's done entries authoritative on first run"没有实现；实际是 `.route_state.json`（gitignored）权威。
- 建议修法：删除死分支，或先读 state 再按 JSON 合并并显式打印冲突。

### F28 `a5_ab.py` 未接入任何门，且硬编码二进制与构建目录
- 文件:行：`Tools/automation/gates/a5_ab.py:9`（`EXE = ROOT/"build/bin/warpx.2d.MPI.OMP.DP.PDP.EB.QED"`）、`:21`（`tempfile.mkdtemp` 不清理）；`gates/full_gate.sh:10` 自认 A/B 门不存在。
- 严重度：minor
- 置信度：confirmed
- 现象与依据：`route_tasks.json` 的 `gates.full` 含 `"ab"`，但没有 `ab` 命令映射（`route_tasks.py:31-43` 的 `GATES` 里 `full` 只有 `full_gate.sh`），A5 门实际上是一段孤立的脚本。`build_2d.sh` 的默认构建目录是 `build`，与硬编码一致，但换 `WARPX_BUILD_DIR` 即失配。
- 建议修法：把 `ab` 纳入 `GATES`，用 `$WARPX_BUILD_DIR`/`TARGET_FILE` 传二进制，`shutil.rmtree` 清理。

### F29 `build_and_run_4d.sh` 的 conda 前缀变量名疑似笔误
- 文件:行：`Tools/verify/build_and_run_4d.sh:38`（`CONDA="${CONDA_PREFIX_PREFIX:-$HOME/miniconda3}"`）。
- 严重度：minor
- 置信度：likely
- 现象与依据：全脚本只有这一处使用该变量，随后传 `-DCMAKE_PREFIX_PATH="$CONDA"`。常规环境导出的是 `CONDA_PREFIX`，`CONDA_PREFIX_PREFIX` 不会存在，于是永远回落到 `$HOME/miniconda3`；若用户用别的 conda 前缀，脚本会拿到错的 AMReX/blaspp 头文件。
- 建议修法：`CONDA="${CONDA_PREFIX:-$HOME/miniconda3}"`（或显式 `CONDA_ROOT`）。

### F30 `inventory_field_solver.py` 写入源码树文档，且 `classify()` 有多余/不可达分支
- 文件:行：`Tools/Modularization/inventory_field_solver.py:7`（`OUT = Docs/research_notes/FIELD_SOLVER_INVENTORY.md`，`:97` 直接覆盖写）、`:29-32`（`FiniteDifferenceSolverFactory.cpp`/`FiniteDifferenceSolver.cpp` 两条 `rel.name` 分支之前，`:11-12` 已把 `FiniteDifferende/Algorithms` 之外的所有 `HybridPIC` 等拦截；实际文件已不在该树）。
- 严重度：minor
- 置信度：likely
- 现象与依据：脚本的"门"（`:78-81` 未分类则 return 1）依赖一份可能已过时的 `PicEm/Modules/FieldSolver` 树；同时它有副作用地改写受版本控制的文档。审计中未执行它以免污染工作树。

### F31 其它可靠性/自证性较弱的点（合并记录，共 9 条）
- 严重度：minor
- 置信度：confirmed（除注明外）
- `tests/runtime/test_particle_factory.cpp:45-52`：第一条断言"未注册工厂返回 null"依赖链接器不把粒子插件的静态注册对象拉进来（注释也承认）。一旦插件改为显式注册，第一条断言语义就变了却仍可能通过。
- `tests/runtime/test_plugin_field.cpp:81-91`：checkpoint 是否写出只按"当前目录下存在以 `plugin_probe_field` 开头的文件"判断，文件名/目录名都算，未校验内容或步骤号。
- `tests/runtime/test_service_registry.cpp:47-58`：`GetTotalDiags()==0` 在 null 上下文下恒真，两条断言区分不了"真服务"与"no-op 服务"。
- `tests/external_plugin/third_party_plugin.c:64-85`：第三个 handler 逐字段赋值（其余两个用 `= {0}`），后续若 ABI 结构体新增字段且这里漏填即 UB；建议统一 `= {0}`。
- `Tools/verify/compare_particles_openpmd.py:66`：`scale = max(max|a|-min|a|, max|a|, 1e-300)` 混用绝对值与跨度，单个同值数组时退化为 `max|a|`；容差 1e-10 相对"跨度"，对动量这类窄分布偏松。
- `Tools/star/cf88_reference.py:200-204`：`expected_q` 与 `RATES` 的 Q 值逐字相同，属于自证；`f76/f61/f71` 的系数未与 CF88 原表独立核对（本次只核对了 f2/f4/f18/f24/f41/f42/f71 与记忆中的 CF88 形式一致）。
- `Tools/star/parse_fgong.py:78,87`：用 `header[0]` 计算 `step` 与行数，而 `:44-48` 明确"以记录数为准、header 可能差 1"；同一文件里两套计数。
- `Tools/star/extract_opal_subset.py:90-92`：`np.savez_compressed` 前没有 `DATA_DIR.mkdir(parents=True, exist_ok=True)`，目录缺失时抛异常。
- `Tools/automation/gates/audit_check.py:10-14,28-37`：只用 `min_headers=372/min_classes=300/min_methods=2000` 下界与文件存在性，实测 `headers=498 classes=349 public_methods=2418` —— 低于下界的回退会失败，但内容是否与当前 `Source` 一致（例如 `classes.csv` 是否过期）无法判断。
- `docs` 侧（非本分区，但为本分区结论的背景）：`S_SERIES_STATUS.md:75` 写 `check_plugin_boundaries.py` 为 `10 known, 0 new`，而当前实测为 `0 known, 0 new` 且允许清单已被清空（`plugin_boundary_allowlist.txt:1-14` 声称那 10 条都已修）；该文件同时声称 `check_core_purity` 是"零容忍"，与 F5 的匹配能力不符。

## 不确定但值得复核的点

1. `check_core_purity.py` / `check_kernel_coupling.py` 声称覆盖"core 不出现具体 WarpX 类型"，但内核 `WarpX.H/.cpp` 本身在 `Source/Plugins/PicEm/`（不是 `Source/Core`）。需要产品层面确认"core"的定义；如果定义是 `Source/Core`，那 0/0 的结论只说明"被搬走的文件里没有"，不能说明模块边界成立。
2. `tests/runtime/CMakeLists.txt:104-118` 的两个 `particle_free_*` 测试只在 `NOT WarpX_PLUGIN_PARTICLECORE` 时注册；当前构建里是否注册、是否真的跑过，需要看各 CI 配置的 `WarpX_PLUGIN_*` 组合（本次只核对了 `audit_build/fork`）。
3. `test_runtime_snapshot.cpp` 的 1e-12 相对容差看起来是"同一次运行的重复计算"，跨平台/跨编译器是否稳定需要实跑（本次未执行 ctest；`audit_build` 已有构建，可直接 `ctest --test-dir audit_build/fork -R runtime_snapshot`）。
4. `cf88_reference.py` 的 f61/f64/f76/f78/f24 系数与 CF88 原表的逐项比对（本次只做了形式核对，未逐字查表）；`Cf88Rates.H` 属另一分区，两边若同源转录错误则互检不出。
5. `opal_opacity.py` 的 `kappa()` 在 X/Z 方向做线性插值、在表格空白角做"最近有效点"回退；`kappa_combined()` 在 T=1.2e4 处从 OPAL 硬切到 `kappa_analytic + H⁻`。切换处是否连续、是否在太阳包层造成台阶，需要用 SSM 包层剖面逐点扫一次（本次只跑了 selftest 的 1/4 抽样）。
6. `sod_reference.py` 的 `star_state` 上界 `hi = 2*max(p_l,p_r)+1e3` 与消元；对强激波（p*/p ≫ 1e3）的鲁棒性未测。
7. `tests/unit/run_tests.py` 的 collect 解析（`"::" in line` 且不以 `<` 开头，再 `split(" - ")`）对带空格/破折号的 parametrize id 可能误切；需要拿一个含空格的参数化用例验证。
8. `Tools/verify/parse_tests.py` 在 SKIP 掉 `_upstream` 之后，`local`/`reference` 的 `add_warpx_test` 计数应回到 ~424 vs ~408；差值与 `only in local` 的 17 个用例是否为 fork 有意新增，需要与 `UPSTREAM_PARITY.md` 对账。
