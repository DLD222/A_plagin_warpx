# WarpX Studio Fork 提交后正确性检查报告

- **审查对象**：`/workspace`（WarpX 的插件化非官方 fork，含 WarpX Studio 网页前端）
- **提交**：`51b17696e770aad6b4bae3142836a51470b456d8`（2026-11-20 10:29:25 +0800）
- **提交说明**：`WarpX Studio: 选择器门控、全量 HDF5 输出、EB/碰撞/诊断校验与网页交互更新`
- **审查方式**：全量静态阅读 + 跨文件执行路径追踪 + 只读复现（直接 import Studio 的 deck 模块，不写任何项目文件）
- **约束遵守**：未修改任何源码、配置或项目文件；仅新增本报告。
- **说明**：该仓库历史上只有一个压缩提交，因此“过去 24 小时提交”即当前快照；审查重点放在提交说明点名的四个区域（门控、HDF5 输出、EB/碰撞/诊断校验、网页交互），并对相关物理模块做交叉核对。

---

## 0. 项目目标与判断标准

根据仓库说明与 `Source/Core/Runtime/` 的实现，项目的根本目标是：

1. 保留 WarpX 的 PIC 物理与算法；
2. 形成“**精简核心 + 相互独立的插件**”结构，插件只通过核心定义的接口/总线接入；
3. 用户可以独立编写新的功能插件，而不需要改动其它插件或核心。

因此本报告按以下顺序判定缺陷：

- **物理正确性**：是否给出与解析理论/上游 WarpX 不一致的场、粒子或诊断结果；
- **静默性**：是否不报错、继续运行，但结果已经被悄悄改变；
- **系统性**：是否由多个文件共同违约（例如前端/后端语义不一致、生成器/解析器契约不一致）；
- **可扩展性**：是否破坏了插件独立接入的契约。

严重度：`致命`（结果错误或静默丢参数）、`高`（合法配置被阻断/必然 abort）、`中`（特定条件下错误或严重误导）、`低`（一致性问题）。

---

## 1. 结论摘要

| 编号 | 严重度 | 类别 | 位置 | 影响概述 |
|---|---|---|---|---|
| BUG-01 | **致命** | 静默丢参数 | `Deck.to_inputs` / `Deck.from_inputs` / HDF5 元数据 | Studio 专用键被写进 WarpX 输入；元数据中含 `#` 时，**物种、诊断、碰撞等后续输入行被整体吞掉**，仿真以空物种/空诊断继续运行 |
| BUG-02 | **高** | 解析器系统性缺陷 | `deck/model.py:140` | 注释符号 `#` 在引号内也被截断，配合奇偶引号续行逻辑导致跨行吞并 |
| BUG-03 | **高** | 校验假阴性（维度） | `deck/validate.py:313-320` 等 | RZ + 任意碰撞、RZ + `read_from_file` 均通过校验，但 WarpX 启动即 abort；RZ 起始模板默认 `n_rz_azimuthal_modes = 2`，必然触发 |
| BUG-04 | **中** | 校验假阳性 | `deck/validate.py:146-151, 198-200` | `3.e8*clight` 这类合法表达式被判为“`e` 不是数值”，错误级会拦截运行；与 README 声明的“未知名字=警告”相反 |
| BUG-05 | **中** | 前端/后端语义不一致 | `validate.py:19` vs `index.html:401-402` | 同一份 deck 在表单显示与后端校验中得到不同的可见参数集，可能隐藏必填物理参数 |
| PHYS-01 | **致命（已记录未解决）** | 物理正确性 | `HybridPICSolveE.cpp:1365-1384` 等 | 混合 PIC + 嵌入式边界时 `∇P_e/(e n_e)` 在切胞处被密度下限放大，E 场过冲并发散；本次新增的 EB 校验未识别 |
| OBS-01 | 低（功能完整性） | HDF5“全量” | `index.html:837-841` | RZ 下“全量 HDF5”实际不含 `T_<species>` / `part_per_cell_<species>`，与按钮语义不符 |

---

## 2. 详细发现

