/* Copyright 2024 Justin Angus
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "ImplicitKernelServicesBuilder.H"
#include "WarpX.H"
#include "Fields.H"
#include "SemiImplicitEM.H"

using warpx::fields::FieldType;
using namespace amrex::literals;

void SemiImplicitEM::Define (WarpX*  a_WarpX, bool  a_from_restart)
{
    BL_PROFILE("SemiImplicitEM::Define()");

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        !m_is_defined,
        "SemiImplicitEM object is already defined!");

    // Retain a pointer back to main WarpX class
    m_WarpX = a_WarpX;
    m_services = BuildImplicitKernelServices(*a_WarpX);

    // Define E and Eold vectors
    BuildSolverVectorContext();
    m_E.Define(m_solver_vec_context, "Efield_fp");
    m_Eold.Define(m_E);
    m_E_save.Define(m_E);

    // Set initial values for E and Eold vectors
    m_E.Copy(FieldType::Efield_fp);
    m_Eold.Copy(a_from_restart ? FieldType::E_old : FieldType::Efield_fp, FieldType::None, true);

    // Define B_old MultiFab
    // This is only needed for substepping
    using ablastr::fields::Direction;
    for (int lev = 0; lev < m_num_amr_levels; ++lev) {
        const auto& ba_Bx = m_services.fields->get(FieldType::Bfield_fp, Direction{0}, lev)->boxArray();
        const auto& ba_By = m_services.fields->get(FieldType::Bfield_fp, Direction{1}, lev)->boxArray();
        const auto& ba_Bz = m_services.fields->get(FieldType::Bfield_fp, Direction{2}, lev)->boxArray();
        const auto& dm = m_services.fields->get(FieldType::Bfield_fp, Direction{0}, lev)->DistributionMap();
        const amrex::IntVect ngb = m_services.fields->get(FieldType::Bfield_fp, Direction{0}, lev)->nGrowVect();
        const int ncomp = m_services.fields->get(FieldType::Bfield_fp, Direction{0}, lev)->nComp();
        m_services.fields->alloc_init(FieldType::B_old, Direction{0}, lev, ba_Bx, dm, ncomp, ngb, 0.0_rt);
        m_services.fields->alloc_init(FieldType::B_old, Direction{1}, lev, ba_By, dm, ncomp, ngb, 0.0_rt);
        m_services.fields->alloc_init(FieldType::B_old, Direction{2}, lev, ba_Bz, dm, ncomp, ngb, 0.0_rt);
    }

    // Reconstruct the initial guess E^{n-1/2} from checkpoint fields E^{n-1} and E^n.
    // On a fresh start, both copies contain E^0, giving initial guess E^0.
    m_E.linComb(1.0_rt - m_theta, m_Eold, m_theta, m_E);

    // Parse implicit solver parameters
    const amrex::ParmParse pp("implicit_evolve");
    parseNonlinearSolverParams(pp);

    // Define the nonlinear solver
    m_nlsolver->Define(m_E, this);

    // Initialize the mass matrices for plasma response
    if (m_use_mass_matrices) { InitializeMassMatrices(config().nox()); }
    InitializeImplicitOptions();

    m_is_defined = true;

}

void SemiImplicitEM::PrintParameters () const
{
    if (!m_services.verbose()) { return; }
    amrex::Print() << "\n";
    amrex::Print() << "-----------------------------------------------------------\n";
    amrex::Print() << "----------- SEMI IMPLICIT EM SOLVER PARAMETERS ------------\n";
    amrex::Print() << "-----------------------------------------------------------\n";
    PrintBaseImplicitSolverParameters();
    m_nlsolver->PrintParams();
    amrex::Print() << "-----------------------------------------------------------\n\n";
}

void SemiImplicitEM::SetupStep (amrex::Real start_time)
{
    // Save particle position and velocity at the start of the time step
    // Copy x to x_n etc
    m_services.save_particles_at_implicit_step_start(0);

    // Particles at t_{n}
    // Efield_fp is at t_{n}
    // Bfield_fp is at t_{n}
    // m_E is at t_{n-1/2}

    // Save E at start of time step
    SaveEoldMultifab(); // Copy Efield_fp into E_old
    m_Eold.Copy(FieldType::Efield_fp); // Copy Efield_fp into m_Eold
    m_E_save.Copy(m_E); // Copy m_E to m_E_save to save the field at t_{n-1/2}

    // E_old is at t_{n}
    // m_Eold is at t_{n}
    // m_E_save is at t_{n-1/2}

    // Save Bg at start of time step
    // In case it is needed to reset B if the nonlinear solver fails and substepping is used
    CopyVectorField(FieldType::B_old, FieldType::Bfield_fp);

    // B_old is at t_{n}

    // Advance WarpX owned Bfield_fp from t_{n} to t_{n+1/2}
    m_services.evolve_b(0.5_rt*m_dt, SubcyclingHalf::FirstHalf, start_time);
    m_services.fill_boundary_b(m_services.ng_eb(), true);

    // Bfield_fp is at t_{n+1/2}
}

int SemiImplicitEM::DoSolve (const amrex::Real start_time,
                             const int a_step,
                             const bool verbose_step)
{
    // Particles will be advanced to t_{n+1/2}
    // Note that initial guess for m_E is that from previous solve: E^{n-1+theta}
    m_nlsolver->Solve(m_E, m_Eold, start_time, m_dt, a_step, verbose_step);
    // Particles at t_{n+1/2}
    // m_E is at t_{n+1/2}
    return m_nlsolver->GetExitStatus();
}

void SemiImplicitEM::ResetStep (amrex::Real start_time)
{
    // Reconstitue m_E
    // m_E_save is at t_{n-1/2}
    // Copy it to m_E
    m_E.Copy(m_E_save);
    // m_E is at t_{n-1/2}

    m_services.reset_implicit_particle_data();

    // Restore E^n, including guards, before recomputing the magnetic half-step.
    // The failed solve leaves Efield_fp containing its last nonlinear iterate.
    CopyVectorField(FieldType::Efield_fp, FieldType::E_old);

    // Reset B field to start of step
    CopyVectorField(FieldType::Bfield_fp, FieldType::B_old);

    // Advance WarpX owned Bfield_fp from t_{n} to t_{n+1/2}
    m_services.evolve_b(0.5_rt*m_dt, SubcyclingHalf::FirstHalf, start_time);
    m_services.fill_boundary_b(m_services.ng_eb(), true);
    // Bfield_fp is at t_{n+1/2} (with new smaller time step)
}

void SemiImplicitEM::FinishStep (const amrex::Real start_time, const int a_step)
{
    const amrex::Real half_time = start_time + 0.5_rt*m_dt;

    // Update WarpX owned Efield_fp to t_{n+1/2}
    m_services.set_electric_field_and_apply_bcs(m_E, half_time);
    m_services.compute_diags_mid_step(a_step, m_dt);

    // Efield_fp is at t_{n+1/2}

    const amrex::Real new_time = start_time + m_dt;

    // Advance particles from t_{n+1/2} to t_{n+1}
    FinishImplicitParticleUpdate(new_time, a_step);
    if (m_nsubsteps > 1) {
        m_services.handle_particles_at_boundaries(a_step, new_time, 0);
    }

    // Particles at t_{n+1}

    // Update the WarpX-owned Efield_fp, preserving m_E at E^{n+1/2}
    // as the initial guess for the next nonlinear solve. E_old retains E^n
    // for checkpointing alongside Efield_fp at E^{n+1}.
    // E^{n+1} = 2*E^{n+1/2} - E^n
    m_services.finish_electric_field_and_apply_bcs(m_theta, new_time);

    // Efield_fp is at t_{n+1}
    // m_E is at t_{n+1/2}
    // m_E_save is at t_{n-1/2}
    // E_old is at t_{n}
    // m_Eold is at t_{n}

    // Advance WarpX owned Bfield_fp from t_{n+1/2} to t_{n+1}
    m_services.evolve_b(0.5_rt*m_dt, SubcyclingHalf::SecondHalf, half_time);
    m_services.fill_boundary_b(m_services.ng_eb(), true);

    // Bfield_fp is at t_{n+1}
}

void SemiImplicitEM::ComputeRHS (WarpXSolverVec& a_RHS,
                           const WarpXSolverVec& a_E,
                                 amrex::Real     start_time,
                                 int             a_nl_iter,
                                 bool            a_from_jacobian,
                                 const bool      update_pc)
{
    BL_PROFILE("SemiImplicitEM::ComputeRHS()");

    // Update WarpX-owned Efield_fp using current state of E from
    // the nonlinear solver at time n+1/2
    const amrex::Real half_time = start_time + 0.5_rt*m_dt;
    m_services.set_electric_field_and_apply_bcs( a_E, half_time );

    // Update particle positions and velocities using the current state
    // of E and B. Deposit current density at time n+1/2
    const amrex::Real dt_scale = 1.0_rt/m_nsubsteps;
    PreRHSOp(half_time, a_nl_iter, a_from_jacobian, dt_scale, update_pc);

    // RHS = cvac^2*0.5*dt*(curl(B^{n+1/2}) - mu0*J^{n+1/2})
    m_services.implicit_compute_rhse(0.5_rt*m_dt, a_RHS);
}
