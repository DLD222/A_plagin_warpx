/* Copyright 2020 Remi Lehe
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "FiniteDifferenceSolver.H"

#include "Runtime/RuntimeConfig.H"
#include "Runtime/ILegacyRuntime.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"

#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER)
#   include "Numerics/FiniteDifferenceAlgorithms/CylindricalYeeAlgorithm.H"
#elif defined(WARPX_DIM_RSPHERE)
#   include "Numerics/FiniteDifferenceAlgorithms/SphericalYeeAlgorithm.H"
#else
#   include "Numerics/FiniteDifferenceAlgorithms/CartesianYeeAlgorithm.H"
#   include "Numerics/FiniteDifferenceAlgorithms/CartesianCKCAlgorithm.H"
#   include "Numerics/FiniteDifferenceAlgorithms/CartesianNodalAlgorithm.H"
#endif
#include "Utils/TextMsg.H"
#include "Utils/WarpXAlgorithmSelection.H"
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
#   include "WarpX.H"
#endif

#include <AMReX.H>
#include <AMReX_GpuDevice.H>
#include <AMReX_PODVector.H>
#include <AMReX_Vector.H>

#include <vector>

/* This function initializes the stencil coefficients for the chosen finite-difference algorithm */
FiniteDifferenceSolver::FiniteDifferenceSolver (
    ElectromagneticSolverAlgo const fdtd_algo,
    std::array<amrex::Real,3> cell_size,
    ablastr::utils::enums::GridType grid_type,
    const warpx::runtime::RuntimeContext* ctx):
    // Register the type of finite-difference algorithm
    m_fdtd_algo{fdtd_algo},
    m_grid_type{grid_type},
    m_runtime_context(ctx)
{
    // return if not FDTD
    if (fdtd_algo == ElectromagneticSolverAlgo::None || fdtd_algo == ElectromagneticSolverAlgo::PSATD) {
        return;
    }

    // Calculate coefficients of finite-difference stencil
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER)
    m_dr = cell_size[0];
    m_nmodes = config().n_rz_azimuthal_modes();
    m_rmin = data().geom(0).ProbLo(0);
    if (fdtd_algo == ElectromagneticSolverAlgo::Yee ||
        fdtd_algo == ElectromagneticSolverAlgo::HybridPIC ) {
        CylindricalYeeAlgorithm::InitializeStencilCoefficients( cell_size,
            m_h_stencil_coefs_r, m_h_stencil_coefs_z );
        m_stencil_coefs_r.resize(m_h_stencil_coefs_r.size());
        m_stencil_coefs_z.resize(m_h_stencil_coefs_z.size());
        amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice,
                              m_h_stencil_coefs_r.begin(), m_h_stencil_coefs_r.end(),
                              m_stencil_coefs_r.begin());
        amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice,
                              m_h_stencil_coefs_z.begin(), m_h_stencil_coefs_z.end(),
                              m_stencil_coefs_z.begin());
        amrex::Gpu::synchronize();
    } else {
        WARPX_ABORT_WITH_MESSAGE(
            "FiniteDifferenceSolver: Unknown algorithm");
    }
#elif defined(WARPX_DIM_RSPHERE)
    m_dr = cell_size[0];
    m_rmin = data().geom(0).ProbLo(0);
    if (fdtd_algo == ElectromagneticSolverAlgo::Yee ||
        fdtd_algo == ElectromagneticSolverAlgo::HybridPIC ) {
        SphericalYeeAlgorithm::InitializeStencilCoefficients(cell_size, m_h_stencil_coefs_r);
        m_stencil_coefs_r.resize(m_h_stencil_coefs_r.size());
        amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice,
                              m_h_stencil_coefs_r.begin(), m_h_stencil_coefs_r.end(),
                              m_stencil_coefs_r.begin());
        amrex::Gpu::synchronize();
    } else {
        WARPX_ABORT_WITH_MESSAGE(
            "FiniteDifferenceSolver: Unknown algorithm");
    }
#else
    if (grid_type == ablastr::utils::enums::GridType::Collocated) {

        CartesianNodalAlgorithm::InitializeStencilCoefficients( cell_size,
            m_h_stencil_coefs_x, m_h_stencil_coefs_y, m_h_stencil_coefs_z );

    } else if (fdtd_algo == ElectromagneticSolverAlgo::Yee ||
               fdtd_algo == ElectromagneticSolverAlgo::ECT ||
               fdtd_algo == ElectromagneticSolverAlgo::HybridPIC) {

        CartesianYeeAlgorithm::InitializeStencilCoefficients( cell_size,
            m_h_stencil_coefs_x, m_h_stencil_coefs_y, m_h_stencil_coefs_z );

    } else if (fdtd_algo == ElectromagneticSolverAlgo::CKC) {

        CartesianCKCAlgorithm::InitializeStencilCoefficients( cell_size,
            m_h_stencil_coefs_x, m_h_stencil_coefs_y, m_h_stencil_coefs_z );

    } else {
        WARPX_ABORT_WITH_MESSAGE(
            "FiniteDifferenceSolver: Unknown algorithm");
    }

    m_stencil_coefs_x.resize(m_h_stencil_coefs_x.size());
    m_stencil_coefs_y.resize(m_h_stencil_coefs_y.size());
    m_stencil_coefs_z.resize(m_h_stencil_coefs_z.size());

    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice,
                          m_h_stencil_coefs_x.begin(), m_h_stencil_coefs_x.end(),
                          m_stencil_coefs_x.begin());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice,
                          m_h_stencil_coefs_y.begin(), m_h_stencil_coefs_y.end(),
                          m_stencil_coefs_y.begin());
    amrex::Gpu::copyAsync(amrex::Gpu::hostToDevice,
                          m_h_stencil_coefs_z.begin(), m_h_stencil_coefs_z.end(),
                          m_stencil_coefs_z.begin());
    amrex::Gpu::synchronize();
#endif
}

const warpx::runtime::RuntimeConfig&
FiniteDifferenceSolver::config () const
{
    if (m_runtime_context != nullptr && m_runtime_context->config != nullptr) {
        return *m_runtime_context->config;
    }
    static const warpx::runtime::RuntimeConfig legacy{};
    return legacy;
}

const warpx::runtime::RuntimeData&
FiniteDifferenceSolver::data () const
{
    if (m_runtime_context != nullptr && m_runtime_context->data != nullptr) {
        return *m_runtime_context->data;
    }
    // Kernel provider comes from the core registry, so this solver does not
    // need the WarpX singleton at all (S6).
    return warpx::runtime::legacy_runtime_data();
}