### BUG-01（致命，静默）：Studio 专用键 `studio.hdf5_metadata` 被写进 WarpX 输入，可吞掉整段 deck

#### 位置

- `Tools/warpx-studio/warpx_studio/deck/model.py:88-95`（`ENV_KEYS` 只包含两个键）
- `Tools/warpx-studio/warpx_studio/deck/model.py:106-112`（`to_inputs` 只跳过 `ENV_KEYS`）
- `Tools/warpx-studio/warpx_studio/web/static/index.html:802-816`（把元数据原文写入 `DECK.params["studio.hdf5_metadata"]`）
- `Tools/warpx-studio/warpx_studio/web/app.py:267-275`（服务端另行读取该键做后处理）
- `Tools/warpx-studio/warpx_studio/runner/job.py:40-69, 192`（运行结束后写 HDF5 属性）

#### 代码事实

`model.py` 的注释明确写着 Studio 专用键必须从输入文本中剥离，否则 WarpX 会警告：

```python
# Studio-only keys are not WarpX parameters ...
# They are stripped from the inputs text (WarpX would warn about an unused parameter)
ENV_KEYS = {
    "studio.solver.plugins": "WARPX_PLUGIN_PATH",
    "studio.plugin.star_profile": "WARPX_STAR_PROFILE",
}
```

但 HDF5 元数据同样只由 Studio 消费，却没有加入该集合：

```python
for key, value in self.params.items():
    if key in self.ENV_KEYS:
        continue
    lines.append(f"{key} = {value}")
```

而前端会把用户输入的**整段 JSON 原文**放进 `params`（`index.html:802,815`），因此 `studio.hdf5_metadata = {...}` 会原样进入 WarpX 输入文件。

#### 只读复现（真实执行结果）

```python
d = Deck(); d.set('max_step','10')
d.add_instance('<species_name>','electrons')
d.set_group_value('<species_name>','electrons','charge','-q_e')
d.set('studio.hdf5_metadata','{"note": "run #2"}')   # 合法 JSON，仅是注释里带 '#'
txt = d.to_inputs()
d2 = Deck.from_inputs(txt)
```

结果：

```text
R1 present in inputs: True
R1 reparsed params: ['max_step', 'studio.hdf5_metadata']
R1 groups: {}
R1 meta = '{"note": "run \nparticles.species_names = electrons\n\n# ---- species
```

即：`particles.species_names`、整个 `<species_name>` 组以及其后的诊断/碰撞组全部被并入了 `studio.hdf5_metadata` 的值里，重新解析后 `groups == {}`。

#### 根因（两个系统性缺陷叠加）

1. **专用键剥离不完整**：`to_inputs` 只按 `ENV_KEYS` 白名单剥离，遗漏 `studio.hdf5_metadata`（以及未来任何 `studio.*` 键）。
2. **注释剥离不看引号**：`deck/model.py:140`：

   ```python
   chunk = line if pending is not None else line.split("#", 1)[0]
   ```

   无论 `#` 是否在双引号内都会截断。截断后引号计数为奇数，`model.py:150-153` 的“跨行续接”逻辑会把下一行拼进来，直到引号重新配平——于是后续输入行被成批吞并。

   多行（美化后）的 JSON 即使不含 `#` 也不会被正确保留：复现显示元数据被截断成 `'{'`，HDF5 属性静默丢失（只是不触发吞并）。

#### 触发路径（不是极端操作）

正常网页流程即可触发：

1. 在 HDF5 面板填写元数据（文本框 `rows=4`，允许换行）；
2. 页面自动 `/api/preview`（`index.html:327`）把包含 `studio.hdf5_metadata` 的文本写回文本框；
3. 用户点击“解析” → `/api/parse`（`app.py:211-218`）→ `Deck.from_inputs`；
4. 之后所有物种/诊断定义消失。

#### 对仿真的影响（物理层面）

