/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_RUNTIME_NULLPARTICLESYSTEM_H_
#define WARPX_RUNTIME_NULLPARTICLESYSTEM_H_

#include "Runtime/IParticleSystem.H"
#include "Runtime/LegacyServices.H"

#include <stdexcept>

namespace warpx::runtime
{

/** A particle system for a run that has none.
 *
 * When the particle plugin is not registered the kernel still calls particleSystem() on every
 * step - to ask how many species deposit, to redistribute, and so on.  Returning this instead of
 * dereferencing a null pointer turns "no particles" into an ordinary empty system: every count
 * is zero and every operation is a no-op, which is exactly what a field-only run should do.
 *
 * It is a null *object*, not a null pointer, on purpose: a guard per call site would be dozens of
 * ifs scattered through the kernel, and the next one would be forgotten.
 */
class NullParticleSystem : public IParticleSystem
{
public:
    [[nodiscard]] const RuntimeData& data () const override { return legacy_data(); }

    void setRuntimeContext (const RuntimeContext*) override {}
    void AllocData () override {}
    void defineAllParticleTiles () override {}
    void InitData () override {}
    [[nodiscard]] amrex::Real maxParticleDtInv () const override { return amrex::Real(0.0); }
    void Evolve (ablastr::fields::MultiFabRegister&, int, std::string const&, amrex::Real,
                 amrex::Real, SubcyclingHalf = SubcyclingHalf::None, bool = false,
                 PositionPushType = PositionPushType::Full,
                 MomentumPushType = MomentumPushType::Full,
                 ImplicitOptions const* = nullptr) override {}
    void DepositCharge (const ablastr::fields::MultiLevelScalarField&, amrex::Real) override {}
    void DepositCurrent (const ablastr::fields::MultiLevelVectorField&, amrex::Real, amrex::Real,
                         PushType = PushType::Explicit,
                         std::optional<int> = std::nullopt) override {}
    void DepositTemperatures (ablastr::fields::MultiFabRegister&, amrex::Real) override {}
    void PushP (int, amrex::Real, const amrex::MultiFab&, const amrex::MultiFab&,
                const amrex::MultiFab&, const amrex::MultiFab&, const amrex::MultiFab&,
                const amrex::MultiFab&, MomentumPushType,
                std::optional<warpx::particles::FieldGatherOrders> = std::nullopt) override {}
    void PushX (amrex::Real) override {}
    void Redistribute () override {}
    void RedistributeLocal (const amrex::IntVect&) override {}
    void AddImplicitParticleAttributes (bool) override {}
    void ClearImplicitParticleVelocities () override {}
    void FinishImplicitVelocityUpdate () override {}
    void PrepareImplicitVelocitiesForDeposition () override {}
    void SaveParticlesAtImplicitStepStart (int) override {}
    void FinishImplicitParticleUpdate (ablastr::fields::MultiFabRegister&, int, amrex::Real,
                                       const amrex::Vector<amrex::Real>&) override {}
    void ResetImplicitParticleData (int) override {}
    void UpdateInjectionPositions (amrex::Real, amrex::Real, const amrex::Vector<int>&, int,
                                   amrex::Real) override {}
    void countParticlesWithSuborbits (int, int, amrex::Long*) const override {}
    void AddParticleCosts (int, amrex::LayoutData<amrex::Real>&, amrex::Real) const override {}
    void doResampling (const amrex::Vector<amrex::Geometry>&, int, bool) override {}
    void doQedEvents (int, const amrex::MultiFab&, const amrex::MultiFab&,
                      const amrex::MultiFab&, const amrex::MultiFab&, const amrex::MultiFab&,
                      const amrex::MultiFab&) override {}
    [[nodiscard]] int nContainers () const override { return 0; }
    [[nodiscard]] int nSpeciesDepositOnMainGrid () const override { return 0; }
    [[nodiscard]] int nSpeciesGatherFromMainGrid () const override { return 0; }
    [[nodiscard]] const ExternalParticleFields& externalParticleFieldsMetadata () const override
    {
        static const ExternalParticleFields empty{};
        return empty;
    }
    void PostRestart () override {}
    void SetParticleBoxArray (int, amrex::BoxArray&) override {}
    void SetParticleDistributionMap (int, amrex::DistributionMapping&) override {}
    void SortParticlesByBin (const amrex::IntVect&, bool, const amrex::IntVect&) override {}
    void deleteInvalidParticles () override {}
    void ApplyBoundaryConditions () override {}
    void ScrapeParticlesAtEB (ablastr::fields::MultiLevelScalarField const&) override {}
    void TransformMomentumToCurvilinear (bool) override {}
    void doFieldIonization (int, const amrex::MultiFab&, const amrex::MultiFab&,
                            const amrex::MultiFab&, const amrex::MultiFab&,
                            const amrex::MultiFab&, const amrex::MultiFab&) override {}
    void doQEDSchwinger () override {}
    void DepositMassMatrices (ablastr::fields::MultiFabRegister&, int, amrex::Real,
                              ImplicitOptions const&) override {}
    void ContinuousFluxInjection (amrex::Real, amrex::Real) const override {}
    void UpdateAntennaPosition (amrex::Real) const override {}
    void CalculateNuei (std::string const&) override {}
    void GenerateGlobalDebyeLength () override {}
    [[nodiscard]] std::unique_ptr<amrex::MultiFab> GetGlobalPlasmaFrequency (int) override
    {
        return nullptr;
    }
    [[nodiscard]] RepeatedPlasmaLens repeatedPlasmaLens () const override { return {}; }
    [[nodiscard]] const amrex::Parser* externalFieldParser (const std::string&) const override
    {
        return nullptr;
    }
    [[nodiscard]] const void* acceleratorLattice (int) const override { return nullptr; }
    [[nodiscard]] std::unique_ptr<amrex::MultiFab>
    GetChargeDensityOnLevel (int, bool, bool) override
    {
        return nullptr;
    }

