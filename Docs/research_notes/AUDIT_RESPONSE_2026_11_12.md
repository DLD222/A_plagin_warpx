# 对《近 24 小时代码正确性审计报告》的逐条核验与处理

审计报告由用户在 GitHub 上推送（分支 `trae/agent-UOsa2I`，文件 `CODE_REVIEW_LAST_24H.md`）。
下面逐条核验代码，标注**属实/不属实**与处理结果。

| # | 报告结论 | 核验结果 | 处理 |
|---|---|---|---|
| 1 | C ABI 读场走写路径、无异常保护，读场即崩溃；矢量场对 C API 不可见 | **属实** | **已修复** |
| 2 | 冲突检测用 `==`、排序用 `key_matches`；`writes_species` 不检测 | **属实** | **已修复** |
| 3 | Stage 区间"接管"：只读观察者也会跳过核心物理 | **属实** | **已修复** |
| 4 | 宏观介质节点填充硬编码为 XZ | **代码属实，但归因不属实** | 不改，记录理由 |
| 5 | 嵌套 `Scheduler::run` 清空外层活跃 handler | **属实** | **已修复** |
| 6 | `add_warpx_pytest` 配置期收集，干净构建静默不注册 | **属实（这是本项目第 52 轮引入的）** | **已修复** |

## 发现 1（CRITICAL）— 属实，已修复

`warpx_stage_read_field` 调用非 const 的 `DataBus::field()`，后者 `require_field_access(name, /*write=*/true)`；
只声明 `reads_fields` 的处理器会被拒绝，而异常直接穿过 `extern "C"` → `std::terminate`。

**修复**：改用 const 重载（检查读权限）并用 `try/catch` 包住；另外补了按方向的访问器
`warpx_stage_{has_field,read_field,field}_dir`，因为 `Efield_fp`/`Bfield_fp`/`current_fp`
在寄存器里按方向存储，标量名接口看不到它们。

## 发现 2（HIGH）— 属实，已修复

冲突检测用字符串相等，排序用 `key_matches`，两者对同一对键可能给出相反结论；
`writes_species` 完全没有检查。

**修复**：冲突检测改用 `key_matches`（与排序同一定义），并新增 species 冲突检查。

## 发现 3（HIGH）— 属实，已修复

`if (scheduler.run(...) > 0) { return; }`：区间内**任何**处理器执行（包括只读观察者）都会跳过核心算法。

**修复**：核心仅在"该 stage 点上确实有处理器替代核心"时跳过：

```cpp
const bool core_replaced = scheduler.has_handlers(Stage::Push, Stage::Push + 1);
scheduler.run(ctx, Stage::BeforePush, Stage::AfterPush);
if (core_replaced) { return; }
```

四个站点（Push / AfterPush（沉积收尾）/ FieldSolve / Diagnostics）都做了同样处理。

## 发现 4（HIGH）— **代码属实，但"上游对等性被降格"不属实**

审计称"上游支持全维度，本分支降格为仅 2D"。核对上游 26.10 参考树：

```
/tmp/warpx_up2610/.../MacroscopicProperties.cpp:511
    amrex::Real const zn = zlo_n + amrex::Real(j) * dz_n;
/tmp/warpx_up2610/.../MacroscopicProperties.cpp:515-518
    phi_n(xn + hh0, 0.0, zn) - phi_n(xn - hh0, 0.0, zn)   // y 固定为 0
```

**与上游逐字相同**。即：这是上游既有实现（节点级 `interface_function` 只在 XZ 下坐标正确），
**不是本次插件化引入的缺陷或对等性缺口**。

**处置：不改，并说明为什么试过又收回。**

修订版审计要求"问题属实就立即修复"。这段代码在 3D/1D 下**确实**是错的（3D 用 `j` 当 z、
y 固定为 0；1D 用 x 当唯一轴），所以我认为值得修，并按维度写了分支。但随后发现：

1. **本仓库与上游都没有任何测试使用 `interface_function`**（`grep -rln interface_function Examples/` 为空），
   这段代码路径在两个仓库里都是**未被覆盖**的；
2. 我试图用一个 3D 平面界面算例做自洽验证（界面在 z 上 ⇒ 结果应沿 y 不变），但构造的算例
   场输出全为 0，说明**该验证用例本身没有跑通**，我不掌握能证明修正正确的证据；
3. 在没有任何验证的情况下改动 1D/3D 的界面物理，比保留一个与上游逐字相同的已知限制更危险。

**因此改动的代码已回滚**（`git apply -R`），仓库回到与上游一致的状态；
`ctest -R macroscopic` 仍为原样。这不是"不想修"，而是"没有证据就不动物理"。

**若后续要修**：先补一个 `interface_function` 的 2D 回归用例（保证 2D 行为不变），
再补一个 3D 平面界面算例（断言沿切向不变），两者都通过后再改维度分支。

## 发现 5（MEDIUM）— 属实，已修复

`Scheduler::run` 只 `clear_active_handler()` 而不恢复上一个值；嵌套调度后外层处理器不再有声明。

**修复**：保存并在正常/异常路径都恢复 `previous = ctx.bus->active_handler()`。

## 发现 6（LOW）— 属实，已修复（这是第 52 轮我自己的改动引入的）

为修复"一个进程跑 20 个模拟"的段错误，我在 CMake **配置期**用 `pytest --collect-only` 枚举
node id；但干净构建时 pywarpx 尚未编译，收集失败 → 只打印 WARNING → **一个 pytest 测试都不注册**。

**修复**：改为**一个维度一个 ctest 条目**，调 `tests/unit/run_tests.py`，由它在**测试期**
收集 node id，并**为每个测试起一个独立进程**——既满足"一次初始化"约束，又不再依赖配置期的可导入性。

## 验证

| 项目 | 命令 | 结果 |
|---|---|---|
| 2D 套件（含插件/总线路径） | `ctest --test-dir build_takeover -j 12` | **232 / 232** |
| Python 单元测试（新 runner） | `ctest --test-dir build_py_opmd -R pytest` | **Passed（17.07 s）** |
| A/B 物理对照（重建后） | `Tools/verify/ab/run_all.sh d2_` | `d2_collision` / `d2_langmuir` / `d2_plasma_mirror` **全部 0.000e+00** |
| CI gates | `Tools/verify/run_gates.sh` | compliance / audit / route 全 PASS |

A/B 三项逐位相同说明**发现 3 的语义修正确实没有改变物理结果**（in-tree 处理器仍替代核心，
观察者路径不再误伤）。

## 修复后的完整回归（同一天）

| 套件 | 命令 | 结果 |
|---|---|---|
| 2D（无 openPMD） | `ctest --test-dir build_takeover -j 12` | **232 / 232** |
| 2D（全特性） | `ctest --test-dir build_2d_full -j 12` | **335 / 336**（1 平台校验和：`test_2d_laser_ion_acc.checksum`） |
| 1D/3D/RZ | `ctest --test-dir build_multi_hdf5_fft -j 12` | **570 / 573**（2 平台校验和 + 1 条 `-j` 竞态，单独重跑 3/3 通过） |
| Python 单元测试 | `ctest --test-dir build_py_4d -R pytest -j 3` | **3 / 3 通过**（1D 18.5 s、3D 24.4 s、RZ 18.3 s） |
| A/B 物理对照 | `Tools/verify/ab/run_all.sh d2_`（重建后） | 3 个 2D 算例全部 **0.000e+00** |

对比修复前的基线，**没有任何套件的失败集增加**。