- `MultiParticleContainer` 在 `species_names` 为空时不会 abort：它只分配 0 个粒子容器（`Source/Plugins/ParticleCore/Particles/MultiParticleContainer.cpp:107-112`）。因此可能出现“运行成功但没有任何粒子”的**真空仿真**；场保持初始值/零，结果毫无物理意义。
- 诊断组也被吞掉时，不会有任何输出，或输出与用户设置不符；若保留了部分诊断引用已消失的物种，则在启动/输出阶段 abort。
- 因为 Studio 的“输入校验”默认关闭（`app.py:74`，`validate: bool = False`），这条路径可以完全不报错地走完。
- 元数据本身也被静默丢弃（多行 JSON 场景），与用户预期不符。

#### 修复方向（不在本报告实施）

- 按前缀剥离所有 `studio.*` 键，而不是维护不完整的白名单；
- `from_inputs` 的注释剥离必须识别双引号状态（并处理 `\"`），再决定是否截断 `#`；
- 序列化前对多行值做单行化（或使用 WarpX 支持的续行语法），保证文本往返稳定。

---

### BUG-02（高）：`from_inputs` 在引号内截断 `#`，导致跨行吞并（BUG-01 的根因，可独立触发）

#### 位置

`Tools/warpx-studio/warpx_studio/deck/model.py:135-163`

#### 机制

- `model.py:140` 先按 `#` 硬切，再在 `model.py:150` 统计 `"` 数量；
- 只要某个**合法值**内部带 `#`（例如 `some_key = "a#b"`、元数据 JSON 中的 `"run #2"`），引号计数变奇数；
- `pending` 会把后续行持续拼接，直到再遇到一个 `"`；期间所有 `key = value` 行都不会进入 `raw` 列表。

#### 影响

- 任意带引号且包含 `#` 的输入值都可能静默吞掉后续参数；
- 与 BUG-01 组合时，被吞掉的是物种/诊断/碰撞定义，直接改变仿真内容；
- 该缺陷也影响用户从外部导入的 WarpX 输入文件（`/api/parse`），不限于 HDF5 元数据。

---

### BUG-03（高）：RZ 维度约束没有进入门控/校验，新“碰撞校验”存在确定假阴性

#### 3.1 RZ + 任意碰撞

- WarpX 源码：`Source/Plugins/Collision/CollisionHandler.cpp:56-57`

  ```cpp
  WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_config->n_rz_azimuthal_modes()==1,
  "RZ mode `warpx.n_rz_azimuthal_modes` must be 1 when using the binary collision module.");
  ```

  注意该断言位于按碰撞类型分支**之前**，因此对任意 `type` 都会触发。

- Studio 的 RZ 起始模板：`Tools/warpx-studio/warpx_studio/deck/model.py:246`

  ```python
  deck.set("warpx.n_rz_azimuthal_modes", "2")
  ```

- 后端校验中与碰撞有关的检查只有互斥项：`Tools/warpx-studio/warpx_studio/deck/validate.py:313-320`，没有 RZ 维度检查。

- 只读复现（RZ 模板 + `pairwisecoulomb`）：

  ```text
  R3 modes: 2 issues: []
  ```

  即 Studio 认为完全合法，但 WarpX 启动即 abort。

#### 3.2 RZ + `read_from_file`

- `Source/Plugins/ParticleCore/Particles/Initialization/VelocityProperties.cpp:88-92`：
  `read_from_file` 在 RZ/RCYLINDER/RSPHERE 下直接 abort。
- `Source/Plugins/ParticleCore/Particles/Initialization/TemperatureProperties.cpp:105, 212`：温度/热速度的 `read_from_file` 同样在 RZ 下 abort。
- Studio 的目录数据与门控脚本没有按 `geometry.dims` 排除该选项（目录只提供选项，门控仅按分布类型显示路径字段）。

- 只读复现（RZ + `maxwellian_u_mean_distribution_type = read_from_file`）：

  ```text
  R4: []
  ```

#### 影响

- 用户在 RZ 项目中添加“物理上需要”的碰撞或从文件读取速度分布，会被 Studio 判定通过，随后 WarpX 在初始化阶段 abort；浪费运行，且错误信息只出现在 WarpX 日志里。
- 这是“新碰撞校验”的系统性缺口：校验以参数选择器为维度，却没有把**几何维度**作为一等约束。目录中只有约 9/369 条门控规则引用了 `geometry.dims`。

