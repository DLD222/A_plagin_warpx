/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 *
 * Particle bookkeeping the implicit Darwin solver used to do itself.  Those
 * loops walk the species and touch the particle attributes directly, which is
 * kernel business: the particle tile layout must not leak into a solver.
 */
#include "WarpX.H"

#include "Particles/MultiParticleContainer.H"
#include "Utils/TextMsg.H"

#include <AMReX_GpuAtomic.H>
#include <AMReX_GpuContainers.H>

using namespace amrex;

void
WarpX::ClearImplicitParticleVelocities ()
{
    BL_PROFILE("WarpX::ClearImplicitParticleVelocities()");
    // This function sets the particle velocities to zero since the "corrector"
    // velocity push only calculate the velocity due to acceleration from
    // the inductive E-field. The actual velocities are still stored in u_n.

    for (auto const& pc : GetPartContainer()) {

        // for (int lev = 0; lev <= finest_level; ++lev)
        const int lev = 0;
        {
#ifdef AMREX_USE_OMP
#pragma omp parallel
#endif
            auto particle_comps = pc->GetRealSoANames();

            for (ParticleTileIter pti(*pc, lev); pti.isValid(); ++pti) {

                auto& attribs = pti.GetAttribs();
                amrex::ParticleReal* const AMREX_RESTRICT ux = attribs[PIdx::ux].dataPtr();
                amrex::ParticleReal* const AMREX_RESTRICT uy = attribs[PIdx::uy].dataPtr();
                amrex::ParticleReal* const AMREX_RESTRICT uz = attribs[PIdx::uz].dataPtr();

                const long np = pti.numParticles();

                amrex::ParallelFor( np, [=] AMREX_GPU_DEVICE (long ip)
                {
                    ux[ip] = 0.0;
                    uy[ip] = 0.0;
                    uz[ip] = 0.0;
                });
            }
        }
    }
}

void
WarpX::FinishImplicitVelocityUpdate ()
{
    BL_PROFILE("WarpX::FinishImplicitVelocityUpdate()");
    // This function sets the particle velocities to include the acceleration
    // from both the electrostatic field (currently held in u_n) and the
    // inductive field (currently held in u)

    for (auto const& pc : GetPartContainer()) {

        // for (int lev = 0; lev <= finest_level; ++lev)
        const int lev = 0;
        {
#ifdef AMREX_USE_OMP
#pragma omp parallel
#endif
            auto particle_comps = pc->GetRealSoANames();

            for (ParticleTileIter pti(*pc, lev); pti.isValid(); ++pti) {

                auto& attribs = pti.GetAttribs();
                amrex::ParticleReal* const AMREX_RESTRICT ux = attribs[PIdx::ux].dataPtr();
                amrex::ParticleReal* const AMREX_RESTRICT uy = attribs[PIdx::uy].dataPtr();
                amrex::ParticleReal* const AMREX_RESTRICT uz = attribs[PIdx::uz].dataPtr();

                amrex::ParticleReal* ux_n = pti.GetAttribs("ux_n").dataPtr();
                amrex::ParticleReal* uy_n = pti.GetAttribs("uy_n").dataPtr();
                amrex::ParticleReal* uz_n = pti.GetAttribs("uz_n").dataPtr();

                const long np = pti.numParticles();

                amrex::ParallelFor( np, [=] AMREX_GPU_DEVICE (long ip)
                {
                    ux[ip] += ux_n[ip];
                    uy[ip] += uy_n[ip];
                    uz[ip] += uz_n[ip];
                });
            }
        }
    }
}

void
WarpX::PrepareImplicitVelocitiesForDeposition ()
{
    BL_PROFILE("WarpX::PrepareImplicitVelocitiesForDeposition()");
    // On entry, u holds the velocity after the electrostatic-only push
    // (PushP in OneStep()) and u_n holds the velocity saved at the start of
    // the step (SaveParticlesAtImplicitStepStart()). This function sets u to
    // the time-centered average of the two, which is what
    // GetImplicitGammaInverse() and setMassMatricesKernels() (shared with
    // the electromagnetic implicit schemes) expect as the deposition-time
    // velocity to compute a correct relativistic gamma factor from.
    // u_n is left holding the electrostatic-only velocity (the u value at the
    // start of this function) rather than the step-start value, since
    // FinishVelocityUpdate() later reads u_n to recombine the electrostatic
    // and inductive velocity contributions; GetImplicitGammaInverse()'s
    // reconstruction is symmetric under swapping which of the two sampled
    // velocities is treated as "u_n" vs "u_nph", so this substitution does
    // not affect the deposition-time physics.

    for (auto const& pc : GetPartContainer()) {

        // for (int lev = 0; lev <= finest_level; ++lev)
        const int lev = 0;
        {
#ifdef AMREX_USE_OMP
#pragma omp parallel
#endif
            auto particle_comps = pc->GetRealSoANames();

            for (ParticleTileIter pti(*pc, lev); pti.isValid(); ++pti) {

                auto& attribs = pti.GetAttribs();
                amrex::ParticleReal* const AMREX_RESTRICT ux = attribs[PIdx::ux].dataPtr();
                amrex::ParticleReal* const AMREX_RESTRICT uy = attribs[PIdx::uy].dataPtr();
                amrex::ParticleReal* const AMREX_RESTRICT uz = attribs[PIdx::uz].dataPtr();

                amrex::ParticleReal* ux_n = pti.GetAttribs("ux_n").dataPtr();
                amrex::ParticleReal* uy_n = pti.GetAttribs("uy_n").dataPtr();
                amrex::ParticleReal* uz_n = pti.GetAttribs("uz_n").dataPtr();

                const long np = pti.numParticles();

                amrex::ParallelFor( np, [=] AMREX_GPU_DEVICE (long ip)
                {
                    const amrex::ParticleReal ux_es = ux[ip];
                    ux[ip] = 0.5_prt*(ux_es + ux_n[ip]);
                    ux_n[ip] = ux_es;

                    const amrex::ParticleReal uy_es = uy[ip];
                    uy[ip] = 0.5_prt*(uy_es + uy_n[ip]);
                    uy_n[ip] = uy_es;

                    const amrex::ParticleReal uz_es = uz[ip];
                    uz[ip] = 0.5_prt*(uz_es + uz_n[ip]);
                    uz_n[ip] = uz_es;
                });
            }
        }
    }
}

void WarpX::AddImplicitParticleAttributes (bool particle_suborbits)
{
    // Set comm to false so that the attributes are not communicated nor written
    // to the checkpoint files.
    int const comm = 0;

    for (auto const& pc : GetPartContainer()) {
#if !defined(WARPX_DIM_1D_Z)
        pc->AddRealComp("x_n", comm);
#endif
#if defined(WARPX_DIM_3D) || defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
        pc->AddRealComp("y_n", comm);
#endif
#if !defined(WARPX_DIM_RCYLINDER)
        pc->AddRealComp("z_n", comm);
#endif
        pc->AddRealComp("ux_n", comm);
        pc->AddRealComp("uy_n", comm);
        pc->AddRealComp("uz_n", comm);

        if (particle_suborbits) {
            pc->AddIntComp("nsuborbits", comm);
        }
    }
}
