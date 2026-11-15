# Tools_Tests 工作日志

claim: Tools_Tests F9,F15-F31 @lead-tools 2026-11-15T18:30
F30 fixed by HASH(F30); FS 根改为 Source/Plugins/FieldSolver（旧 PicEm/Modules/FieldSolver 已搬空），输出默认 audit_runs/FIELD_SOLVER_INVENTORY.md（$FIELD_SOLVER_INVENTORY 可覆盖），不再覆盖受控文档。验证: 任意 cwd 下运行 → wrote audit_runs/... (59 files)、exit 0、git status 无 Docs 改动 ✓
F21 fixed by HASH(F21); PNG 改写到 $SOLAR_COMPARE_OUT（默认 audit_runs/star/，已 gitignore），源码树不再被写。验证: 从 /tmp 运行 solar_compare_plot.py --check → 产物落 audit_runs/star/solar_comparison.png，git status Docs/research_notes 干净；断言 L=1.125/R=0.539 仍通过 ✓
F20 fixed by HASH(F20); 删掉恒真断言，改为校验 kappa_analytic 返回的 kes 分量 == 0.2(1+X)（能抓到返回值顺序错）；层间容差 15%→6%，删除与之重复的 worst<2.0；eps 比值在 SSM 为 0 时打 n/a、量级超 1e3 用 %g。验证: --selftest 通过（实测 worst 0.050）；反证: 把 epsilon_pp 放大 20% → AssertionError 1.1219 ✓
F18 fixed by 05c0ecc; printf 改为打印真实 check() 计数（新增 checks 变量）。验证: 重编 test_nonpic_kernel_2d 后输出 'non-PIC kernel: 7 checks passed'，与源码中 7 处 check() 一致 ✓
F22 fixed by HASH(F22); external_plugin 用 CMAKE_SHARED_LIBRARY_{PREFIX,SUFFIX} 与平台共享库标志（APPLE: -dynamiclib），非 UNIX 直接 return；tests/plugins 的 dlopen 测试加 POSIX 守卫。验证: ctest -R "plugin_|external_plugin" -j4 → 10/10 Passed ✓
F17 fixed by HASH(F17); 5 个 stub 改名为 compile_only_*（含 <type_traits>），并在 tests/runtime/CMakeLists.txt 里显式 add_test。验证: ctest -R '^compile_only' -j4 → 5/5 Passed（此前这 5 个可执行文件从不出现在 ctest 里）✓
F27 fixed by HASH(F27); load_state 已 setdefault 全部任务，原先 271-275 的合并循环是死代码，删掉并注释说明 JSON/state 的分工。验证: route_tasks.py next / report 均 exit 0、输出不变 ✓
F26 fixed by HASH(F26); 改成棘轮门: 扫 Source/Plugins（排除 FieldSolver 插件）统计 FieldSolver includes 与具体求解器类型用量，与 field_solver_extraction_baseline.json 比较，只允许下降。验证: 正常 exit 0；反证: 往 PicEm/WarpX.cpp 追加 #include "FieldSolver/InjectedDefect.H" → exit 1，还原后 exit 0 ✓；--report 只打印
F25 fixed by HASH(F25); 验证: ctest --no-tests=error -R '^this_matches_nothing$' → exit 8（旧行为 exit 0），真实正则仍匹配 5 个测试 ✓；classify_ctest_log.py: 默认 exit 0，加 --fail-on candidate 后对含 candidate 的日志 exit 1（合成日志 /tmp/fake_ctest.log）✓
F29 fixed by HASH(F29); 验证: bash -n 语法 OK，CONDA="${CONDA_PREFIX:-...}" ✓
F24 fixed; 验证: 在 /tmp 下运行 Tools/A0/parse_headers.py 与 build_catalog.py → 前者写 Tools/A0/header_catalog.json、后者读到（之前读 /tmp/header_catalog.json 会 FileNotFoundError）✓；注意：build_catalog 会重建 Docs/research_notes/A0/*，验证后已 git checkout 还原
F19 fixed by HASH(F19); 验证: python3 Tools/star/sod_reference.py --selftest → all self-checks passed, p*=0.30313018 u*=0.92745262 未变；导数分支是 Newton 的 Jacobian，收敛解与自检残差断言复现相同 ✓
（BACKLOG 状态已置 wip:lead-tools；每条修完追加结论行）
