#ifndef WARPX_RUNTIME_IPARTICLECONTAINER_H_
#define WARPX_RUNTIME_IPARTICLECONTAINER_H_
#include "Modules/Particles/IParticleAttributeAccess.H"
#include "Modules/Particles/IQEDEngineAccess.H"
#include "Modules/Particles/ParticleAttributes.H"
#include "Modules/Particles/SpeciesPhysicalProperties.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX_ParticleContainer.H>
#include <AMReX_REAL.H>
#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <utility>
namespace warpx::runtime {
class RuntimeContext;
class RuntimeData;
class RuntimeConfig;
/** \brief Project particle-container module base (AMReX-backed). */
class IParticleContainer
    : public amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>,
      public IParticleAttributeAccess,
      public IQEDEngineAccess
{
public:
    using Base = amrex::ParticleContainerPureSoA<PIdx::nattribs, 0, amrex::PolymorphicArenaAllocator>;
    using Base::Base;
    using ParticleTileType = typename Base::ParticleTileType;
    ~IParticleContainer () override = default;
    [[nodiscard]] virtual const RuntimeConfig& config () const = 0;
    [[nodiscard]] virtual const RuntimeData& particleData () const = 0;
    [[nodiscard]] virtual int getIonizationInitialLevel () const = 0;
    [[nodiscard]] virtual bool isPhysicalSpecies (PhysicalSpecies species) const = 0;

    [[nodiscard]] int NumRealComps () const { return Base::NumRealComps(); }
    bool HasRealComp (const std::string& n) { return Base::HasRealComp(n); }
    int GetRealCompIndex (const std::string& n) { return Base::GetRealCompIndex(n); }

    struct MomentumSum { amrex::Real Px{0}, Py{0}, Pz{0}, W{0}; };
    [[nodiscard]] virtual const std::string& getName () const = 0;
    [[nodiscard]] virtual amrex::Real getCharge () const = 0;
    [[nodiscard]] virtual amrex::Real getMass () const = 0;
    [[nodiscard]] virtual int finestLevel () const = 0;
    [[nodiscard]] virtual std::int64_t totalNumberOfParticles () const = 0;
    [[nodiscard]] virtual amrex::Real sumParticleWeight (bool local) const = 0;
    [[nodiscard]] virtual std::pair<amrex::Real, amrex::Real>
    sumParticleWeightAndEnergy (bool local) const = 0;
    [[nodiscard]] virtual bool isPhoton () const = 0;
    [[nodiscard]] virtual int DoQED () const = 0;
    [[nodiscard]] virtual MomentumSum sumParticleMomentum () const = 0;
    [[nodiscard]] virtual const RuntimeContext* runtime_context () const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::ParticleReal>& EExternalParticle () const = 0;
    [[nodiscard]] virtual const amrex::Vector<amrex::ParticleReal>& BExternalParticle () const = 0;
    [[nodiscard]] virtual amrex::Real currentInjectionPosition () const = 0;
    virtual void setCurrentInjectionPosition (amrex::Real) = 0;
    [[nodiscard]] virtual int doNotDeposit () const = 0;
    [[nodiscard]] virtual bool getTemperatureDepositionFlag () const noexcept = 0;
    [[nodiscard]] virtual std::string getSpeciesTypeName () const = 0;
    virtual void deleteInvalidParticles () = 0;
    virtual void defineAllParticleTiles () noexcept = 0;
    [[nodiscard]] virtual bool has_virtual_photons () const = 0;
    [[nodiscard]] virtual bool has_virtual_photons_beam_size_effect () const = 0;
    [[nodiscard]] virtual int getVirtualPhotonSpeciesIndex () const = 0;
    virtual void DepositCurrent (ablastr::fields::MultiLevelVectorField const& J,
                                 amrex::Real dt, amrex::Real relative_time,
                                 PushType push_type = PushType::Explicit,
                                 std::optional<int> particle_shape = std::nullopt) = 0;
    virtual void DepositCharge (ablastr::fields::MultiLevelScalarField const& rho,
                                bool local = false, bool reset = false,
                                bool interpolate_across_levels = true,
                                int icomp = 0) = 0;
    virtual void DepositTotalNGPTemperature (int lev) = 0;
    [[nodiscard]] virtual std::unique_ptr<amrex::MultiFab>
    GetChargeDensityOnLevel (int lev, bool local, bool finalize) = 0;
};
}
#endif
