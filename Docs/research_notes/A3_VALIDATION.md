# A3 定向回归记录

> 日期：2026-11-06  
> 目标：验证 A3.1–A3.3 的 InitData/OneStep 阶段化与显式 PIC handler 化。

## 1. 默认构建

```bash
ctest --test-dir build -R \
 '^(runtime_|test_2d_langmuir_multi\.(run|analysis)|test_2d_embedded_boundary_cube\.(run|analysis)|test_2d_pml_x_yee\.(run|analysis)|test_2d_collision_xz\.(run|analysis)$)' \
 --output-on-failure
```

```
13/13 PASS
```

覆盖：

- runtime 5 项（snapshot/particles/BTD/ES/LB）
- Langmuir
- EB cube
- PML Yee
- collision

## 2. FFT 构建

```bash
LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH" \
ctest --test-dir build_fft -R \
 '^(runtime_|test_2d_langmuir_multi_psatd\.(run|analysis)|test_2d_pml_x_psatd(_restart)?\.(run|analysis)$)' \
 --output-on-failure
```

```
13/13 PASS
```

覆盖：

- PSATD runtime
- PSATD Langmuir
- PSATD PML
- PSATD PML restart

## 3. ASAN

```bash
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build_asan \
  -R '^(runtime_|test_2d_embedded_boundary_cube\.(run|analysis)$)' \
  --output-on-failure
```

```
7/7 PASS
```

无 use-after-free / 越界报告。

## 4. 阶段与 handler

已验证：

```
InitData: BeforeInit -> Init -> AfterInit -> FieldSolve -> Diagnostics
OneStep:  Push -> AfterPush -> FieldSolve
```

已注册 handler：

```
PicPushHandler
PicFinalizeHandler
PicFieldSolveHandler
```

## 5. 未验证

- 原始 `P0/E3/S5/E6` 基准脚本不在当前 2D ctest 配置中；
- A/B 逐位门仍推迟到 A5；
- CollisionPackage/BinaryCollision 的 handler 尚未执行化。

## 6. 结论

A3 的显式 PIC 路径、阶段顺序、PML/PSATD/EB/碰撞回归均通过；
完整 `A3.4` 仍需原始基准脚本与 A/B 门，不能用本定向门替代。
