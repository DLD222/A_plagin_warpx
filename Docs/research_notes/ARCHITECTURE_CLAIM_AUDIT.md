# 插件化目标与实现一致性审计

> 审计对象：`main`，HEAD `2050e6a` 之后的工作树。  
> 结论先行：**当前项目是“WarpX 内核 + 阶段插件外壳”，不是“删除 WarpX 后仍可运行的插件化仿真器”。**

## 1. 五个问题的直接回答

| # | 问题 | 结论 |
|---|---|---|
| 1 | 是否包含原版 WarpX 全部功能 | **源码层面包含**（未删除 WarpX），但本机只验证 2D CPU；1D/3D/RZ 未通过统一构建，GPU 未验证 |
| 2 | 用户能否只写 CPP 插件接入新物理 | **部分可以**：阶段 handler、诊断、源项、碰撞模型可以；新场求解器/推进器/新维度仍需改内核 |
| 3 | 是否按物理/逻辑功能清晰分模块 | **部分**：Runtime/Stage/Registry 清晰；`WarpX` 仍是上帝类，`PicFieldSolveHandler` 仍按枚举分发 6+ 算法 |
| 4 | 删除原版 WarpX 后原算例是否同结果 | **不能**：`Runtime` 直接 `#include "WarpX.H"`、`WarpX::Create()`，所有算例依赖 WarpX |
| 5 | 输入输出是否与原版相同 | **默认路径基本相同**；checkpoint 增加 `field_schema.txt`，插件加载增加 `WARPX_PLUGIN_PATH`；PICMI 未构建验证 |

## 2. 关键证据

### 2.1 Runtime 仍拥有 WarpX

```cpp
// Source/Runtime/Runtime.cpp
#include "WarpX.H"
m_warpx = WarpX::Create();
```

```cpp
// Source/Runtime/Runtime.H
std::unique_ptr<WarpX> m_warpx;
```

删除 `Source/WarpX.H/.cpp` 后，`Runtime` 无法编译。

### 2.2 内置包只有三个 handler

```cpp
// Source/Runtime/PicEmPackage.H
host.add(std::make_shared<PicPushHandler>());
host.add(std::make_shared<PicFinalizeHandler>());
host.add(std::make_shared<PicFieldSolveHandler>());
```

- `PicPushHandler` 调 `host->pushParticles()`；
- `PicFinalizeHandler` 调 `host->finalizeDeposition()`；
- `PicFieldSolveHandler` 调 `host->fieldSolve()`。

真正的 Yee/Boris/PSATD/EB/QED 仍在 `WarpX` 内。

### 2.3 当前构建矩阵

```text
build:        2D CPU OMP, EB/QED ON, FFT/OFF, openPMD/OFF, Python/OFF
build_fft:    2D CPU OMP, FFT ON
build_hdf5:   2D CPU OMP, openPMD + parallel HDF5, 159/159 PASS
build_multi:  1D/3D/RZ 统一构建，失败
```

`build_multi` 实测错误：

```text
EmbeddedBoundaryInit.cpp: IntVectND<3> 构造只给 2 个分量
MomentsSolver/FV/MomentFVNDDriver.H: 找不到 Equations/MomentsConfig.H
```

说明 1D/3D/RZ 尚未达到“全部功能可构建”。

## 3. 26 项功能审计

