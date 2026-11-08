#ifndef WARPX_RUNTIME_IElectrostaticSOLVER_H_
#define WARPX_RUNTIME_IElectrostaticSOLVER_H_
#include <ablastr/fields/MLMGOptions.H>
#include <ablastr/fields/MultiFabRegister.H>
#include "Runtime/ParticleContext.H"

#include <AMReX_Geometry.H>
#include <array>
#include <optional>
class MultiFluidContainer;
namespace warpx::runtime {
/** Electrostatic field-solver plugin contract (A7.3.2a). */
class IElectrostaticSolver
{
public:
    virtual ~IElectrostaticSolver () = default;
    virtual void InitData () = 0;
    virtual void ComputeSpaceChargeField (
        ablastr::fields::MultiFabRegister& fields,
        ParticleContext const& particles,
        MultiFluidContainer* fluids,
        int max_level, bool verbose_step) = 0;
    [[nodiscard]] virtual bool boundaryPotentialSpecified () const = 0;
    virtual void computePhi (ablastr::fields::MultiLevelScalarField const& rho,
                             ablastr::fields::MultiLevelScalarField const& phi,
                             std::array<amrex::Real,3> beta,
                             ablastr::fields::MLMGOptions const& options,
                             bool is_igf_2d_slices,
                             std::optional<ablastr::fields::MultiLevelVectorField> efield = std::nullopt) const = 0;
    virtual void definePhiBCs (const amrex::Geometry& geom) = 0;
};
}
#endif
