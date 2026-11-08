/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_RUNTIMEDATA_H_
#define WARPX_RUNTIME_RUNTIMEDATA_H_

#include "Utils/WarpXAlgorithmSelection.H"

#include <ablastr/utils/text/IntervalsParser.H>

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_Array.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_LayoutData.H>
#include <AMReX_Vector.H>

#include <memory>
#include <AMReX_REAL.H>
#include <AMReX_RealBox.H>

namespace amrex { class AmrCore; class EBFArrayBoxFactory; namespace EB2 { class IndexSpace; } }

class NCIGodfreyFilter;
class IHybridPICModel;
class MultiFluidContainer;

namespace warpx::runtime
{
class ISpeciesContainer;
class IFilterBank;
class ISimulationKernel;
class ISpeciesMetadata;
class IParticleContainerAccess;
class MaterialRegistry;
class FieldRegistry;

/**
 * \brief Narrow, read-mostly view of the kernel state used by diagnostics.
 *
 * This is the A1.4 replacement target for global singleton access in the
 * reduced diagnostics. It deliberately exposes data (fields, particles,
 * geometry, clock), not algorithms.
 */
class RuntimeData
{
public:
    virtual ~RuntimeData () = default;

    [[nodiscard]] virtual ablastr::fields::MultiFabRegister& fields () = 0;
    [[nodiscard]] virtual const ablastr::fields::MultiFabRegister& fields () const = 0;

    [[nodiscard]] virtual amrex::AmrCore& amrCore () const = 0;
    [[nodiscard]] virtual ISpeciesMetadata& speciesMetadata () = 0;
    [[nodiscard]] virtual ISimulationKernel& kernel () = 0;
    [[nodiscard]] virtual const ISimulationKernel& kernel () const = 0;
    [[nodiscard]] virtual IParticleContainerAccess& particleAccess () = 0;
    [[nodiscard]] virtual const ISpeciesMetadata& speciesMetadata () const = 0;
    [[nodiscard]] virtual const IParticleContainerAccess& particleAccess () const = 0;
    [[nodiscard]] virtual MultiFluidContainer* fluids () = 0;

    [[nodiscard]] virtual int finestLevel () const = 0;
    [[nodiscard]] virtual int maxLevel () const = 0;

    [[nodiscard]] virtual const amrex::Geometry& geom (int lev) const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::Geometry>& geomAll () const = 0;
    [[nodiscard]] virtual const amrex::BoxArray& boxArray (int lev) const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::BoxArray>& boxArrayAll () const = 0;
    [[nodiscard]] virtual const amrex::DistributionMapping& distributionMap (int lev) const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::DistributionMapping>& distributionMapAll () const = 0;
    [[nodiscard]] virtual amrex::Array<amrex::Real, 3> cellSize (int lev) const = 0;
    [[nodiscard]] virtual amrex::XDim3 lowerCorner (const amrex::Box& box, int lev, amrex::Real delta = 0.0) const = 0;
    [[nodiscard]] virtual amrex::XDim3 upperCorner (const amrex::Box& box, int lev) const = 0;
    [[nodiscard]] virtual amrex::RealBox realBox (const amrex::Box& box, int lev) const = 0;
    [[nodiscard]] virtual amrex::XDim3 invCellSize (int lev) const = 0;
    [[nodiscard]] virtual amrex::IntVect ngEB () const = 0;
    [[nodiscard]] virtual amrex::IntVect ngDeposRho () const = 0;
    [[nodiscard]] virtual amrex::IntVect ngDeposJ () const = 0;
    [[nodiscard]] virtual bool useVerboncoeurAxisCorrection () const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::Real>& vGalilean () const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::Real>& dtAll () const = 0;
    [[nodiscard]] virtual IHybridPICModel* hybridPICModel () const = 0;
    [[nodiscard]] virtual const MaterialRegistry& materials () const = 0;
    [[nodiscard]] virtual const FieldRegistry& fieldRegistry () const = 0;
    [[nodiscard]] virtual const amrex::EB2::IndexSpace* ebIndexSpace (int lev) const = 0;
    [[nodiscard]] virtual IFilterBank& filterBank () = 0;
    [[nodiscard]] virtual const IFilterBank& filterBank () const = 0;
    [[nodiscard]] virtual amrex::IntVect blockingFactor (int lev) const = 0;
    [[nodiscard]] virtual amrex::IntVect numProcs () const = 0;
    [[nodiscard]] virtual amrex::IntVect maxGridSize (int lev) const = 0;
    [[nodiscard]] virtual amrex::IntVect refRatio (int lev) const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::IntVect>& refRatioAll () const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::IntVect>& refRatioVector () const = 0;
    [[nodiscard]] virtual const amrex::Array<amrex::Real, 3>& galileanShift () const = 0;

    [[nodiscard]] virtual amrex::Real t_new (int lev) const = 0;
    [[nodiscard]] virtual amrex::Real dt (int lev) const = 0;
    [[nodiscard]] virtual int istep (int lev) const = 0;
    [[nodiscard]] virtual amrex::Vector<int> istepVector () const = 0;
    [[nodiscard]] virtual EvolveScheme evolveScheme () const = 0;

    [[nodiscard]] virtual amrex::LayoutData<amrex::Real>* costs (int lev) const = 0;
    [[nodiscard]] virtual amrex::Real loadBalanceEfficiency (int lev) const = 0;
    [[nodiscard]] virtual ablastr::utils::text::IntervalsParser loadBalanceIntervals () const = 0;
    [[nodiscard]] virtual int maxStep () const = 0;
    [[nodiscard]] virtual amrex::Real stopTime () const = 0;
    [[nodiscard]] virtual bool isSynchronized () const = 0;
    virtual void computeCostsHeuristic () const = 0;

    //! Embedded-boundary factory (only valid when AMREX_USE_EB is on).
    [[nodiscard]] virtual const amrex::EBFArrayBoxFactory& fieldEBFactory (int lev) const = 0;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_RUNTIMEDATA_H_
