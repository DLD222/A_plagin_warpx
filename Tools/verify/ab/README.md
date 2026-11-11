# A/B comparison against the upstream reference

Two scripts, checked in so the tables in `Docs/research_notes/UPSTREAM_PARITY.md` can be
reproduced without the scratch directory they were originally written in:

| script | what it does |
|---|---|
| `run_ab.sh <case> warp:<input> [args...]` | runs the same input through `$UP_BIN` and `$MINE_BIN`, in `<case>/up` and `<case>/mine` |
| `compare_fields.py` | loads the last plotfile of every `<case>/{up,mine}` pair and prints `case plotfile fields scale max|diff| max|diff|/scale` |

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
