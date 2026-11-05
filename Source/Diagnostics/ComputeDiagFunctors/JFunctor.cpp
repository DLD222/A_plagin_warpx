/* This file is part of WarpX.
 *
 * Authors: Roelof Groenewald
 * License: BSD-3-Clause-LBNL
 */

#include "JFunctor.H"

#include "Fields.H"
#include "Parallelization/WarpXSumGuardCells.H"
#include "Particles/MultiParticleContainer.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeData.H"
#include "WarpX.H"

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/utils/Communication.H>

#include <AMReX.H>
#include <AMReX_Extension.H>
#include <AMReX_IntVect.H>
#include <AMReX_MultiFab.H>

using warpx::fields::FieldType;

JFunctor::JFunctor (const int dir, int lev,
                   amrex::IntVect crse_ratio,
                   bool convertRZmodes2cartesian,
                   bool deposit_current, int ncomp)
    : ComputeDiagFunctor(ncomp, crse_ratio), m_dir(dir), m_lev(lev),
      m_convertRZmodes2cartesian(convertRZmodes2cartesian),
      m_deposit_current(deposit_current)
{ }

void
JFunctor::operator() (amrex::MultiFab& mf_dst, int dcomp, const int /*i_buffer*/) const
{
    using ablastr::fields::Direction;

    /** pointer to source multifab (can be multi-component) */
    amrex::MultiFab* m_mf_src = const_cast<ablastr::fields::MultiFabRegister&>(data().fields())
        .get(FieldType::current_fp, Direction{m_dir}, m_lev);

    // Deposit current if no solver or the electrostatic solver is being used
    if (m_deposit_current)
    {
        host().depositCurrent(m_lev, data().dt(m_lev));
    }

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_mf_src != nullptr, "m_mf_src can't be a nullptr.");
    AMREX_ASSUME(m_mf_src != nullptr);

    InterpolateMFForDiag(
        mf_dst, *m_mf_src, dcomp,
        data().distributionMap(m_lev), m_convertRZmodes2cartesian);
}
