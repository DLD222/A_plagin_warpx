/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Compile check for the generic particle contracts that stay in Core.  The
 * alias-only contracts (IParticleTile / IParticleIterator / IParticlePosition
 * Access) were removed under D1-a: they aliased concrete WarpX types and
 * belonged to the particle plugin, not to the core.
 */
#include "Modules/Particles/IParticleReduce.H"
#include "Modules/Particles/IParticleSpeciesAccess.H"

int main () { return 0; }
