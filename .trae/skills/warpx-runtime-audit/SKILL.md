---
name: "warpx-runtime-audit"
description: "Audits WarpX's plugin Runtime layer against the route doc, reports bugs with file:line evidence, and pushes an MD report. Invoke when asked to review the latest WarpX code, list bugs, or verify earlier findings."
---

# WarpX Runtime Audit

Read-only audit of the pluginized `Source/Runtime/` layer in this WarpX fork.
Never edit source code during an audit; the only write is the report file.

## 0. Hard rules

1. **Find the real baseline first.** The user's latest code is often on `main`,
   while the worktree is on a side branch. Always run:
   ```bash
   git rev-parse --abbrev-ref HEAD
   git log --all --oneline -10
   git log main --pretty='%h %ad %s' --date=iso -5
   git merge-base main HEAD || echo "NO COMMON ANCESTOR"
   ```
   If `merge-base` is empty, the branches have unrelated histories — any
   `git diff HEAD..` check is meaningless for the user's code. Audit `main`.
2. **No compile claims.** This sandbox has no AMReX, conda, or `build/`.
   State explicitly in the report: "未编译、未跑 ctest；结论为静态取证".
3. **Every finding needs a line-level citation** from the audited revision:
   `git grep -n "<symbol>" <rev> -- <path>`. Never cite the working tree when
   the baseline differs.
4. **Declare scope honestly.** Say what was read (e.g. 43 files under
   `Source/Runtime/`) and what was not (upstream untouched files, Examples).
5. Do not invent YAML/JSON API names; read the actual file.

## 1. Workflow

### Step 1 — Baseline and route document

```bash
git ls-tree -r --name-only <rev> -- Source/Runtime tests/runtime
git ls-tree --name-only <rev> -- Docs/research_notes/ | sort
git show <rev>:Docs/research_notes/ROUTE_COMPLETE_2026-11-07.md | sed -n '1,120p'
```

Use the newest route doc found (it gets replaced each iteration; do not assume
a fixed filename). Extract its completion claims and invariants — those are the
audit criteria.

### Step 2 — Read the core in this order

1. `Runtime.H` / `Runtime.cpp` — lifecycle: construct, `initialize`, `evolve`,
   `finalize`, `sync_*`, teardown order.
2. `RuntimeContext.H` / `RuntimeData.H` / `TimeState.H` / `FrameState.H` /
   `RuntimeConfig.H` — ownership, copying, constness, statelessness.
3. `HostServices.H` + `WarpXHostServices.H/.cpp` — interface vs overrides;
   count `virtual` vs `override` and diff the names.
4. `PluginLoader.*` / `PluginABI.H` — ABI struct layout, version checks,
   dlopen flags, symbol lifetime.
5. `HandlerSpec.H` + `StagePoint.H` — ordering, conflict detection, duplicate add.
6. `DataBus.H/.cpp`, `FieldRegistry.H`, `SpeciesRegistry.H`, packages.
7. `Source/Evolve/WarpXEvolve.cpp` — whether stage handlers are actually
   invoked and whether legacy paths are double-executed.
8. `tests/runtime/*` + `CMakeLists.txt` — guards, working dirs, input keys,
   assertions vs configured diagnostics.

### Step 3 — Re-check the previous findings

Keep the prior report's IDs stable (H1..H7 / M1..M12 / L1..L7 or B1..).
For each: ✅ fixed / 🟡 partial / ❌ open, with evidence. Never silently drop an
ID; if it no longer applies, say why.

### Step 4 — Bug patterns to probe (high yield)

- Teardown/reset order with raw-pointer back-references (UAF).
- Stage functions with inconsistent precondition checks (`host` vs `data`).
- Registry/ordering code that sorts one container but validates another.
- `add()` paths lacking dedupe or `writes_species` conflict checks.
- "Stateless view" classes that still have a `FromLegacy()`/snapshot copy.
- Container sized by `maxLevel()` but filled by `finest_level` (column drift).
- C ABI structs with default member initializers or non-POD members.
- CMake loops registering dimension-specific inputs for all dimensions.
- Tests sharing a working directory, or asserting outputs they never configure.
- Duplicate includes, include-group order, `/tmp` hardcodes, unconditional
  RZ-only statics.

### Step 5 — Write the report

Create `Docs/research_notes/CODE_AUDIT_<YYYY-MM-DD>.md` with this skeleton:

```markdown
# 代码审计报告（<date>，<stage>）
> 只读审计，未修改源码。基线 <branch>@<sha>。未编译、未跑 ctest。
## 0. 审计范围声明
## 1. 前序问题状态表（✅ / 🟡 / ❌ + 证据）
## 2. 本轮新发现（高危 / 中危 / 低危，每条含 file:line）
## 3. 确认正确的设计点（避免只报坏消息）
## 4. 建议修复顺序（按风险/改动量）
## 5. 复核命令（可直接重跑）
## 6. 与既有文档的关系
```

Numbering: `B<n>` for new high/medium bugs, `L<n>` for style; keep prior IDs.
Each item must include: location, why it is wrong, the concrete failure mode,
and the minimal fix (line count if trivial).

### Step 6 — Commit and push

```bash
git config user.name "DLD222"
git config user.email "DLD222@users.noreply.github.com"
git add Docs/research_notes/CODE_AUDIT_<date>.md
git commit -m "docs: code audit <date> (<stage>)"
git push origin HEAD:refs/heads/trae/agent-iGh8PB
git ls-remote origin refs/heads/trae/agent-iGh8PB   # verify SHA
```

Commit only the report. Never sweep in `.screenshots/` or unrelated files.

## 2. Pitfalls learned

- A previous "0 fixed" verdict came from auditing the checked-out side branch.
  Always verify the user's branch before concluding "nothing changed".
- `main` is frequently re-created as a single squashed snapshot commit, so
  per-commit diffs are useless; compare content at the revision instead.
- `git grep -c` returns exit 1 on zero matches — that is a valid "0 hits"
  result, not a command failure.
- Alarm-style claims without line evidence get rejected; keep every claim
  reproducible via the §5 command block.
