# 核验环境（四维数值等价）

这里记录**本机实测可用**的核验环境搭建方式，供人工与定时任务复用。

## 为什么需要这些脚本

- 本项目依赖 openPMD 输出，而 openPMD 需要**并行 HDF5** 才能启用 HDF5 后端。
- 本机 conda 的 HDF5 是串行版；openPMD 检测到"有 MPI 但只有串行 HDF5"时会**静默关闭**
  HDF5 后端（CMake 有明确警告），于是所有 openPMD 用例报
  `openPMD-api built without support for backend 'HDF5'`。
- 用系统 OpenMPI 自建并行 HDF5 后，四维失败集从 112 降到 42，修完诊断缺陷后进一步降到
  ≈25（剩余全部为环境/构建前提）。

## 用法

```bash
Tools/verify/build_parallel_hdf5.sh /tmp/hdf5_mpi          # 一次性
Tools/verify/build_and_run_4d.sh                            # 配置+构建+跑四维
Tools/verify/build_and_run_4d.sh build_multi_fft -DWarpX_FFT=ON   # 需要 PSATD 用例时
```

## 已知前提（不是代码缺陷）

| 用例族 | 前提 |
|---|---|
| `*_boosted_opmd` | 需 `-DWarpX_FFT=ON`（`algo.maxwell_solver = psatd`） |
| `load_external_field*` | 需 `openPMD-example-datasets`（输入里写 `../../../../openPMD-example-datasets/...`） |
| `test_3d_particles_in_pml_mr` | 单独运行约 100 s，`-j 12` 下易超时 |

## 2026-11-09 实测结果

| 构建 | 通过/总数 |
|---|---|
| 2D（OPENPMD=OFF） | 222/255 |
| 1D/3D/RZ（OPENPMD=OFF） | 397/469 |
| 1D/3D/RZ（OPENPMD=ON + 并行 HDF5） | ≈444/469 |
