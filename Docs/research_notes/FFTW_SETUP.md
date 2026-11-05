# PSATD 的本地 FFTW 构建与验收

> 目标：在没有系统 `libfftw3-dev` 的机器上，用仓库外的本地 FFTW 构建并运行 PSATD 算例。

## 1. 本地 FFTW

AMReX 同时查找 `fftw3`（double）和 `fftw3f`（single），因此需要两套库，装到同一前缀：

```bash
mkdir -p "$HOME/third_party" && cd "$HOME/third_party"
curl -L --fail -o fftw.tar.gz https://www.fftw.org/fftw-3.3.10.tar.gz
tar xzf fftw.tar.gz

# double
cd fftw-3.3.10
./configure --prefix="$HOME/third_party/fftw" --enable-shared --enable-openmp --disable-fortran
make -j 24 && make install

# single（复制一份源码，避免覆盖 double 的 config）
cd ..
cp -a fftw-3.3.10 fftw-3.3.10-single
cd fftw-3.3.10-single && make distclean
./configure --prefix="$HOME/third_party/fftw" --enable-single --enable-shared --enable-openmp --disable-fortran
make -j 24 && make install
```

结果：

```
$HOME/third_party/fftw/lib/pkgconfig/fftw3.pc
$HOME/third_party/fftw/lib/pkgconfig/fftw3f.pc
```

## 2. 配置 FFT 构建

```bash
PKG_CONFIG_PATH="$HOME/third_party/fftw/lib/pkgconfig" \
cmake -S . -B build_fft \
  -DWarpX_DIMS=2 -DWarpX_EB=ON -DWarpX_MPI=ON -DWarpX_COMPUTE=OMP \
  -DWarpX_PYTHON=OFF -DWarpX_OPENPMD=OFF -DWarpX_FFT=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_SOURCE_DIR_FETCHEDAMREX="$HOME/WarpX_fluid/build_2d/_deps/fetchedamrex-src"

cmake --build build_fft -j 24
```

运行时需要本地库路径：

```bash
export LD_LIBRARY_PATH="$HOME/third_party/fftw/lib:$LD_LIBRARY_PATH"
```

## 3. PSATD 算例

输入：`tests/runtime/inputs_2d_psatd`

```
algo.maxwell_solver = PSATD
geometry.dims = 2
periodic boundaries
Ex = 1 V/m 初始均匀场
```

测试：

```
runtime_psatd_2        # 跑完整算例，退出码 0
runtime_psatd_check_2  # 解析 field_energy，断言 step=1 且能量守恒
```

预期：

```
E_lev0 = 1.77083756376e-11 J
total  = 1.77083756376e-11 J
```

## 4. 结果

在 `build_fft` 上：

```
5/5 Test PASS
  runtime_snapshot_2
  runtime_snapshot_particles_2
  runtime_psatd_2
  runtime_psatd_check_2
  runtime_lb_2
```

`build`（FFT OFF）仍只跑 3 个非 PSATD 测试，保持原回归集不变。
