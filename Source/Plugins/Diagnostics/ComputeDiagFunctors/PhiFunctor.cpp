/* Copyright 2025 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * Authors: Roelof Groenewald (TAE Technologies)
 *
 * License: BSD-3-Clause-LBNL
 */
#include "PhiFunctor.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"

#include <ablastr/constant.H>

#include "Diagnostics/ComputeDiagFunctors/ComputeDiagFunctor.H"


using warpx::fields::FieldType;

PhiFunctor::PhiFunctor (const int lev,
    const amrex::IntVect crse_ratio,
    bool convertRZmodes2cartesian, const int ncomp)
    : ComputeDiagFunctor(ncomp, crse_ratio),
    m_lev(lev), m_convertRZmodes2cartesian(convertRZmodes2cartesian)
{ }

void
PhiFunctor::operator() (amrex::MultiFab& mf_dst, int dcomp, const int /*i_buffer*/) const
{
    auto phi = host().fieldAlgebra().computePhi(m_lev);
    InterpolateMFForDiag(
        mf_dst, *phi, dcomp, data().distributionMap(m_lev),
        m_convertRZmodes2cartesian
    );
}
