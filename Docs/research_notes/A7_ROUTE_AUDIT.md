# A7 技术路线审计

审计对象：`A7_MASTER_PLAN.md`、`THREE_TASK_TECHNICAL_PATH.md` 与当前代码。

结论先说：

```text
A7 的工程顺序大体正确，
但完成判据过弱，
A7.1 把“去 WarpX.H”误当成“插件独立”，
A7.2 又用 WarpXKernel 把上帝类整体包了起来。
```

因此当前状态不能表述为“插件化完成”，只能表述为“A7.1 Core 去 WarpX 完成”。插件独立属于 A7.2。

---

## 1. A7 原设计回顾

```text
A7.0 源码完整
A7.1 Core 无 WarpX
A7.2 Kernel 拆分
A7.3 FieldSolver
A7.4 Particles
A7.5 Diagnostics
A7.6 Boundaries/EB
A7.7 其余模块
A7.8 插件 ABI
A7.9 删除 WarpX
```

原设计的隐含假设：

```text
先把 Core 与 WarpX 解耦，
再把 WarpX 内部拆成模块，
最后插件化。
```

这个顺序本身是合理的。

---

## 2. 审计发现

### 2.1 严重：A7.1 判据“Core 无 WarpX.H”不等于插件化

当前满足：

```text
Source/Core 没有 #include "WarpX.H"
Source/Core 没有 WarpX::
```

但：

```text
Source/Plugins 只有一个 PicEm
Diagnostics/Collision 仍在 PicEm/Modules 内部
模块之间直接 include 具体实现
```

所以满足的是：

```text
Core 去 WarpX 头文件
```

不满足：

```text
插件之间相互独立，仅通过 Core 主线耦合
```

这是定义层面的错误，不是实现遗漏。

### 2.2 严重：WarpXKernel 是 Facade，不是 Kernel 拆分

`IPicKernel` 引入后：

```cpp
m_kernel = create_pic_kernel(argc, argv);
```

但 `WarpXKernel` 内部仍然是：

```text
WarpX::Create
WarpX::InitData
WarpX::Evolve
WarpX::GetPartContainer
WarpX::GetMultiFabRegister
```

结果：

```text
Core 不再 include WarpX.H
但 WarpX 上帝类仍完整存在，只是被接口遮住
```

这不是 A7.2 的 Kernel 拆分，只是把依赖倒置了一层。

### 2.3 严重：模块化仍以“目录”代替“单位契约”

A7.3–A7.7 描述的是目录树：

```text
FieldSolver/FiniteDifference
Particles/Push
Diagnostics/Full
```

但验收没有规定：

```text
一个 Unit 是否只读写声明的字段
是否只依赖 StageContext
是否可被禁用后仍能编译
是否存在跨 Unit 直接指针
```

结果 241 个 CPP 多数只是移动了位置。

### 2.4 高：A7.8 的插件 ABI 与“可删除插件”矛盾

当前：

```text
Runtime 必须有一个 IPicKernel
WarpXKernel 在 PicEm 插件里
删除 PicEm 后 Runtime 无 kernel 可创建
```

因此：

```text
PicEm 不是可插拔插件，而是必需插件
```

与“删除某个插件后内核仍能编译”不一致。

### 2.5 高：A7.9 的删除条件没有覆盖 WarpX 类型扩散

即使 Core 无 WarpX.H，仍存在：

```text
IParticleContainerAccess 早期返回 WarpXParticleContainer
ParticleDiag / MovingWindow / Thermalizer / Collision 的向下转换
RuntimeConfig live getter 仍由 WarpX 数据填充
```

说明“Core 无 WarpX.H”不足以作为删除 WarpX 的前置条件。

### 2.6 中：四维门禁过弱

当前：

```text
2D 159/159
1D/3D/RZ 仅 smoke
FFT/openPMD 组合未全矩阵
```

A7.3–A7.7 每个 Unit 迁移后只要求“至少一个算例”，无法证明等价性。

### 2.7 中：源码完整性检查是文件计数，不是语义覆盖

`check_legacy_coverage.py` 只能证明：

