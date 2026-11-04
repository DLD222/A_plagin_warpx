# A0 API catalog — Parallelization

<!-- SPDX-License-Identifier: BSD-3-Clause -->

> Generated from headers; 1 classes/structs. Signatures are parsed heuristically; check the header for exact defaults/templates.

## `guardCellManager`

- file: `Source/Parallelization/GuardCellManager.H`
- kind: `class`
- public methods: 15; public members: 0; private members: 0

```cpp
void Init( amrex::Real dt, const amrex::Real * dx, bool do_subcycling, bool do_fdtd_nci_corr, ablastr::utils::enums::GridType grid_type, bool do_moving_window, int moving_window_dir, int particle_max_grid_crossings, int nox, int nox_fft, int noy_fft, int noz_fft, int nci_corr_stencil, ElectromagneticSolverAlgo electromagnetic_solver_id, CurrentDepositionAlgo current_deposition_algo, EvolveScheme evolve_scheme, int max_level, const amrex::Vector<amrex::Real>& v_galilean, const amrex::Vector<amrex::Real>& v_comoving, bool safe_guard_cells, int do_psatd_JRhom, bool fft_do_time_averaging, bool do_pml, int do_pml_in_domain, int pml_ncell, const amrex::Vector<amrex::IntVect>& ref_ratios, bool use_filter, const amrex::IntVect& bilinear_filter_stencil_length)
amrex::IntVect ng_alloc_EB = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_alloc_J = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_alloc_Rho = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_alloc_F = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_alloc_G = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_FieldSolver = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_FieldSolverF = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_FieldSolverG = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_FieldGather = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_UpdateAux = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_MovingWindow = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_afterPushPSATD = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_depos_J = amrex::IntVect::TheZeroVector()
amrex::IntVect ng_depos_rho = amrex::IntVect::TheZeroVector()
```

