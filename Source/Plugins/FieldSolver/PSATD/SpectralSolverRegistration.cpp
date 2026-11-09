#include "Runtime/SpectralSolverFactory.H"
#include "Runtime/RuntimeContext.H"

#include "FieldSolver/PSATD/SpectralSolver.H"
#include "FieldSolver/PSATD/SpectralSolverRZ.H"

#include <memory>

//! Linker anchor: keeps this registration object in the aggregate archive.
extern "C" void warpx_spectral_anchor (void) {}

namespace warpx::runtime
{
namespace
{
std::unique_ptr<ISpectralSolver>
make_spectral_solver (SpectralSolverConfig const& config)
{
    std::unique_ptr<ISpectralSolver> solver;
#ifdef WARPX_DIM_RZ
    auto pss = std::make_unique<SpectralSolverRZ>(config.lev,
                                                  *config.realspace_ba,
                                                  *config.dm,
                                                  config.n_rz_azimuthal_modes,
                                                  config.norder_z,
                                                  config.grid_type,
                                                  config.v_galilean,
                                                  config.dx,
                                                  config.dt,
                                                  config.pml,
                                                  config.update_with_rho,
                                                  config.fft_do_time_averaging,
                                                  config.time_dependency_J,
                                                  config.time_dependency_rho,
                                                  config.dive_cleaning,
                                                  config.divb_cleaning,
                                                  config.context != nullptr ? config.context->data
                                                                            : nullptr);
    if (config.use_kspace_filter) {
        pss->InitFilter(config.filter_npass_each_dir, config.use_filter_compensation);
    }
    solver = std::move(pss);
#else
    auto pss = std::make_unique<SpectralSolver>(*config.realspace_ba,
                                                *config.dm,
                                                config.norder_x,
                                                config.norder_y,
                                                config.norder_z,
                                                config.grid_type,
                                                config.v_galilean,
                                                config.v_comoving,
                                                config.dx,
                                                config.dt,
                                                config.pml,
                                                config.periodic_single_box,
                                                config.update_with_rho,
                                                config.fft_do_time_averaging,
                                                config.psatd_solution_type,
                                                config.time_dependency_J,
                                                config.time_dependency_rho,
                                                config.dive_cleaning,
                                                config.divb_cleaning,
                                                config.context);
    solver = std::move(pss);
#endif
    return solver;
}

const bool spectral_factory_registered = [] {
    register_spectral_solver_factory(&make_spectral_solver);
    return true;
}();
} // namespace
} // namespace warpx::runtime