```text
829 个旧文件都有对应
```

不能证明：

```text
旧文件被编译
旧物理路径仍可达
旧结果仍等价
```

---

## 3. 与两条根本约束的对照

### 插件化

| 约束 | 当前 |
|---|---|
| Core 与 Plugin 分离 | 部分满足 |
| Plugin 之间不直接依赖 | 不满足 |
| 仅通过 Core 主线耦合 | 不满足 |
| 删除插件可编译 | 不满足（PicEm 必需） |
| 外部插件可替换 | 不满足（IAmrPolicy） |

### 模块化

| 约束 | 当前 |
|---|---|
| 一功能一模块一 CPP | 部分满足 |
| 模块只依赖 StageContext | 不满足 |
| 字段/物种声明式读写 | 部分满足 |
| 模块可禁用/替换 | 部分满足 |
| 模块目录 = 物理功能 | 部分满足 |

---

## 4. 修正后的 A7 路线

### A7.1R：Core 契约收口

完成：

```text
Core 无 WarpX.H
RuntimeConfig 双轨
IParticleContainer 基类
HostServices 窄接口
```

未完成：

```text
Core 仍有一个必需的 IPicKernel 实现依赖
```

修正判据：

```text
Core 只依赖 IPicKernel 接口；
kernel 实现由插件注册，不在 Core 构造。
```

### A7.2R：插件边界先行，而不是继续拆 WarpX

先拆插件：

```text
Plugins/PicEm       粒子/场推进
Plugins/Diagnostics 诊断
Plugins/Collision   碰撞
Plugins/QED         QED
```

规则：

```text
Diagnostics 不 include Particles/*
Collision 不 include Diagnostics/*
所有跨插件访问走 DataBus/Stage 接口
```

验收：

```text
删除 Diagnostics 插件，PicEm 仍可编译运行
删除 Collision 插件，PicEm 仍可编译运行
```

### A7.3R：Kernel 单位化

把 WarpXKernel 内的调用拆成：

```text
Kernel/TimeStep
Kernel/LevelLifecycle
Kernel/FieldState
Kernel/SpeciesState
Kernel/IO
```

每个 Unit 只通过 `RuntimeContext`/`DataBus` 访问状态。

验收：

```text
WarpXKernel.cpp 不再是 200+ 行总调度
每个 Unit 可单独禁用
```

### A7.4R：物理模块单位化

按 A7.3–A7.7 继续，但增加：

```text
Unit 依赖图
字段/物种读写声明
禁用后编译/运行测试
```

### A7.5R：四维 A/B

每个 Unit 迁移：

```text
1D / 2D / 3D / RZ
```

至少覆盖：

```text
langmuir
collision
EB
ionization
```

并记录：

```text
迁移前 checksum / 迁移后 checksum
```

### A7.6R：删除 WarpX

删除条件改为：

```text
Core 无 WarpX 类型
所有插件无 WarpX 类型
WarpX.H/.cpp 删除后全插件可编译
四维 A/B 通过
```

---

## 5. 当前状态重新表述

准确表述：

```text
A7.0 完成
A7.1 Core 去 WarpX.H 完成
A7.1 插件独立 未完成
A7.2 Kernel 拆分 未开始（当前是 Facade）
A7.3–A7.7 目录/骨架
A7.8 部分
A7.9 未开始
```

---

## 6. 建议的下一步

优先级：

```text
1. 把 Diagnostics 从 PicEm 拆成独立插件
2. 把 Collision 从 PicEm 拆成独立插件
3. 加插件边界检查脚本
4. 再继续 Kernel 单位化
```

不建议继续做的：

```text
继续把 WarpXKernel 包大
继续用 static_cast 修编译
继续只做目录移动
```

---

## 7. 审计结论

```text
A7 的方向正确
A7.1 的“Core 无 WarpX.H”完成；插件独立条件移到 A7.2
A7.2 当前被 Facade 替代
A7.8 的“插件可删除”尚未成立
```

因此：

```text
不能宣布 A7.1 等价于插件化完成
不能宣布 A7.2 已开始
```

下一步必须先做插件边界，不是继续拆文件。