#### 修复方向

- 在 `validate()` 中增加维度约束表（碰撞、`read_from_file`、部分诊断类型等）；
- 门控规则对 RZ 禁用不支持的选项，或在 UI 中显式标注并阻止运行。

---

### BUG-04（中）：数值表达式校验误报，合法表达式被错误级拦截

#### 位置

`Tools/warpx-studio/warpx_studio/deck/validate.py:112-152, 189-200`；前端拦截 `index.html`（`severity === "error"` 时不运行）。

#### 事实

- `validate.py:146-147` 的注释声称会正确处理 `1.e19`：

  ```python
  # 先把数字字面量整体去掉：否则 "1.e19" 里的 "e19" 会被当成标识符
  stripped = re.sub(r"\d*\.?\d+(?:[eEdD][+-]?\d+)?", " ", text)
  ```

  但该正则的尾数部分要求小数点后至少一位数字，`1.e19` 只会匹配到 `1`，剩下的 `e19` 被当成标识符 `e`。

- 只读复现（2D 模板，密度写成合法 C/WarpX 表达式）：

  ```text
  density = 3.e8*clight
  R2: [('error', 'electrons.density',
       '`e` 不是数值，也不是内置常量或 my_constants 里定义过的常量（需要的话先写 my_constants.e = ...）')]
  ```

- `validate.py:198-200` 把该问题记为 `severity="error"`；前端在错误级时阻止运行。这与 `README.md:234` 声明的“未知名字 = 警告”直接矛盾。

#### 影响（物理/使用层面）

- 合法的物理表达式（尤其是使用 `1.e19`、`3.e8` 这种省略小数位的科学计数法）会被判定为错误，无法运行；
- 用户为绕过误报会关闭“输入校验”（默认本来就是关闭的），从而同时失去对真实错误（碰撞互斥、必填项、诊断格式等）的保护；
- 这只是**校验层**的误报，不会改变已经生成的输入文件，但它直接阻碍正确物理配置的落地。

#### 修复方向

- 用 C/C++ 数字字面量的完整正则（允许 `1.`、`1.e19`、`.5e-3` 等）；
- 未知标识符按 README 的契约降为警告，或与 WarpX 的 parser 可用名字表对齐。

---

### BUG-05（中）：前端与后端对“真值”的定义不一致，同一 deck 的可见参数集不同

#### 位置

- 后端：`Tools/warpx-studio/warpx_studio/deck/validate.py:19`

  ```python
  TRUE_WORDS = {"1", "true", "t", "yes", "y", "on"}
  ```

- 前端：`Tools/warpx-studio/warpx_studio/web/static/index.html:401-402`

  ```js
  if (rule.op === "truthy") return ["1", "true", "t", "yes"].includes(value);
  if (rule.op === "falsy")  return !["1", "true", "t", "yes"].includes(value);
  ```

- 后端内部也不统一：`validate.py:287, 327` 的 QED/CRR 判定又只接受 `("1","true","t","yes")`。

#### 影响

- 对 `y`、`on` 这类值，表单显示与后端校验结论相反：
  - `truthy` 门控下，后端认为参数可见/必填，前端却隐藏 → 用户可能看不到必填物理参数；
  - `falsy` 门控下，前端显示本不该生效的参数 → 用户误以为它会影响物理，实际被 WarpX 忽略或导致 abort。
- 该问题与“选择器门控”直接相关：门控规则是生成 deck 的语义来源，前端/后端不一致会让“门控即事实”的契约失效。
- README 声称前后端规则一致并有回归测试，但当前实现不满足。

#### 修复方向

- 把真值集合下沉为唯一的后端定义并通过 `/api/catalog` 下发，或前端复用同一份 JSON；
- 与 WarpX `ParmParse` 实际接受的布尔语法对齐，非法值应显式报错而不是二义性地当作真/假。

---

### PHYS-01（致命，已记录但仍未解决）：混合 PIC + 嵌入式边界（EB）在切胞处 `∇P_e/(e n_e)` 发散

