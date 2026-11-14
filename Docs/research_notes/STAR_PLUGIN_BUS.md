# 恒星插件接入总线：走通了什么，卡在哪里

目标条目 §15.4 要求 NewtonGravity 与 FluidMoments **可由输入卡启用**，且**不改核心**。这里的
结论分两半：**加载与调度路径已实测走通**；**真正的求解器还差一个 ABI 入口**。

## 1. 已实测走通的部分

`external_plugin_demo/star_plugin.c` 是一个**仓库外、用 gcc 编译、不链接任何 WarpX 代码**的
恒星形状插件：它通过 `warpx_state_request_field` 声明一个 7 分量的 `star_state`
（ρ、φ、动量 3 分量、组分槽），在 `AFTER_FIELD_SOLVE` 注册处理器，并在每步拿到 field 指针与 dt。

```bash
gcc -shared -fPIC -std=c11 -I<repo>/Source/Core star_plugin.c -o libstar_plugin.so
WARPX_PLUGIN_PATH=$PWD/libstar_plugin.so warpx.1d <inputs with max_step=3>
```

实测输出：

```
[star] step 1 at AFTER_FIELD_SOLVE, dt=2.60597e-11, total advanced=2.60597e-11 s
[star] step 2 at AFTER_FIELD_SOLVE, dt=2.60597e-11, total advanced=5.21194e-11 s
```

**核心未做任何修改**。这证明了两件事：

1. 一个恒星插件需要的**状态**（多分量场、跨 regrid 由核心持有）和**调度点**（每步一次、
   在电磁场求解之后）在现有总线上都具备；
2. DataBus 的契约是**真执行的**——第一次运行时它拒绝了未在 `writes_fields` 里声明的写访问：
   `handler 'star_advance' accessed field 'star_state' without declaring it as written`。
   这正是 F-09 那类"静态顺序决定行为"问题的反面：总线会把越权当场报出来。

## 2. 卡在哪里：ABI 没有几何

`NewtonGravitySolver::solvePotential` 需要 `amrex::Geometry`（域边界、网格间距、周期标志）来
构造 `MLABecLaplacian`；`FluidMoments::advanceSweep` 也需要几何来取 `dx` 与周期条件。
而 `PluginCApi.H` 暴露的只有：

| 类别 | 入口 |
|---|---|
| 步进 | `warpx_stage_step/time/dt` |
| 场 | `warpx_stage_{has,read}_field[_dir]`、`warpx_stage_field[_dir]` |
| 标量 | `warpx_stage_scalar`、`warpx_stage_set_scalar` |
| 状态 | `warpx_state_request_field` |

**没有任何入口返回 Geometry**，也没有层数。插件拿到的是 `MultiFab*`，但拿不到它能用来
构造算子的域信息——所以"在插件里调用真正的求解器"这条路，缺的不是权限而是数据。

## 3. 需要的那一个入口（增量为 ABI v7，不改派发逻辑）

```c
/* 某一层的几何参数：索引域、物理域、网格间距、周期标志。求解器插件据此自建 Geometry。
   成功返回 1。 */
int32_t warpx_stage_geometry (WarpXStageContext* ctx, int32_t lev,
                              int32_t lo[3], int32_t hi[3],
                              double prob_lo[3], double prob_hi[3], double dx[3],
                              int32_t periodic[3]);
```

它是**纯读取**、与现有 `warpx_stage_*` 同构，实现只需把 `Geom(lev)` 的四个量拷出去，
不触碰调度、注册或契约检查。加上它之后，`star_plugin` 里的处理器就可以：

```c
amrex::Geometry geom = build_geometry_from(lo, hi, prob_lo, prob_hi, dx, periodic);
NewtonGravitySolver::solvePotential(rho, phi, geom, 1e-13);
```

并进一步推流体步。这属于**能力补充**而不是架构改动：插件仍然通过契约访问，核心仍然不认识
任何一个恒星类型。

## 4. 当前状态与下一步

| 项 | 状态 |
|---|---|
| 加载 / 声明状态 / 每步调度 / 契约检查 | ✅ 实测通过（本节命令） |
| ABI 暴露几何 | ⬜ 需要上面那个入口 |
| 在插件里调用 NewtonGravity / FluidMoments | ⬜ 依赖上一条 |
| 两个插件库可供链接 | ⬜ 目前是 OBJECT 库，需同时产出 STATIC 变体 |

下一步的顺序是明确的：先加 `warpx_stage_geometry`（以及 OBJECT 库的 STATIC 变体），
再把 `star_plugin.c` 从"记录步进"升级为"解势 + 推流体"，最后用一个算例验证引力-流体耦合
在真实 WarpX 时间循环里成立。
