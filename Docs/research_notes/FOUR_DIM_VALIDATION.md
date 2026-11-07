# 四维构建与验证（2026-11-07）

## 构建矩阵

```text
build             2D CPU OMP, EB/QED, FFT OFF, openPMD OFF
build_fft         2D CPU OMP, FFT ON
build_hdf5        2D CPU OMP, openPMD + parallel HDF5
build_multi2      1D/3D/RZ, EB/QED, FFT OFF, openPMD OFF
build_multi_hdf5  1D/3D/RZ, EB/QED, openPMD + parallel HDF5
```

## 1D/3D/RZ 构建

```text
build_multi2      build PASS
build_multi_hdf5  build PASS
```

CMake 目标：

```text
warpx.1d...OPMD.EB.QED
warpx.3d...OPMD.EB.QED
warpx.rz...OPMD.EB.QED
```

## Smoke 测试

非 openPMD：

```text
1D collision_z
3D collision_iso
3D particle_boundaries
RZ langmuir_multi
8/8 PASS
```

openPMD + parallel HDF5：

```text
1D langmuir_multi
1D collision_z
1D laser_acceleration
3D langmuir_multi
3D collision_iso
RZ langmuir_multi
8/8 PASS
```

## 2D

```text
默认 build:  76/87
HDF5 构建:   159/159
```

## 仍缺

- 1D/3D/RZ + FFT：需要 `blaspp`；
- 1D/3D/RZ 全量 openPMD 测试；
- 3D EB/QED 全量；
- RZ 轴专项；
- GPU。

## 结论

四维不再只有 2D：

```text
1D  build + smoke
2D  build + 159/159
3D  build + smoke
RZ  build + smoke
```

但“全功能四维”仍未完成，不能把 smoke 当全量。
