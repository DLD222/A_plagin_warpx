/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "PETScImplicitBridge.H"

#include "ImplicitSolver.H"

namespace warpx::petsc
{

void compute_rhs (ImplicitSolver* a_op,
                  WarpXSolverVec& a_rhs,
                  const WarpXSolverVec& a_E,
                  amrex::Real a_time,
                  int a_nl_iter,
                  bool a_from_jacobian,
                  bool a_update_pc)
{
    a_op->ComputeRHS(a_rhs, a_E, a_time, a_nl_iter, a_from_jacobian, a_update_pc);
}

void pre_linear_solve (ImplicitSolver* a_op, bool a_update_pc)
{
    a_op->PreLinearSolve(a_update_pc);
}

amrex::Real get_theta_for_pc (const ImplicitSolver* a_op)
{
    return a_op->GetThetaForPC();
}

void assemble_pc_matrix (ImplicitSolver* a_op)
{
    a_op->AssemblePCMatrix();
}

const amrex::Geometry& geometry (const ImplicitSolver* a_op, int a_level)
{
    return a_op->GetGeometry(a_level);
}

amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM> lin_op_bc_lo (const ImplicitSolver* a_op)
{
    return a_op->GetLinOpBCLo();
}

amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM> lin_op_bc_hi (const ImplicitSolver* a_op)
{
    return a_op->GetLinOpBCHi();
}

std::unique_ptr<MatrixSolverOps<WarpXSolverVec>> make_pc_matrix_ops (ImplicitSolver* a_op)
{
    return ImplicitSolver::MakePCMatrixOps(a_op);
}

int num_amr_levels (const ImplicitSolver* a_op)
{
    return a_op->numAMRLevels();
}

const MassMatrixCoeff* mass_matrices_coeff (const ImplicitSolver* a_op)
{
    return a_op->GetMassMatricesCoeff();
}

void get_pc_matrix (ImplicitSolver* a_op,
                    amrex::Gpu::DeviceVector<int>& a_r_indices_g,
                    amrex::Gpu::DeviceVector<int>& a_num_nz,
                    amrex::Gpu::DeviceVector<int>& a_c_indices_g,
                    amrex::Gpu::DeviceVector<amrex::Real>& a_a_ij,
                    int& a_n, int& a_ncols_max)
{
    a_op->GetPCMatrix(a_r_indices_g, a_num_nz, a_c_indices_g, a_a_ij, a_n, a_ncols_max);
}

} // namespace warpx::petsc
