#include "RhoFunctor.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include <stdexcept>
#include <string>

#include "Diagnostics/ComputeDiagFunctors/ComputeDiagFunctor.H"
#include "Fluids/MultiFluidContainer.H"
#include "Fluids/WarpXFluidContainer.H"
#include "Modules/Particles/ParticleTileIter.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <AMReX.H>
#include <AMReX_IntVect.H>
#include <AMReX_MultiFab.H>

#include <memory>

RhoFunctor::RhoFunctor (const int lev,
                        const amrex::IntVect crse_ratio,
                        bool apply_rz_psatd_filter,
                        const int species_index,
                        bool convertRZmodes2cartesian,
                        const int ncomp)
    : ComputeDiagFunctor(ncomp, crse_ratio),
      m_lev(lev),
      m_apply_rz_psatd_filter(apply_rz_psatd_filter),
      m_species_index(species_index),
      m_convertRZmodes2cartesian(convertRZmodes2cartesian)
{}

void
RhoFunctor::operator() ( amrex::MultiFab& mf_dst, const int dcomp, const int /*i_buffer*/ ) const
{
    std::unique_ptr<amrex::MultiFab> rho =
        host().particleServices().computeRho(m_lev, m_species_index, m_apply_rz_psatd_filter,
            nComp(), amrex::IntVect::TheNodeVector(), data().ngDeposRho(),
            m_convertRZmodes2cartesian);

    // A7.1: the host decides the rho layout (nodal/cell, RZ components).  Do not
    // infer it from RuntimeConfig here; reject a mismatched source explicitly
    // instead of letting MultiFab::Add/Interpolate read out of bounds.
    if (!m_convertRZmodes2cartesian && rho->nComp() != nComp()) {
        throw std::runtime_error(
            "RhoFunctor: rho ncomp " + std::to_string(rho->nComp()) +
            " does not match the diagnostic ncomp " + std::to_string(nComp()));
    }

    InterpolateMFForDiag(mf_dst, *rho, dcomp, data().distributionMap(m_lev),
                         m_convertRZmodes2cartesian);
}
