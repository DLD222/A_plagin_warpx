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

#include "Utils/TextMsg.H"

#include <AMReX_GpuAtomic.H>
#include <AMReX_GpuContainers.H>

using namespace amrex;

void
WarpX::ClearImplicitParticleVelocities ()
{
    particleSystem().ClearImplicitParticleVelocities();
}

void
WarpX::FinishImplicitVelocityUpdate ()
{
    particleSystem().FinishImplicitVelocityUpdate();
}

void
WarpX::PrepareImplicitVelocitiesForDeposition ()
{
    particleSystem().PrepareImplicitVelocitiesForDeposition();
}

void WarpX::AddImplicitParticleAttributes (bool particle_suborbits)
{
    particleSystem().AddImplicitParticleAttributes(particle_suborbits);
}
