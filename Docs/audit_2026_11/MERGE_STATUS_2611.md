# 26.10 → 26.11 合并状态（2026-11-19，第 8 轮）

## 已完成
- `dependencies.json` 钉到 26.11（warpx / amrex / pyamrex），本地源码 `audit_build/deps/amrex-26.11`
- fork 与参考版四个维度（1D/2D/3D/RZ）**均已用 AMReX 26.11 重建**，运行日志显示 `Initializing AMReX (26.11)`
- 上游 149 处改动三方合并：73 处自动合入（8 处产生实质改动）
- 手工修复：
  - `BremsstrahlungFunc.H`：采用上游求值顺序（先按旧 arg 归一化再复位）
  - `JacobianFunctionMF.H`：RSPHERE 下 `pc_curl_curl_mlmg` 直接 abort（26.11 保护）
  - `WarpX::ErrorEst`：延迟加密改为显式开关 `warpx.deferred_refine`（默认 0，与上游一致）

## 27 处"冲突"的实际状态
逐条比对上游 26.11 新增行是否已出现在 fork：**24 处已等价**（fork 通过 RuntimeBridge/
插件抽象实现了同样的语义，例如 `ComputeDiagsMidStep(step, dt)`、`FinalizeRhoOnLevel` 的
调用顺序、PML 的 `GetEBUpdateEFlag_fp/cp`），**1 处补上保护**（JacobianFunctionMF），
剩 2 处是表面差异（`AlgMGPC.H`/`ExternLibPC.H` 的 PETSc 桥接，语义一致）。

结论：26.11 的源码级更新**已实质完成**，无需再逐行合入。
