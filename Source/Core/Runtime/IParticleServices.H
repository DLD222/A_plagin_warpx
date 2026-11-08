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
    //! Deposit charge from every species into `rho` (MPC-level operation).
    virtual void depositCharge (ablastr::fields::MultiLevelScalarField const& rho,
                                amrex::Real relative_time) = 0;
    virtual std::unique_ptr<amrex::MultiFab> computeRho (int, int, bool) = 0;
    //! Deposit rho and validate its layout.  In RZ the deposit legitimately has
    //! 2*n_modes-1 components while a Cartesian diagnostic asks for one, so the
    //! caller states whether extra components are expected (`allow_extra`).
    virtual std::unique_ptr<amrex::MultiFab> computeRho (int lev, int species, bool filter,
                                                         int ncomp, const amrex::IntVect& nodal,
                                                         const amrex::IntVect& ngrow,
                                                         bool allow_extra = false) = 0;
    virtual void syncRho (ablastr::fields::MultiLevelScalarField const&, ablastr::fields::MultiLevelScalarField const&, ablastr::fields::MultiLevelScalarField const&) = 0;
    virtual void finalizeRhoOnLevel (int, amrex::MultiFab*, ablastr::utils::enums::PatchType) = 0;
    virtual void applyFilterandSumBoundaryRho (int, int, amrex::MultiFab&, int, int) = 0;
    virtual void pushParticlesAndDeposit (amrex::Real, amrex::Real, int) = 0;
    virtual void finalizeDeposition () = 0;
    virtual void calculateNuei (const std::string&) = 0;
    virtual void syncCurrent (const std::string&) = 0;
    virtual void applyInverseVolumeScalingToCurrentDensity (amrex::MultiFab*, amrex::MultiFab*, amrex::MultiFab*, int) = 0;
    virtual void transformMomentumToCurvilinear (bool) = 0;
    virtual void generateGlobalDebyeLength () = 0;
    virtual void generateVirtualPhotons () = 0;
};
}
#endif