> 说明：该问题在仓库自带的 `Source/Core/Runtime/WORK_SUMMARY.md:10,28-40` 中已被记录为“未解决”，并非本次提交新引入；但本次提交声称增加了“EB 校验”，实际并没有识别或拦截该组合，因此仍应作为**确认存在的关键物理缺陷**报告。

#### 位置

- E 场求解（Cartesian）：`Source/Plugins/FieldSolver/FiniteDifference/HybridPICSolveE.cpp:1365-1384`
- E 场求解（RZ）：同文件 `863-877`、`942-955`、`1012-1026`
- 电子压力闭合：`Source/Plugins/PicEm/Modules/HybridPIC/HybridPICModel/HybridPICModel.cpp:1250-1283`
- 已知问题记录：`Source/Core/Runtime/WORK_SUMMARY.md:10,28-40`

#### 物理机制

1. 电子压力按多方闭合计算（`HybridPICModel.cpp:1280-1283`）：

   ```cpp
   const Real ne = std::max(rho(i,j,k), rho_floor) / PhysConst::q_e;
   const Real Te_joule = elec_temp * std::pow(ne/n0_ref, gamma_minus_1);
   Pe(i,j,k) = ne * Te_joule;
   ```

   在 EB 覆盖/切胞内 `rho < rho_floor`，`Pe` 被压到与 `n_floor` 同量级。

2. 广义欧姆定律中（`HybridPICSolveE.cpp:1369-1384`）：

   ```cpp
   const Real grad_Pe = include_grad_pe ? UpwardDx(Pe, ...) : 0;
   const auto rho_val_limited = (rho_dec < rho_floor)
       ? rho_floor : std::max(rho_val, rho_floor);
   Ex(i,j,k) = (enE_x - grad_Pe) / rho_val_limited;
   ```

   `grad_Pe` 使用跨越 EB 界面的差分（界面外侧为等离子体 `Pe ~ n0 T0`，内侧被压到 `n_floor`），而分母被下限钳到 `rho_floor = q_e * n_floor`。于是

   ```text
   |E| ~ |∇P_e| / (e n_floor)  ∝ 1/n_floor
   ```

   在 `n_floor << n0`（典型 1e-6 n0 量级）时被放大数个数量级。

3. `holmstrom_vacuum_region` 只在**真空判定**的节点把 E 置零（`1365-1366`）；对于 `rho_dec >= rho_floor`、但 `rho_val` 很小的部分填充切胞，仍走除法路径，放大依旧存在。

#### 影响

- 仓库记录显示：E 场在 step 680→800 从约 2.2e5 过冲到 3.1e6（约 14 倍），每步增长约 0.45%/step，约 0.8 ns（step 800）触发 NaN 中止（`WORK_SUMMARY.md:32-38`）。
- **在 abort 之前，场已经被系统性放大**，因此任何“混合 PIC + EB”的中间结果都不可信；这不是简单的数值不稳定性，而是边界切胞处压力梯度的非物理闭合。
- 该问题与官方 WarpX 的对应实现表现一致（同文档说明），因此属于“ fork 尚未补齐原版限制/修复”的范畴。

#### 建议（不实施）

- 对切胞的 `Pe` 使用邻居平均/单侧梯度，使 `∇P_e` 与 `n_e` 的钳制一致；
- 或在 EB 界面处把 `Pe` 梯度投影到切胞有效体积，并对 `1/n_e` 使用与压力重建相同的有效密度；
- 至少在 Studio 的 EB/碰撞校验中对“hybrid + EB”给出显式警告，避免用户把已发散的运行当成物理结果。

---

## 3. 插件化 / 模块化评估

审查了 `Source/Core/Runtime/*`、`Source/Plugins/PicEm/RuntimeBridge/*` 以及插件间 include 关系，结论：

1. **核心总线确实存在且是接口化的**
   - `Runtime.cpp` 构造 `DataBus` 与 `PluginHost`，读取 `WARPX_PLUGIN_PATH`；
   - `PluginLoader.cpp` 支持冒号分隔的多个 `.so`，通过 C ABI 的 `register_handlers` 注册；
   - 核心提供 `IHybridPICModel.H`、`ICollisionHandler.H` 等抽象接口，插件通过接口接入。

