/* Copyright 2022 S. Eric Clark, LLNL
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_MAGNETOSTATICSOLVER_H_
#define WARPX_MAGNETOSTATICSOLVER_H_

#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Array.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_MLMG.H>
#include <AMReX_REAL.H>

class WarpX;

namespace MagnetostaticSolver {

    /** Boundary Handler for the Vector Potential Poisson Solver
     * This only will handle homogeneous Dirichlet boundary conditions on
     * embedded boundaries, and homogeneous dirichlet/Neumann or periodic boundary conditions
     */
    class VectorPoissonBoundaryHandler {
    public:
        amrex::Array<amrex::Array<amrex::LinOpBCType, AMREX_SPACEDIM>, 3> lobc, hibc;
        bool bcs_set = false;
        std::array<std::array<bool, AMREX_SPACEDIM * 2>, 3> dirichlet_flag;
        bool has_non_periodic = false;

        void defineVectorPotentialBCs (
            const amrex::Geometry& geom,
            amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const& field_boundary_lo,
            amrex::Array<FieldBoundaryType,AMREX_SPACEDIM> const& field_boundary_hi);
    };

    /** use amrex to directly calculate the magnetic field since with EB's the
     *
     * simple finite difference scheme in WarpX::computeE sometimes fails
     */
    class EBCalcBfromVectorPotentialPerLevel {
      private:
        ablastr::fields::MultiLevelVectorField m_b_field;
        ablastr::fields::MultiLevelVectorField m_grad_buf_e_stag;
        ablastr::fields::MultiLevelVectorField m_grad_buf_b_stag;
        //! Field-centering orders and device coefficients (were read from WarpX*).
        int m_fg_nox = 0;
        int m_fg_noy = 0;
        int m_fg_noz = 0;
        amrex::Real const* m_coeffs_x = nullptr;
        amrex::Real const* m_coeffs_y = nullptr;
        amrex::Real const* m_coeffs_z = nullptr;
        bool m_single_precision_comms = false;

      public:
        EBCalcBfromVectorPotentialPerLevel (ablastr::fields::MultiLevelVectorField const & b_field,
                                            ablastr::fields::MultiLevelVectorField const & grad_buf_e_stag,
                                            ablastr::fields::MultiLevelVectorField const & grad_buf_b_stag,
                                            int fg_nox, int fg_noy, int fg_noz,
                                            amrex::Real const* coeffs_x,
                                            amrex::Real const* coeffs_y,
                                            amrex::Real const* coeffs_z,
                                            bool single_precision_comms)
                : m_b_field(b_field),
                  m_grad_buf_e_stag(grad_buf_e_stag),
                  m_grad_buf_b_stag(grad_buf_b_stag),
                  m_fg_nox(fg_nox),
                  m_fg_noy(fg_noy),
                  m_fg_noz(fg_noz),
                  m_coeffs_x(coeffs_x),
                  m_coeffs_y(coeffs_y),
                  m_coeffs_z(coeffs_z),
                  m_single_precision_comms(single_precision_comms)
        {}

        void operator() (amrex::Array<std::unique_ptr<amrex::MLMG>, 3> & mlmg, int lev);

        // Function to perform interpolation from cell edges to cell faces
        void doInterp (amrex::MultiFab & src, amrex::MultiFab & dst);
    };
} // namespace MagnetostaticSolver

#endif //WARPX_MAGNETOSTATICSOLVER_H_
