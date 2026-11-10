/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_PETSCIMPLICITBRIDGE_H_
#define WARPX_PETSCIMPLICITBRIDGE_H_

#include "NonlinearSolvers/MatrixSolverOps.H"
#include "Solvers/WarpXSolverVec.H"

#include <memory>

#include <AMReX_GpuContainers.H>
#include <AMReX_Array.H>
#include <AMReX_Geometry.H>
#include <AMReX_LO_BCTYPES.H>
#include <AMReX_MultiFab.H>
#include <AMReX_Vector.H>
#include <AMReX_REAL.H>

class ImplicitSolver;

/** Thin bridge between the PETSc wrappers and the implicit solver.
 *
 * The wrappers work on a forward-declared ImplicitSolver, but two of the calls they make
 * need the complete type.  They cannot include ImplicitSolver.H themselves: that header
 * (transitively) pulls Utils/WarpXAlgorithmSelection.H, which declares ReductionType at
 * global scope and collides with petscvec.h's typedef of the same name in either include
 * order.  These two free functions live in a translation unit that sees the complete type
 * and no PETSc header, so the wrappers keep working with the forward declaration only.
 */
namespace warpx::petsc
{

void compute_rhs (ImplicitSolver* a_op,
                  WarpXSolverVec& a_rhs,
                  const WarpXSolverVec& a_E,
                  amrex::Real a_time,
                  int a_nl_iter,
                  bool a_from_jacobian,
                  bool a_update_pc);

void pre_linear_solve (ImplicitSolver* a_op, bool a_update_pc);

//! Implicit treatment of light waves used by the preconditioner.
amrex::Real get_theta_for_pc (const ImplicitSolver* a_op);

//! Assemble the sparse matrix the preconditioner consumes.
void assemble_pc_matrix (ImplicitSolver* a_op);

//! Copy the assembled sparse matrix into caller-provided device vectors.
//! Geometry of one AMR level, as the preconditioner sees it.
const amrex::Geometry& geometry (const ImplicitSolver* a_op, int a_level);

//! Linear-operator boundary conditions at the low end.
amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM> lin_op_bc_lo (const ImplicitSolver* a_op);

//! Linear-operator boundary conditions at the high end.
amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM> lin_op_bc_hi (const ImplicitSolver* a_op);

//! Matrix operator the preconditioner iterates with.
std::unique_ptr<MatrixSolverOps<WarpXSolverVec>> make_pc_matrix_ops (ImplicitSolver* a_op);

//! Number of AMR levels the solver works on.
int num_amr_levels (const ImplicitSolver* a_op);

//! Deposited mass matrices per level, or nullptr when they are not used.
using MassMatrixCoeff = amrex::Vector<amrex::Array<amrex::MultiFab*, 3>>;
const MassMatrixCoeff* mass_matrices_coeff (const ImplicitSolver* a_op);

void get_pc_matrix (ImplicitSolver* a_op,
                    amrex::Gpu::DeviceVector<int>& a_r_indices_g,
                    amrex::Gpu::DeviceVector<int>& a_num_nz,
                    amrex::Gpu::DeviceVector<int>& a_c_indices_g,
                    amrex::Gpu::DeviceVector<amrex::Real>& a_a_ij,
                    int& a_n, int& a_ncols_max);

} // namespace warpx::petsc

#endif // WARPX_PETSCIMPLICITBRIDGE_H_
