#ifndef WARPX_RUNTIME_IPARTICLESERVICES_H_
#define WARPX_RUNTIME_IPARTICLESERVICES_H_
#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/utils/Enums.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>
#include <memory>
#include <string>
namespace warpx::runtime {
class IParticleServices {
public:
    virtual ~IParticleServices () = default;
    virtual void depositCurrent (int, amrex::Real) = 0;
    virtual std::unique_ptr<amrex::MultiFab> computeRho (int, int, bool) = 0;
    virtual void syncRho (ablastr::fields::MultiLevelScalarField const&, ablastr::fields::MultiLevelScalarField const&, ablastr::fields::MultiLevelScalarField const&) = 0;
    virtual void finalizeRhoOnLevel (int, amrex::MultiFab*, ablastr::utils::enums::PatchType) = 0;
    virtual void applyFilterandSumBoundaryRho (int, int, amrex::MultiFab&, int, int) = 0;
    virtual void pushParticlesAndDeposit (amrex::Real, amrex::Real, int) = 0;
    virtual void finalizeDeposition () = 0;
    virtual void calculateNuei (const std::string&) = 0;
    virtual void syncCurrent (const std::string&) = 0;
    virtual void applyInverseVolumeScalingToCurrentDensity (amrex::MultiFab*, amrex::MultiFab*, amrex::MultiFab*, int) = 0;
};
}
#endif
