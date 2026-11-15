# Tools_Tests 工作日志

claim: Tools_Tests F9,F15-F31 @lead-tools 2026-11-15T18:30
F17 fixed by HASH(F17); 5 个 stub 改名为 compile_only_*（含 <type_traits>），并在 tests/runtime/CMakeLists.txt 里显式 add_test。验证: ctest -R '^compile_only' -j4 → 5/5 Passed（此前这 5 个可执行文件从不出现在 ctest 里）✓
F27 fixed by HASH(F27); load_state 已 setdefault 全部任务，原先 271-275 的合并循环是死代码，删掉并注释说明 JSON/state 的分工。验证: route_tasks.py next / report 均 exit 0、输出不变 ✓
F26 fixed by HASH(F26); 改成棘轮门: 扫 Source/Plugins（排除 FieldSolver 插件）统计 FieldSolver includes 与具体求解器类型用量，与 field_solver_extraction_baseline.json 比较，只允许下降。验证: 正常 exit 0；反证: 往 PicEm/WarpX.cpp 追加 #include "FieldSolver/InjectedDefect.H" → exit 1，还原后 exit 0 ✓；--report 只打印
F25 fixed by HASH(F25); 验证: ctest --no-tests=error -R '^this_matches_nothing$' → exit 8（旧行为 exit 0），真实正则仍匹配 5 个测试 ✓；classify_ctest_log.py: 默认 exit 0，加 --fail-on candidate 后对含 candidate 的日志 exit 1（合成日志 /tmp/fake_ctest.log）✓
F29 fixed by HASH(F29); 验证: bash -n 语法 OK，CONDA="${CONDA_PREFIX:-...}" ✓
F24 fixed; 验证: 在 /tmp 下运行 Tools/A0/parse_headers.py 与 build_catalog.py → 前者写 Tools/A0/header_catalog.json、后者读到（之前读 /tmp/header_catalog.json 会 FileNotFoundError）✓；注意：build_catalog 会重建 Docs/research_notes/A0/*，验证后已 git checkout 还原
F19 fixed by HASH(F19); 验证: python3 Tools/star/sod_reference.py --selftest → all self-checks passed, p*=0.30313018 u*=0.92745262 未变；导数分支是 Newton 的 Jacobian，收敛解与自检残差断言复现相同 ✓
（BACKLOG 状态已置 wip:lead-tools；每条修完追加结论行）
