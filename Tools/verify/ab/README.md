# A/B comparison against the upstream reference

Two scripts, checked in so the tables in `Docs/research_notes/UPSTREAM_PARITY.md` can be
reproduced without the scratch directory they were originally written in:

| script | what it does |
|---|---|
| `run_ab.sh <case> warp:<input> [args...]` | runs the same input through `$UP_BIN` and `$MINE_BIN`, in `<case>/up` and `<case>/mine` |
| `compare_fields.py` | loads the last plotfile of every `<case>/{up,mine}` pair and prints `case plotfile fields scale max|diff| max|diff|/scale` |
| `run_all.sh [prefix...]` | runs the whole case set of `UPSTREAM_PARITY.md` and prints the table; inputs are resolved against the repository root, so it can be run from any directory |

```bash
export UP_BIN=/tmp/warpx_up2610/build_1d/bin/warpx.1d.MPI.OMP.DP.PDP.OPMD.QED
export MINE_BIN=$PWD/build_multi_hdf5_fft/bin/warpx.1d.MPI.OMP.DP.PDP.OPMD.FFT.EB.QED
Tools/verify/ab/run_ab.sh d1_langmuir warp:Examples/Tests/langmuir/inputs_test_1d_langmuir_multi
python3 Tools/verify/ab/compare_fields.py
```

Inputs that write **particles only** have no meshes to compare: use
`Tools/verify/compare_particles_openpmd.py` on the two `diags/diag1` directories instead.

## Getting the upstream reference to build

Two things bit this project and are worth knowing before starting:

1. **MPI**: build the reference against the **same** MPI that `mpiexec` will launch it with.
   A conda-prefixed build links conda's MPICH *and* the system OpenMPI, and aborts at run
   time with `MPII_Comm_set_attr: Invalid communicator`. On this machine the reference had
   to be configured with `-DCMAKE_CXX_COMPILER=/usr/bin/mpicxx` and no conda prefix.
2. **1D**: upstream 26.10 does not compile in 1D - `MacroscopicProperties` names
   `amrex::EBFArrayBoxFactory` without declaring it and includes `AMReX_EBFabFactory.H`
   unconditionally. A forward declaration plus an `#if defined(AMREX_USE_EB)` guard, applied
   to the *reference tree only*, is enough. Our repository already has both.

## The whole set

`run_all.sh` carries the case table (seventeen cases across 1D/2D/3D/RZ) and picks the
binaries per dimensionality:

```bash
export UP_BIN_1D=... MINE_BIN_1D=...      # and _2D, _3D, _RZ
Tools/verify/ab/run_all.sh                # everything
Tools/verify/ab/run_all.sh d1_ rz_       # only those prefixes
```

A case whose binaries are not exported is **skipped with a message**, so a partial setup
still yields a partial table instead of failing.

The particle-only QED cases (`d3_bw_ang`, `d3_bw_rel`, `d3_compton2`) are not in the table:
they write no meshes, and are compared with `compare_particles_openpmd.py` instead.