2. **未发现核心硬编码具体插件名**
   - 未在 `Source/Core` 中发现以插件名（如 `hybrid_resistive_drag`、`star_gravity`）为条件的分支；
   - PicEm 之外的插件没有直接 include PicEm 内部实现头（跨插件 include 未发现）。

3. **存在一处“插件内部工厂硬编码”的可扩展性限制（非核心违约）**
   - `Source/Plugins/Collision/CollisionHandler.cpp:15-29` 直接 include 所有碰撞实现头；
   - `CollisionHandler.cpp:59-140` 以 `type` 字符串硬编码创建具体碰撞模型。
   - 这意味着“新增一种碰撞模型”需要修改 Collision 插件自身的工厂与 CMake，而不是以独立 `.so` 插入。若项目目标是“任何功能都能作为独立插件、不改其它代码”，这里是一个明确缺口；若碰撞模型被视为同一个 Collision 插件内的子模块，则属于可接受的内部设计。建议在插件规范中明确这一粒度。

4. **Studio 与 C++ 插件总线是两层独立系统**
   - Studio 通过 `studio.solver.plugins → WARPX_PLUGIN_PATH` 选择插件，本身不参与 C++ 总线；
   - 这层用 Python/JS 重新实现了参数门控语义（见 BUG-05），是当前“同一规则、两套实现”风险的主要来源。

---

## 4. 已交叉核对、未发现问题的部分

为避免只报问题而缺少覆盖面，以下路径已逐项核对：

- **诊断字段名合法性**：RZ / 1D / 2D / 3D 的 `fields`、`particle_vars` 与 `FullDiagnostics.cpp` 的 functor 分支逐一对照（`Er/Et/Ez`、`Br/Bt/Bz`、`jr/jt/jz`、`divE/divB`、`part_per_cell` 等均存在对应分支）。
- **RZ 下 `T_<species>` / `part_per_cell_<species>` 的排除**：确认真实存在 `nComp()` 与 `m_varnames.size()` 的断言（`FullDiagnostics.cpp:629-634`），Studio 在 RZ 中避开它们是正确的工作区，不是新 bug。
- **碰撞类型字符串**：`tools_gate_collisions.py` 的取值与 `CollisionHandler.cpp` 的 `type` 分派一致。
- **Maxwellian 双分布断言、CRR + Boris 断言**：`validate.py` 的检查与 `TemperatureProperties.cpp` / `PhysicalParticleContainer.cpp` 的 abort 条件一致。
- **RZ 边界默认值**：`boundary.field_lo/hi = "none periodic"` 与上游 RZ 示例一致。
- **张量介质求解器**：`MacroscopicEvolveE.cpp` 的 Yee 旋度分量、`chi^-1` 行、矩阵指数 `M1/M2` 的推导（`eps^{-1/2}` 变换与 `(1-exp(-B dt))/B`）核对未发现符号/量纲错误。
- **未被审查覆盖**：未运行 `ctest`/checksum（会写 build 目录），未执行真实 WarpX 仿真；本报告的所有复现均为只读的 Python 层调用。

---

## 5. 优先级建议

1. **立即修复 BUG-01 + BUG-02**：这是唯一能让“物理上完全不同的输入”静默通过、且不产生任何错误信息的路径；应先在序列化/解析层加往返测试（含 `#`、`\"`、多行 JSON）。
2. **修复 BUG-03**：把 `geometry.dims` 纳入校验/门控；至少覆盖“RZ + 碰撞”“RZ + read_from_file”“RZ 模式数”三项。
3. **修复 BUG-04/05**：校验误报与前后端真值集不一致会诱导用户关闭校验，从而放大其它风险。
4. **PHYS-01** 属于物理算法层，建议单独立项；在修复前，Studio 应对 hybrid+EB 给出明确警告/禁止。

---

## 附录 A：只读复现脚本

以下脚本仅调用 Studio 的纯 Python 模块，不写项目文件：

