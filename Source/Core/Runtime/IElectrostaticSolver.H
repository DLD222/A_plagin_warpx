#ifndef WARPX_RUNTIME_IElectrostaticSOLVER_H_
#define WARPX_RUNTIME_IElectrostaticSOLVER_H_
#include <ablastr/fields/MLMGOptions.H>
#include <ablastr/fields/MultiFabRegister.H>
#include "Runtime/ParticleContext.H"

#include <string>

#include <AMReX_Geometry.H>
#include <array>
#include <optional>
class MultiFluidContainer;
namespace warpx::runtime {
struct RuntimeContext;
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
    //! MLMG options used when solving for phi (generic accessor: callers must
    //! not need the concrete solver type).
    [[nodiscard]] virtual const ablastr::fields::MLMGOptions& mlmgOptions () const = 0;
    //! Set the potential on a domain boundary (axis, high/low side) and rebuild
    //! the parsers; an empty string leaves that boundary unchanged.
    virtual void setPotentialBoundaryString (int axis, bool high, const std::string& value) = 0;
};

/**
 * \brief Electrostatic solver factory (S6): the kernel asks, the field-solver
 * plugin answers.  Returns nullptr when the plugin is not linked.
 */
using ElectrostaticSolverFactory = std::unique_ptr<IElectrostaticSolver> (*) (
    int algorithm, int max_level, const RuntimeContext* context);

void register_electrostatic_solver_factory (ElectrostaticSolverFactory factory);

[[nodiscard]] std::unique_ptr<IElectrostaticSolver>
create_electrostatic_solver (int algorithm, int max_level,
                            const RuntimeContext* context);

[[nodiscard]] bool has_electrostatic_solver_factory ();

} // namespace warpx::runtime
#endif
