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

## 3b. 已打通：插件真的调用了 NewtonGravity（本轮）

`Source/Plugins/NewtonGravity/StarRuntimePlugin.cpp` 是本仓库内的一个 **MODULE** 库
（`star_runtime_plugin_3d`，在插件自己的 CMakeLists 里定义，**核心一行未改**），通过
`WARPX_PLUGIN_PATH` 加载。它用 ABI v7 的 `warpx_stage_geometry` 自建 `amrex::Geometry`，
然后调用**真正的** `NewtonGravitySolver::solvePotential`：

```
[star] initialised: star_rho set to 1e-3, waiting for the first step
[star] step 1: solved Poisson on a 4096-cell domain, phi_min = -2.46401e-13
[star] step 2: solved Poisson on a 4096-cell domain, phi_min = -2.15601e-13
[star] step 3: solved Poisson on a 4096-cell domain, phi_min = -1.84148e-13
```

复现：`Tools/verify/check_star_runtime_plugin.sh`。

过程中遇到并修掉的两个问题（都是真实的）：

1. **访问模式**：读 `star_rho` 必须用 `warpx_stage_read_field`，写要用 `warpx_stage_field`；
   DataBus 会把用错模式当场拒绝。这是总线契约在起作用，不是障碍。
2. **边界类型**：默认的 `PotentialBoundary::periodic` 与几何的非周期标志冲突，触发 AMReX
   `MLLinOp` 断言。改用 `PotentialBoundary::open`（非周期面 Dirichlet、周期方向保持周期）后正常。

## 3c. 崩溃的定位（本轮）：`ngrow > 0` 的插件自声明场

按"逐个关掉再跑"的方式二分，结论明确：

| 改动 | 结果 |
|---|---|
| 关掉 `solvePotential` | 仍崩溃（exit 134） |
| 关掉 `on_init` | 仍崩溃 |
| 关掉 handler 注册 | 仍崩溃 |
| 关掉 `on_schema`（不声明任何场） | 不崩溃，干净报错（exit 6，因为没有 `star_rho`） |
| `ngrow` 从 1 改成 0 | 不崩溃，干净报错（exit 6，求解需要 ghost） |
| 插件只链静态恒星层、AMReX 符号由宿主解析 | 仍崩溃 |
| 静态库加 PIC | 仍崩溃 |

所以**不是**求解器、不是 `on_init`、不是 handler、不是"第二份 AMReX"，而是
**`warpx_state_request_field(name, ncomp, ngrow>0, checkpoint)` 声明的自有场在退出时造成堆损坏**。
对照：仓库自带的 demo 插件也声明场，但 `ngrow = 0`，退出干净（exit 0）。

**但我随后做了一个决定性的对照，纠正了这个归因**：把**纯 C 的 demo 插件**（`star_plugin.c`，
不链接 AMReX）也加一个 `ngrow = 1` 的场，退出 **0**（干净）。

```
C demo 插件 + ngrow=1 场   -> exit 0
我的 C++ 插件 + ngrow=1 场 -> exit 134
```

所以**核心的 state registry / 场分配这条路径是好的**，崩溃是**我的 C++ 插件**特有的
（它链接了恒星层的静态库，AMReX 符号从宿主解析）。这与"第二份 AMReX"的假设也不同——
那种情况下 C 插件不会幸免。真正要查的是 C++ 插件里与 AMReX 相关的**静态初始化/析构**或
inline 符号在 .so 与宿主之间的重复。

**这条归因的修正是本轮最重要的产出之一**：如果停在"核心有缺陷"的结论上，就会把一条自己引入的
问题挂到别人账上。

**未修**：需要读 `StateRegistry` 的分配与释放路径（它给插件场的 ghost 是如何分配、
在 `Finalize` 之后由谁释放）。在此之前，`check_star_runtime_plugin.sh` 检查物理输出、
单独打印退出状态。

## 3c-old. 原始记录

同一算例**不加载插件**时干净退出（exit 0）；加载后物理全部正确完成（三步都解出势），
但**退出阶段**在 `std::deque<std::tuple<...>>` 析构里堆损坏（exit 134）。

已确认这是**我的插件引入的**，不是既有问题。下一步要定位的是：插件注册的 handler/状态在
`Finalize` 之后的析构顺序，或 `on_schema` 请求的场与退出时释放的交互。
**在修掉之前，`check_star_runtime_plugin.sh` 会打印 NOTE 并检查物理输出，而不假装退出正常。**

## 4. 当前状态与下一步

| 项 | 状态 |
|---|---|
| 加载 / 声明状态 / 每步调度 / 契约检查 | ✅ 实测通过（本节命令） |
| ABI 暴露几何 | ⬜ 需要上面那个入口 |
| 在插件里调用 NewtonGravity / FluidMoments | ⬜ 依赖上一条 |
| 两个插件库可供链接 | ✅ STATIC 变体（`warpx_*_static_<dim>`） |
| 在插件里调用 NewtonGravity | ✅ **实测三步**（见 §3b） |
| 退出阶段崩溃 | ⬜ 已知缺陷（§3c），正在定位 |

下一步的顺序是明确的：先加 `warpx_stage_geometry`（以及 OBJECT 库的 STATIC 变体），
再把 `star_plugin.c` 从"记录步进"升级为"解势 + 推流体"，最后用一个算例验证引力-流体耦合
在真实 WarpX 时间循环里成立。
