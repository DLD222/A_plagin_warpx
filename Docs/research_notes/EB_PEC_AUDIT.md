# EB 表面 PEC 审计

## 1. 结论

原版基线 `7bcb44a` 也没有 EB 表面 PEC 的显式实现。

原版只有：

```text
域边界 PEC      Source/BoundaryConditions/WarpX_PEC.cpp
EB 几何掩膜     Source/EmbeddedBoundary/EmbeddedBoundaryInit.cpp
```

没有：

```text
ApplyPECtoEmbeddedBoundary
E_t = 0 on EB surface
B_n continuity on EB surface
```

## 2. 原版证据

基线提交：

```text
7bcb44a chore: 插件化架构工作区（源码拷贝，HEAD 0de0e26）
```

搜索：

```text
git grep -n "ApplyPECtoEB\|PECtoEB\|eb_pec" 7bcb44a -- Source
```

结果：无。

原版 EM 推进只传 EB 掩膜：

```text
Source/FieldSolver/WarpXPushFieldsEM.cpp
  EvolveE(..., m_eb_update_E_fp[lev], dt)
  EvolveB(... m_flag_info_face, borrowing, dt)
```

原版 EB 掩膜语义：

```text
完全被覆盖的边不更新
```

不是：

```text
cut cell 上 E_t 投影到零
cut cell 上 B_n 连续
```

## 3. 域边界 PEC 没有丢

当前：

```text
Source/Plugins/PicEm/Modules/BoundaryConditions/WarpX_PEC.cpp
```

与基线比较：

```text
域边界 PEC 的镜像逻辑仍在
仅 RZ 下 J 的反体积标定移到 finalization
```

这是有意修正，不是删除。

## 4. EB 文件没有丢

基线 EB 文件与当前路径：

```text
DistanceToEB.H             -> Source/Core/EmbeddedBoundary/DistanceToEB.H
Enabled.H                  -> Source/Core/EmbeddedBoundary/Enabled.H
WarpXFaceInfoBox.H         -> Source/Core/EmbeddedBoundary/WarpXFaceInfoBox.H
WarpXFaceInfoBox_fwd.H     -> Source/Core/EmbeddedBoundary/WarpXFaceInfoBox_fwd.H
ParticleBoundaryProcess.H  -> Source/Plugins/ParticleCore/Particles/EmbeddedBoundary/
ParticleScraper.H          -> Source/Plugins/ParticleCore/Particles/EmbeddedBoundary/
```

## 5. 全量文件审计

基线：

```text
Source 文件：777
```

当前：

```text
Source 文件：988
```

按文件名比对，基线中仅两个 basename 不在当前树：

```text
Source/Diagnostics/WarpXIO.cpp
Source/FieldSolver/MagnetostaticSolver/MagnetostaticSolver.cpp
```

对应当前实现：

```text
WarpXIO.cpp
  -> Source/Plugins/PicEm/RuntimeBridge/IO/WarpXCheckpoint.cpp

MagnetostaticSolver.cpp
  -> Source/Plugins/FieldSolver/Magnetostatic/MagnetostaticSolverImpl.cpp
```

两者都是改名/拆分，不是功能删除。

## 6. 仍然真实存在的缺口

```text
EB 表面 PEC（E_t=0、B_n 连续）未实现
```

这不是本项目的重构丢失，而是基线就没有。

## 7. 若要实现

需要新增：

```text
ApplyPECtoEmbeddedBoundary
```

并接入：

```text
WarpXPushFieldsEM.cpp
  EvolveE 后
  EvolveB 后
```

所需数据：

```text
EBFArrayBoxFactory
  boundary area
  boundary centroid
  boundary normal
```

并分别覆盖：

```text
3D
2D XZ
RZ
```

不能直接复用域边界 PEC 的镜像逻辑。
