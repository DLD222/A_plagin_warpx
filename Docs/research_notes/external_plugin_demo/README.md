# 仓库外插件验证（R2/R4 证据）

这里的 `third_party_plugin.c` **原样**在仓库外编译、加载、运行过。步骤：

```bash
# 1) 用普通 C 编译器编译（只包含公共 C ABI 头，不链接任何 WarpX 代码）
gcc -shared -fPIC -std=c11 -I<repo>/Source/Core third_party_plugin.c -o libthird_party_plugin.so

# 2) 加载进真实算例
WARPX_PLUGIN_PATH=$PWD/libthird_party_plugin.so \
  warpx.2d.MPI.OMP.DP.PDP.EB.QED <inputs>
```

观察到的输出（证明核心未做任何修改就能用）：

```
[third_party] init stage handler ran
[third_party] handler call 1 at stage AFTER_FIELD_SOLVE, dt=1.17933e-10
[third_party] finalize stage handler ran
```

插件做了三件"核心不知道"的事：声明一个自有场状态（`third_party_score`）、在
`AFTER_FIELD_SOLVE` 注册并执行处理器、读写命名标量。

## 这次验证发现的两个真实缺陷（均已修复）

1. **阶段边界点不可调度**：调度区间是半开区间 `[a, b)`，注册在 `AfterFieldSolve` /
   `AfterCollisions` / `Finalize` 上的处理器没有任何区间覆盖，因此永远不执行。
3. **初始化与收尾阶段同样不可调度**：`Init`/`BeforeInit`/`AfterInit`/`Finalize` 上没有
   任何调度调用。已补初始化三段与 Finalize 点的调度。
2. **可执行文件未导出 C ABI 符号**：外部插件 `dlopen` 时报
   `undefined symbol: warpx_state_request_field`，需要给可执行目标 `ENABLE_EXPORTS`。