```python
import sys, json
sys.path.insert(0, '/workspace/Tools/warpx-studio')
from warpx_studio.deck.model import Deck, minimal_deck
from warpx_studio.deck.validate import validate

cat = json.load(open('/workspace/Tools/warpx-studio/warpx_studio/catalog/data/catalog.json'))

# BUG-01 / BUG-02
d = Deck(); d.set('max_step', '10')
d.add_instance('<species_name>', 'electrons')
d.set_group_value('<species_name>', 'electrons', 'charge', '-q_e')
d.set('studio.hdf5_metadata', '{"note": "run #2"}')
d2 = Deck.from_inputs(d.to_inputs())
assert d2.groups == {}                      # 物种组被吞掉
assert 'particles.species_names' not in d2.params

# BUG-03
d = minimal_deck('rz')
d.add_instance('<collision_name>', 'c1')
d.set_group_value('<collision_name>', 'c1', 'type', 'pairwisecoulomb')
assert validate(d, cat) == []               # 校验通过，但 WarpX 会 abort

# BUG-04
d = minimal_deck('2d')
d.set_group_value('<species_name>', 'electrons', 'density', '3.e8*clight')
assert any('不是数值' in i['message'] for i in validate(d, cat))
```

## 附录 B：关键文件与行号索引

| 文件 | 行号 | 说明 |
|---|---|---|
| `Tools/warpx-studio/warpx_studio/deck/model.py` | 88-95 | `ENV_KEYS` 白名单 |
| 同上 | 106-112 | `to_inputs` 只跳过白名单 |
| 同上 | 124-163 | `from_inputs` 的 `#` 截断与续行 |
| 同上 | 246 | RZ 默认 `n_rz_azimuthal_modes = 2` |
| `Tools/warpx-studio/warpx_studio/deck/validate.py` | 19 | `TRUE_WORDS` |
| 同上 | 112-152 | 数值表达式校验（`1.e19` 正则缺陷） |
| 同上 | 189-200 | 未知名字被记为 `error` |
| 同上 | 313-320 | 碰撞校验仅覆盖 super/subcycle |
| 同上 | 405-414 | 诊断格式校验（`format` 被 lower） |
| `Tools/warpx-studio/warpx_studio/web/static/index.html` | 401-402 | 前端真值集合 |
| 同上 | 802-816 | 元数据原文写入 `params` |
| 同上 | 837-841 | RZ 下“全量”排除 `T_`/`ppc_` |
| `Tools/warpx-studio/warpx_studio/web/app.py` | 74 | 校验默认关闭 |
| 同上 | 201-218 | `/api/preview` 与 `/api/parse` |
| 同上 | 267-275 | 服务端读取 `studio.hdf5_metadata` |
| `Source/Plugins/Collision/CollisionHandler.cpp` | 56-57 | RZ 模式数断言 |
| `Source/Plugins/ParticleCore/Particles/Initialization/VelocityProperties.cpp` | 88-92 | RZ 禁止 `read_from_file` |
| `Source/Plugins/ParticleCore/Particles/Initialization/TemperatureProperties.cpp` | 105, 212 | 温度/热速度的 RZ 禁止 |
| `Source/Plugins/FieldSolver/FiniteDifference/HybridPICSolveE.cpp` | 1365-1384（及 863-877/942-955/1012-1026） | `∇P_e / rho_floor` 放大 |
| `Source/Plugins/PicEm/Modules/HybridPIC/HybridPICModel/HybridPICModel.cpp` | 1250-1283 | 多方压力闭合 |
| `Source/Core/Runtime/WORK_SUMMARY.md` | 10, 28-40 | 已记录的 hybrid+EB 未解决问题 |
| `Source/Plugins/Diagnostics/FullDiagnostics.cpp` | 629-634 | RZ functor 分量数断言 |

---

**结论**：本次提交的四个新区域中，至少存在 4 个可复现的缺陷（BUG-01～04）和 1 个跨层语义不一致（BUG-05）；其中 BUG-01 是唯一能够在不产生任何错误信息的情况下把物理配置改成另一套的“静默致命”问题。物理层另确认 hybrid+EB 的已知发散仍未解决（PHYS-01）。
