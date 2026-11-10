# 验证矩阵（2026-11-11）

本仓库的所有结论都可以用下表的构建复现。每一行给出：构建目录、配置选项、测试总数、
本机最近一次运行的失败清单与归类。**检查器**（四个）在任何时候都必须全 0/OK。

| 构建 | 维度 | 特性 | 测试数 | 失败 | 归类 |
|---|---|---|---|---|---|
| `build_takeover` | 2 | EB、QED（openPMD=OFF） | 264 | 32 | 全部 openPMD 依赖（本构建未开 openPMD） |
| `build_2d_full` | 2 | EB、QED、**FFT**、**openPMD** | 336 | 1 | `test_2d_laser_ion_acc.checksum`（平台校验和） |
| `build_multi_hdf5_fft` | 1、3、RZ | EB、QED、**FFT**、**openPMD**（并行 HDF5） | 573 | 3 | 2 条平台校验和 + 1 条需要 Python 绑定（本构建 PYTHON=OFF） |
| `build_py_opmd` | 2 | EB、QED、**FFT**、**openPMD**、**Python** | 426 | 1 | `test_2d_ionization_picmi.checksum`（平台校验和） |
| `build_py_4d` | 1、3、RZ | EB、QED、**FFT**、**openPMD**、**Python** | 724 | 6 | 4 条平台校验和 + 2 条 pytest（`libamrex_3d` 段错误：调试时只重建了 1D 目标造成的构建不一致） |
| `build_exotic` | RCYLINDER、RSPHERE | EB、QED、openPMD | 6 | 0 | — |
| `build_petsc` | 2、RZ、RCYLINDER、3 | EB、QED、**PETSc** | 680 | 4 | 全部 `*_petsc_*.checksum`（PETSc 版本/平台差异），run 与 analysis 全过 |

## 复现命令

```bash
# 检查器（任何时候都必须全 0/OK）
python3 Tools/Modularization/check_core_purity.py
python3 Tools/Modularization/check_plugin_boundaries.py
python3 Tools/Modularization/check_kernel_coupling.py
python3 Tools/Modularization/check_build_boundaries.py

# 跑某个构建（务必先确认没有别的会话在跑，并导出无头后端）
ps aux | grep -E "ctest|cmake --build"
export MPLBACKEND=Agg OMP_NUM_THREADS=1
ctest --test-dir build_multi_hdf5_fft -j 2 --timeout 2400
```

## 各构建的配置要点

```bash
# 四维（1D/3D/RZ）+ openPMD + FFT，用自建并行 HDF5
export LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:/tmp/hdf5_mpi/lib:$LD_LIBRARY_PATH"
cmake -S . -B build_multi_hdf5_fft -DWarpX_DIMS="1;3;RZ" -DWarpX_OPENPMD=ON \
      -DWarpX_FFT=ON -DWarpX_COMPUTE=OMP -DHDF5_ROOT=/tmp/hdf5_mpi \
      -DCMAKE_PREFIX_PATH="$HOME/miniconda3;$HOME/third_party/fft-deps"

# PETSc：需要 conda 的 petsc 与配套编译器（mpicc/mpicxx）
export PATH="$HOME/miniconda3/bin:$PATH"
cmake -S . -B build_petsc -DWarpX_DIMS="2;RZ;RCYLINDER;3" -DWarpX_PETSC=ON \
      -DAMReX_PETSC=ON -DCMAKE_C_COMPILER=$HOME/miniconda3/bin/mpicc \
      -DCMAKE_CXX_COMPILER=$HOME/miniconda3/bin/mpicxx \
      -DCMAKE_PREFIX_PATH=$HOME/miniconda3

# 特殊维度
cmake -S . -B build_exotic -DWarpX_DIMS="RCYLINDER;RSPHERE" -DWarpX_OPENPMD=ON
```

## 外部依赖（非仓库内容）

| 依赖 | 位置 | 用途 |
|---|---|---|
| 并行 HDF5 1.14.6 | `/tmp/hdf5_mpi`（`Tools/verify/build_parallel_hdf5.sh`） | openPMD 输出、四维套件 |
| FFTW | `$HOME/third_party/fftw` | PSATD / FFT |
| blaspp / lapackpp | `$HOME/third_party/fft-deps` | RZ + FFT |
| openPMD 示例数据集 | `$HOME/openPMD-example-datasets` | `load_external_field` 与 gaussian_beam 类用例 |
| `lasy` 0.7.0 | conda 环境 | `*_lasy_file*` 用例（20 条） |
| PETSc 3.26.1 | conda 环境 | `*_petsc_*` 用例 |

## 官方算例覆盖

按 CTest 命令实际引用的输入文件统计：**405 / 407** 个官方 `inputs_test_*` / `PICMI_inputs_*`
至少被一个构建使用。剩余 2 个是 `gaussian_beam` 的 openPMD 用例，**上游自己的 CMake 无法注册**
（依赖的 `_prepare` 定义在其后，`if(dependency AND NOT TEST ${dependency}.run) return()` 必然为真），
本仓库该文件与上游逐字节一致。