    // ---- IParticleContainerAccess / ISpeciesMetadata ------------------------
    // A run with no particle system has no containers to hand out.  The counts are zero, which
    // makes every kernel loop a no-op; the accessors below would be a programming error to call
    // and say so rather than returning a dangling reference.
    [[nodiscard]] int nSpecies () const override { return 0; }
    [[nodiscard]] std::vector<std::string> GetSpeciesNames () const override { return {}; }
    [[nodiscard]] std::vector<std::string> GetSpeciesAndLasersNames () const override { return {}; }
    [[nodiscard]] int getSpeciesID (const std::string&) const override { return -1; }
    [[nodiscard]] amrex::Vector<amrex::Long> NumberOfParticlesInGrid (int) const override
    {
        return {};
    }
    [[nodiscard]] bool initializesSelfFields (int) const override { return false; }
    [[nodiscard]] IParticleContainer& GetParticleContainer (int) const override { no_containers(); }
    [[nodiscard]] IParticleContainer& GetParticleContainerFromName (const std::string&) const override
    { no_containers(); }
    [[nodiscard]] IParticleContainer* GetParticleContainerPtr (int) const override { return nullptr; }
    [[nodiscard]] IParticleContainer& GetParticleView (int) const override { no_containers(); }
    [[nodiscard]] IParticleContainer& GetParticleView (const std::string&) const override
    { no_containers(); }
    [[nodiscard]] IParticleContainer& GetParticleInterface (int) const override { no_containers(); }
    void Increment (amrex::MultiFab&, int) override {}
    void ReadHeader (std::istream&) override {}
    void WriteHeader (std::ostream&) const override {}
    void Restart (const std::string&) override {}
    void SetDoBackTransformedParticles (bool) override {}
    void SetDoBackTransformedParticles (const std::string&, bool) override {}

private:
    [[noreturn]] static void no_containers ()
    {
        throw std::logic_error(
            "This run has no particle system: the particle plugin was not registered.");
    }
};

//! The one instance every caller shares.
[[nodiscard]] inline NullParticleSystem& null_particle_system ()
{
    static NullParticleSystem system;
    return system;
}

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_NULLPARTICLESYSTEM_H_
