# Tools_Tests 工作日志

claim: Tools_Tests F9,F15-F31 @lead-tools 2026-11-15T18:30
F29 fixed by HASH(F29); 验证: bash -n 语法 OK，CONDA="${CONDA_PREFIX:-...}" ✓
F24 fixed; 验证: 在 /tmp 下运行 Tools/A0/parse_headers.py 与 build_catalog.py → 前者写 Tools/A0/header_catalog.json、后者读到（之前读 /tmp/header_catalog.json 会 FileNotFoundError）✓；注意：build_catalog 会重建 Docs/research_notes/A0/*，验证后已 git checkout 还原
F19 fixed by HASH(F19); 验证: python3 Tools/star/sod_reference.py --selftest → all self-checks passed, p*=0.30313018 u*=0.92745262 未变；导数分支是 Newton 的 Jacobian，收敛解与自检残差断言复现相同 ✓
（BACKLOG 状态已置 wip:lead-tools；每条修完追加结论行）