| # | 功能 | 源码存在 | 插件化 | 本机验证 |
|---|---|---|---|---|
| 1 | 1D/2D/3D/RZ | ✅ | ❌ | 仅 2D |
| 2 | 所有场求解器 | ✅ | ❌ 枚举分发 | 2D 部分 |
| 3 | 背景固定电荷/中性背景 | ✅ | ❌ | 未单独验证 |
| 4 | 场/电流滤波 | ✅ | ❌ | 2D 部分 |
| 5 | 粒子排序 | ✅ | ❌ | 未单独验证 |
| 6 | checkpoint/restart | ✅ | 部分 schema 驱动 | 2D PASS |
| 7 | PICMI/pywarpx | ✅ | ❌ | 未构建 |
| 8 | 多物种独立配置 | ✅ | ❌ | 2D 部分 |
| 9 | 轨迹诊断 | ✅ | ❌ | 未单独验证 |
| 10 | 在线标量诊断 | ✅ | ❌ | 2D 部分 |
| 11 | RZ 轴修正 | ✅ | ❌ | 未构建 |
| 12 | 粒子变薄 | ✅ | ❌ | 未单独验证 |
| 13 | 多种注入 | ✅ | ❌ | 2D 部分 |
| 14 | 滑动窗口/降采样诊断 | ✅ | ❌ | 2D 部分 |
| 15 | 负载均衡 | ✅ | ❌ | HDF5 2D PASS |
| 16 | EB cut-cell | ✅ | ❌ | 2D PASS |
| 17 | Silver-Müller | ✅ | ❌ | 未单独验证 |
| 18 | 热发射边界 | ✅ | ❌ | 未单独验证 |
| 19 | 固定外场/纯追踪 | ✅ | ❌ | 2D 部分 |
| 20 | 光子/QED | ✅ | ❌ | 2D PASS |
| 21 | MPI/OMP/GPU | ✅ | ❌ | MPI/OMP 2D；GPU 无 |
| 22 | moving window | ✅ | ❌ | 2D 部分 |
| 23 | 切片/子区域输出 | ✅ | ❌ | 2D 部分 |
| 24 | 库仑碰撞 | ✅ | 部分 | 2D PASS |
| 25 | 电离链 | ✅ | ❌ | 2D PASS |
| 26 | 解析外场 | ✅ | ❌ | 2D 部分 |

“源码存在”不等于“已验证”；“插件化”列只有 `PicPush/PicFinalize/PicFieldSolve/Diagnostics/Collision` 阶段外壳。

## 4. 用户插件能力边界

### 可以

- 在已有阶段注册 `IHandler`；
- 读写已注册字段/物种；
- 增加源项、诊断、碰撞模型、参数化外场；
- 通过 `WARPX_PLUGIN_PATH` 加载 `.so`。

### 不可以

- 新增场求解器算法而不改 `PicFieldSolveHandler` / `WarpX`；
- 新增粒子推进器而不改 `WarpX`；
- 新增维度；
- 新增全局边界条件；
- 替换 AMR 机制；
- 无 WarpX 源码运行。

`IAmrPolicy` 只覆盖 tagging/DM/base grid 决策。

## 5. 输入输出兼容性

### 输入

默认路径仍由 `WarpX::Create()` 读取原 `ParmParse` 输入，旧参数不变。

新增：

```text
WARPX_PLUGIN_PATH   （可选）
warpx.refine_start_time
warpx.refine_stop_time
warpx.fine_tag_lo_2 / hi_2
warpx.refine_region_change_time
```

均为可选。

### 输出

- plotfile/openPMD 字段名保持；
- checkpoint 增加 `field_schema.txt`（附加文件，不替代旧文件）；
- checkpoint 文件名保留 legacy 映射；
- 插件只增加 handler，不改变默认输出。

### 未验证

- PICMI/pywarpx 在 Runtime 下是否保持；
- 1D/3D/RZ 输出；
- GPU 输出。

## 6. 审计中直接修复的问题

1. **PML 重复创建**
   - 现象：laser_ion_acc step100 load balance 后
     `pml_E_fp[level=0] already exists`；
   - 修复：`erase_internal` + `CreatePMLForLevel` 先清旧 PML。
2. **硬编码物种名**
   - `PicPushHandler`/`PicEmPackage` spec 里写死
     `electrons/ions/photons`；
   - 修复：改为空依赖。
3. **多维度构建**
   - `test_plugin_loader_missing_file_2d` 未加 `TARGET lib_2d` 守卫；
   - 修复：加 `if(TARGET lib_2d)`。
4. **3D 编译**
   - `FullDiagnostics` 的 `get_alldirs` const 问题；
   - `MacroscopicProperties` 缺 EB 类型声明；
   - 已做部分修复，1D/3D/RZ 仍未完整构建。

## 7. 不能声称“全部完成”的部分

- 4D 构建/运行；
- GPU；
- PICMI；
- 全功能插件化（solver/pusher/边界）；
- 删除 WarpX 后独立运行；
- 1D/3D/RZ 全量测试。

## 8. 建议

优先级：

1. 修完 1D/3D/RZ 构建并跑各自官方算例；
2. 构建 + 运行 PICMI/pywarpx；
3. 把 `PicFieldSolveHandler` 拆成 Yee/PSATD/静电等独立模块；
4. 把 `PicPushHandler` 拆成 Pusher/Deposition/Finalize；
5. 再讨论“无 WarpX 内核”的独立 Runtime。
