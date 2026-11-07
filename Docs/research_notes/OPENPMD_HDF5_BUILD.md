# openPMD + parallel HDF5 构建

## 目标

解决默认构建中 11 个 openPMD 测试失败：

```text
charge_exchange_dsmc
collisions_split_momentum_push_*（3）
embedded_boundary_em_particle_absorption_sh_factor_*（3）
embedded_circle
qed_breit_wheeler_opmd
rigid_injection_btd
laser_ion_acc
```

## 1. 构建并行 HDF5

系统 OpenMPI：

```bash
tar -xzf hdf5-1.14.6.tar.gz -C /tmp/hdf5-src
cd /tmp/hdf5-build
/tmp/hdf5-src/hdf5-1.14.6/configure \
  --prefix=$HOME/hdf5-mpi \
  --enable-parallel --enable-shared \
  --disable-fortran --disable-cxx \
  CC=mpicc CXX=mpicxx
make -j 12
make install
```

## 2. 配置 WarpX

复用已缓存的 AMReX/openPMD 源，避免重新下载：

```bash
cmake -S . -B build_hdf5 \
  -DWarpX_DIMS=2 \
  -DWarpX_OPENPMD=ON \
  -DHDF5_ROOT=$HOME/hdf5-mpi \
  -DHDF5_IS_PARALLEL=TRUE \
  -DopenPMD_USE_HDF5=ON \
  -DopenPMD_USE_ADIOS2=OFF \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX=$HOME/WarpX_fluid/build_2d/_deps/fetchedamrex-src \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDOPENPMD=$PWD/build_opmd/_deps/fetchedopenpmd-src \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDNLOHMANN_JSON=$PWD/build_opmd/_deps/fetchednlohmann_json-src \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDTOML11=$PWD/build_opmd/_deps/fetchedtoml11-src \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDPICSAR=$PWD/build_opmd/_deps/fetchedpicsar-src
cmake --build build_hdf5 -j 24
```

运行：

```bash
LD_LIBRARY_PATH=$HOME/hdf5-mpi/lib ctest --test-dir build_hdf5
```

## 3. 结果

```text
159 / 159 PASS
```

默认构建中失败的 11 个测试全部通过。

## 4. 同时修复的 bug

`laser_ion_acc` 在 step 100 触发 load balance 后：

```text
MultiFabRegister::alloc_init failed because
pml_E_fp[dir=x][level=0] already exists
```

原因：`CreatePMLForLevel` 重新创建 PML 前没有清理旧的 PML 字段。

修复：

- `MultiFabRegister::erase_internal(name)`；
- `CreatePMLForLevel` 先重置 `LevelResources::pml`；
- 删除该 level 的 `pml_*` 注册字段；
- 再创建新 PML。

## 5. 结论

- 默认无 openPMD 构建：76/87，失败是配置缺失；
- 并行 HDF5 构建：159/159；
- 没有修改测试阈值；
- 没有绕过 openPMD。
