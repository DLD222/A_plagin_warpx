/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * Authors: Roelof Groenewald (Realta Fusion)
 *
 * License: BSD-3-Clause-LBNL
 */
#include "Fields.H"
#include "SemiImplicitDarwin.H"
#include "Utils/Parser/ParserUtils.H"

#include <ablastr/warn_manager/WarnManager.H>

using warpx::fields::FieldType;
using namespace amrex::literals;

void SemiImplicitDarwin::Define ( warpx::runtime::ImplicitKernelServices const& a_services, bool from_restart)
{
    amrex::ignore_unused(from_restart);
    BL_PROFILE("SemiImplicitDarwin::Define()");

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        !m_is_defined,
        "SemiImplicitDarwin object is already defined!");

        m_services = a_services;

    // The guard-cell handling throughout this solver (SumBoundaryJ and
    // FillBoundaryAndSync calls using the domain periodicity) and the GMRES
    // operator in DarwinLinearFieldOperator assume periodic boundaries; with conducting
    // (PEC) walls the run would proceed but give wrong results near the walls.
    for (int lev = 0; lev < m_num_amr_levels; ++lev) {
        WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
            m_services.geom(lev).isAllPeriodic(),
            "The semi-implicit Darwin solver requires periodic field boundary "
            "conditions in all directions.");
    }

    // Define dA MultiFabs
    using ablastr::fields::Direction;
    for (int lev = 0; lev < m_num_amr_levels; ++lev) {
        const auto& ba_Ex = m_services.fields->get(FieldType::Efield_fp, Direction{0}, lev)->boxArray();
        const auto& ba_Ey = m_services.fields->get(FieldType::Efield_fp, Direction{1}, lev)->boxArray();
        const auto& ba_Ez = m_services.fields->get(FieldType::Efield_fp, Direction{2}, lev)->boxArray();
        const auto& dm_E = m_services.fields->get(FieldType::Efield_fp, Direction{0}, lev)->DistributionMap();
        const amrex::IntVect nge = m_services.fields->get(FieldType::Efield_fp, Direction{0}, lev)->nGrowVect();
        m_services.fields->alloc_init(FieldType::dA_fp, Direction{0}, lev, ba_Ex, dm_E, 1, nge, 0.0_rt);
        m_services.fields->alloc_init(FieldType::dA_fp, Direction{1}, lev, ba_Ey, dm_E, 1, nge, 0.0_rt);
        m_services.fields->alloc_init(FieldType::dA_fp, Direction{2}, lev, ba_Ez, dm_E, 1, nge, 0.0_rt);
    }

    // Define WarpXSolverVec instances for the magnetoinductive equation solution (Z) and source
    BuildSolverVectorContext();
    m_Z.Define(m_solver_vec_context, "Bfield_fp");
    m_Z.zero();
    m_source.Define(m_Z);
    m_source.zero();

    // Particle shape used for the magnetostatic part of the solve (current and
    // mass matrices deposition, inductive E-field gather, and B-field gather).
    // Defaults to the electrostatic particle shape, algo.particle_shape.
    m_magnetostatic_particle_shape = config().nox();
    const amrex::ParmParse pp_implicit_evolve("implicit_evolve");
    utils::parser::queryWithParser(
        pp_implicit_evolve, "magnetostatic_particle_shape", m_magnetostatic_particle_shape);
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        m_magnetostatic_particle_shape >= 1 && m_magnetostatic_particle_shape <= config().nox(),
        "implicit_evolve.magnetostatic_particle_shape must be between 1 and algo.particle_shape");

    // Set parameters used by `InitializeMassMatrices`
    m_use_mass_matrices = true;
    m_use_mass_matrices_pc = false;
    m_use_mass_matrices_jacobian = true;

    // Initialize the mass matrices for plasma response (their stencil width
    // is set by the magnetostatic particle shape). This has to come before the
    // linear operator is defined below, since the operator sizes its scratch
    // space from the mass matrix stencil.
    InitializeMassMatrices(m_magnetostatic_particle_shape);
    InitializeImplicitOptions();

    // Get the linear solver input parameters
    const amrex::ParmParse pp_l(amrex::getEnumNameString(m_linear_solver_type));
    pp_l.query("verbose_int",         m_linsol_verbose_int);
    pp_l.query("restart_length",      m_linsol_restart_length);
    pp_l.query("absolute_tolerance",  m_linsol_atol);
    pp_l.query("relative_tolerance",  m_linsol_rtol);
    pp_l.query("max_iterations",      m_linsol_maxits);
    pp_l.query("pc_type",             m_pc_type);
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
        m_pc_type == PreconditionerType::none ||
        m_pc_type == PreconditionerType::pc_darwin_mlmg,
        "The semi-implicit Darwin solver only supports pc_darwin_mlmg as "
        "the GMRES preconditioner (amrex_gmres.pc_type).");

    // Define the linear operator (this also allocates the scratch space it
    // uses to evaluate the operator on each GMRES iteration)
    m_linear_function = std::make_unique<DarwinLinearFieldOperator>();
    m_linear_function->define(m_Z, this, m_pc_type);

    // Define the linear solver
    if (m_linear_solver_type == LinearSolverType::amrex_gmres) {
        m_linear_solver = std::make_unique<AMReXGMRES<WarpXSolverVec,DarwinLinearFieldOperator>>();
    }
    else {
        amrex::Abort("Darwin linear solver: unknown type");
    }
    m_linear_solver->define(*m_linear_function);
    m_linear_solver->setVerbose( m_linsol_verbose_int );
    m_linear_solver->setRestartLength( m_linsol_restart_length );
    m_linear_solver->setMaxIters( m_linsol_maxits );

    // The predictor velocity push gathers the electrostatic E-field with the
    // Galerkin scheme. Skip that, and warn, if the user has explicitly selected
    // momentum-conserving gathering.
    m_predictor_use_galerkin = (config().field_gathering_algo() != GatheringAlgo::MomentumConserving);
    if (!m_predictor_use_galerkin) {
        ablastr::warn_manager::WMRecordWarning("Semi-implicit Darwin solver",
            "algo.field_gathering = momentum_conserving is set; the predictor "
            "velocity push will keep using momentum-conserving gathering "
            "rather than switching to the Galerkin scheme.",
            ablastr::warn_manager::WarnPriority::medium);
    }

    m_is_defined = true;
}

void SemiImplicitDarwin::PrintParameters () const
{
    amrex::Print() << "\n";
    amrex::Print() << "-----------------------------------------------------------\n";
    amrex::Print() << "--------- SEMI IMPLICIT DARWIN SOLVER PARAMETERS ----------\n";
    amrex::Print() << "-----------------------------------------------------------\n";

    auto linsol_name = amrex::getEnumNameString(m_linear_solver_type);
    amrex::Print()     << "Linear solver (" << linsol_name << ") verbose:            " << m_linsol_verbose_int << "\n";
    amrex::Print()     << "Linear solver (" << linsol_name << ") restart length:     " << m_linsol_restart_length << "\n";
    amrex::Print()     << "Linear solver (" << linsol_name << ") max iterations:     " << m_linsol_maxits << "\n";
    amrex::Print()     << "Linear solver (" << linsol_name << ") relative tolerance: " << m_linsol_rtol << "\n";
    amrex::Print()     << "Linear solver (" << linsol_name << ") absolute tolerance: " << m_linsol_atol << "\n";
    amrex::Print()     << "Linear solver (" << linsol_name << ") preconditioner:     " << amrex::getEnumNameString(m_pc_type) << "\n";
    amrex::Print()     << "Magnetostatic particle shape:                  " << m_magnetostatic_particle_shape << "\n";
    if (m_linear_function) { m_linear_function->printParameters(); }
    amrex::Print() << "-----------------------------------------------------------\n\n";
}

int SemiImplicitDarwin::OneStep ( [[maybe_unused]] amrex::Real  start_time,
                                                   amrex::Real  a_dt,
                                                   int          a_step,
                                                   bool verbose_step)
{
    BL_PROFILE("SemiImplicitDarwin::OneStep()");

    using ablastr::fields::Direction;

    // Set the member time step
    m_dt = a_dt;

    const int finest_level = 0;

    // Fields have E^{n} (from phi^n only), B^{n-1/2}
    // Particles have u^{n-1/2} and x^{n}.

    // The electrostatic solve at the end of the previous step only sets E in the
    // valid cells, so exchange guard cells now (this happens in
    // WarpX::ExplicitFillBoundaryEBUpdateAux for explicit solvers).
    m_services.fill_boundary_e(m_services.ng_eb(), false);

    // Save u and x at the start of the time step
    // TODO: only save u since we don't need to keep x
    m_services.save_particles_at_implicit_step_start(0);

    // Push particle velocities with E_fp (which currently just contains -grad(phi) since
    // the E-field was cleared during the last Poisson solve). E is gathered with the
    // Galerkin scheme (unless the user explicitly requested momentum-conserving
    // gathering), with the electrostatic particle shape. B is gathered exactly as in
    // the corrector push and the mass matrices, i.e., with the magnetostatic particle shape.
    for (int lev = 0; lev <= finest_level; ++lev)
    {
        m_services.push_particles_momentum(
            lev,
            m_dt,
            *m_services.fields->get(FieldType::Efield_fp, Direction{0}, lev),
            *m_services.fields->get(FieldType::Efield_fp, Direction{1}, lev),
            *m_services.fields->get(FieldType::Efield_fp, Direction{2}, lev),
            *m_services.fields->get(FieldType::Bfield_fp, Direction{0}, lev),
            *m_services.fields->get(FieldType::Bfield_fp, Direction{1}, lev),
            *m_services.fields->get(FieldType::Bfield_fp, Direction{2}, lev),
            MomentumPushType::Full,
            warpx::particles::FieldGatherOrders{/*e_shape=*/config().nox(), /*e_galerkin=*/m_predictor_use_galerkin,
                              /*b_shape=*/m_magnetostatic_particle_shape}
        );
    }

    // Prepare current deposition: the velocities are time centered with
    // u -> (u^{n+1/2} + u^{n-1/2}) / 2.0 (with just the ES acceleration applied
    // for the advanced velocity), and the advanced velocity is saved to u_n
    PrepareVelocitiesForCurrentDeposition();

    // Accumulate current* and the mass matrices (with the magnetostatic shape)
    AccumulateCurrentAndMassMatrices();

    // Python callback insertion
    if (m_services.run_python_callback) { m_services.run_python_callback("afterdeposition"); }

    // Populate the source vector
    // i.e. fill m_source with `2 * laplacian(B) + 2 * mu_0 curl(J)`
    CalculateSourceVector();

    // Refresh the preconditioner from the freshly deposited mass matrices
    // (no-op unless a preconditioner is enabled).
    m_linear_function->updatePreCondMat();

    // Solve the magnetoinductive equation:
    // bilaplacian(Z) + curl(chi curl(Z)) = 2 * laplacian(B) + 2 * mu_0 curl(J)
    // where chi is the mass matrix scaled by 2 * mu_0 / dt (see
    // ApplyScaledMassMatrices), i.e. the linear response of the deposited
    // current to the inductive E-field that this solve produces.
    int const verbosity = verbose_step ? m_linsol_verbose_int : 0;
    m_linear_solver->setVerbose(verbosity);
    m_linear_solver->solve(m_Z, m_source, m_linsol_rtol, m_linsol_atol);

    // Map a failed linear solve onto the negative-means-failure convention
    // used by the caller.
    const int exit_status = m_linear_solver->converged() ? 0 : -1;
    if (exit_status < 0) {
        return exit_status;
    }

    // Set E = -dA/dt (B is updated after the corrector push below)
    ComputeInductiveEfromdA(a_step);

    // Set particle velocities to 0 since the push below is just calculating
    // the acceleration due to the inductive E-field
    ClearParticleVelocities();

    // Push particle velocities (E-field now only includes the inductive component).
    // E and B are gathered with the magnetostatic particle shape and without order
    // reduction, as the mass matrices assume: the E gather is then the transpose of the
    // (direct) current deposition, and B is the same as in the predictor push and the
    // mass matrices.
    for (int lev = 0; lev <= finest_level; ++lev)
    {
        m_services.push_particles_momentum(
            lev,
            m_dt,
            *m_services.fields->get(FieldType::Efield_fp, Direction{0}, lev),
            *m_services.fields->get(FieldType::Efield_fp, Direction{1}, lev),
            *m_services.fields->get(FieldType::Efield_fp, Direction{2}, lev),
            *m_services.fields->get(FieldType::Bfield_fp, Direction{0}, lev),
            *m_services.fields->get(FieldType::Bfield_fp, Direction{1}, lev),
            *m_services.fields->get(FieldType::Bfield_fp, Direction{2}, lev),
            MomentumPushType::Full,
            warpx::particles::FieldGatherOrders{/*e_shape=*/m_magnetostatic_particle_shape,
                              /*e_galerkin=*/false,
                              /*b_shape=*/m_magnetostatic_particle_shape}
        );
    }

    // Update particle velocities to include acceleration from both
    // electrostatic and inductive electric field components
    FinishVelocityUpdate();

    // Push particle positions forward (velocities are already updated)
    m_services.push_particles_position(m_dt);

    // Update magnetic field using dB/dt = -curl(E)
    m_services.evolve_b(m_dt, SubcyclingHalf::None, a_step*m_dt);
    m_services.fill_boundary_b(m_services.ng_eb(), true);

    return exit_status;
}

void SemiImplicitDarwin::ComputeRHS ( [[maybe_unused]] WarpXSolverVec& a_RHS,
                                      [[maybe_unused]] const WarpXSolverVec& a_Z,
                                      [[maybe_unused]] amrex::Real start_time,
                                      [[maybe_unused]] int a_nl_iter,
                                      [[maybe_unused]] bool a_from_jacobian,
                                      [[maybe_unused]] const bool update_pc )
{
    // The Darwin scheme is linear in its unknown and never installs a
    // nonlinear solver, so it has no nonlinear residual to compute. This
    // override only exists because ImplicitSolver::ComputeRHS() is pure
    // virtual. The linear operator that GMRES applies each iteration is
    // DarwinLinearFieldOperator::apply() instead.
    WARPX_ABORT_WITH_MESSAGE(
        "SemiImplicitDarwin::ComputeRHS() is not implemented: the semi-implicit "
        "Darwin solver is linear and uses DarwinLinearFieldOperator::apply() instead.");
}


void SemiImplicitDarwin::AccumulateCurrentAndMassMatrices ()
{

    BL_PROFILE("SemiImplicitDarwin::AccumulateCurrentAndMassMatrices()");

    using warpx::fields::FieldType;

    const int lev = 0;

    // Deposit the current density from all species, using the time-centered
    // particle velocities as appropriate for the implicit push and the
    // magnetostatic particle shape. This also resets the current MultiFabs
    // before depositing.
    m_services.deposit_current_all_species(
        m_services.fields->get_mr_levels_alldirs(FieldType::current_fp, lev),
        m_dt, 0.0_rt, PushType::Implicit, m_magnetostatic_particle_shape);

    // Zero and accumulate the mass matrices from all species. This shares the
    // zero-then-deposit machinery with the electromagnetic implicit solvers
    // (see ImplicitSolver::PreLinearSolve), which drive the same
    // WarpX::DepositMassMatrices() -> MultiParticleContainer::DepositMassMatrices(),
    // here with the magnetostatic particle shape (set in m_implicit_options).
    m_services.deposit_mass_matrices(m_dt, m_implicit_options);

    // The deposit routine only fills half of each diagonal mass matrix's
    // band (exploiting symmetry); mirror the other half to complete
    // deposition before boundary summation.
    FinishMassMatricesDeposition();

    // Sync current (filter and sum boundaries)
    m_services.sync_current("current_fp");
    m_services.finalize_j();

    // Sum boundaries for mass matrices
    m_services.sync_mass_matrices();
}

void SemiImplicitDarwin::CalculateSourceVector ()
{
    // Compute the right-hand side of the magnetoinductive equation
    // bilaplacian(Z) + curl(chi curl(Z)) = 2 * laplacian(B) + 2 * mu_0 curl(J)
    // where chi is the mass matrix scaled by 2 * mu_0 / dt (see
    // ApplyScaledMassMatrices).
    BL_PROFILE("SemiImplicitDarwin::CalculateSourceVector()");

    const int lev = 0;

    // Zero out existing source values
    m_source.zero();

    // Grab the magnetic field and current density
    ablastr::fields::MultiLevelVectorField Bfield = m_services.fields->get_mr_levels_alldirs(FieldType::Bfield_fp, lev);
    ablastr::fields::MultiLevelVectorField jfield = m_services.fields->get_mr_levels_alldirs(FieldType::current_fp, lev);

    // Ensure guard cells are valid before differentiating these fields below -
    // this function doesn't otherwise control when Bfield_fp/current_fp were
    // last synced, so don't rely on that happening elsewhere.
    for (int ii = 0; ii < 3; ii++)
    {
        Bfield[lev][ii]->FillBoundaryAndSync(m_services.geom(lev).periodicity());
        jfield[lev][ii]->FillBoundaryAndSync(m_services.geom(lev).periodicity());
    }

    // Create temporary multifabs with B-staggering for storage
    amrex::MultiFab lapB_x(Bfield[lev][0]->boxArray(), Bfield[lev][0]->DistributionMap(),
                           Bfield[lev][0]->nComp(), Bfield[lev][0]->nGrowVect());
    amrex::MultiFab lapB_y(Bfield[lev][1]->boxArray(), Bfield[lev][1]->DistributionMap(),
                           Bfield[lev][1]->nComp(), Bfield[lev][1]->nGrowVect());
    amrex::MultiFab lapB_z(Bfield[lev][2]->boxArray(), Bfield[lev][2]->DistributionMap(),
                           Bfield[lev][2]->nComp(), Bfield[lev][2]->nGrowVect());
    ablastr::fields::VectorField lapB = {&lapB_x, &lapB_y, &lapB_z};

    amrex::MultiFab curlJ_x(Bfield[lev][0]->boxArray(), Bfield[lev][0]->DistributionMap(),
                            Bfield[lev][0]->nComp(), Bfield[lev][0]->nGrowVect());
    amrex::MultiFab curlJ_y(Bfield[lev][1]->boxArray(), Bfield[lev][1]->DistributionMap(),
                            Bfield[lev][1]->nComp(), Bfield[lev][1]->nGrowVect());
    amrex::MultiFab curlJ_z(Bfield[lev][2]->boxArray(), Bfield[lev][2]->DistributionMap(),
                            Bfield[lev][2]->nComp(), Bfield[lev][2]->nGrowVect());
    ablastr::fields::VectorField curlJ = {&curlJ_x, &curlJ_y, &curlJ_z};

    // Calculate the vector Laplacian of B and write result into first temporary MF
    m_services.fdtd_solver(lev)->ComputeVectorLaplacian(
        lapB, Bfield[lev], m_services.eb_update_b_flag()[lev], lev
    );

    // Calculate the curl of J and write result into second temporary MF
    m_services.fdtd_solver(lev)->ComputeCurlA(
        curlJ, jfield[lev], m_services.eb_update_b_flag()[lev], lev
    );

    // Calculate 2 * laplacian(B) + 2 * mu_0 curl(J) and write result in m_source
    const auto& b = m_source.getArrayVec();
    for (int ii = 0; ii < 3; ii++)
    {
        amrex::MultiFab::LinComb(
            *b[lev][ii], 2.0*PhysConst::mu0, *curlJ[ii], 0, 2.0, *lapB[ii], 0, 0, 1, 0
        );
    }

    // This is the RHS GMRES solves against for the entire step.
    for (int ii = 0; ii < 3; ii++)
    {
        b[lev][ii]->FillBoundaryAndSync(m_services.geom(lev).periodicity());
    }
}

void SemiImplicitDarwin::ComputeInductiveEfromdA ( int astep )
{
    // This function updates the Efield_fp MF to hold the new inductive E-field.
    BL_PROFILE("SemiImplicitDarwin::ComputeInductiveEfromdA()");

    const int lev = 0;

    // Grab the E-field MultiFabs
    ablastr::fields::MultiLevelVectorField Efield = m_services.fields->get_mr_levels_alldirs(FieldType::Efield_fp, lev);

    // Grab the dA_fp MultiFabs to store dA = curl(Z) (the solved-for Z lives
    // on B's staggering; dA lives on A/E's staggering)
    ablastr::fields::MultiLevelVectorField dAfield = m_services.fields->get_mr_levels_alldirs(FieldType::dA_fp, lev);

    // Grab m_Z MultiFabs (the solved-for Z). Z's valid region is sound at this
    // point - it is a linear combination of vectors that were themselves
    // consistent, and GMRES's arithmetic is element-wise - but WarpXSolverVec
    // always allocates with zero guard cells (see its Define()), so there are
    // none to fill in place, while the ComputeCurlB stencil below reads i-1.
    // Hence the copy into a local scratch one cell wider, and the boundary fill
    // on that, same as in the linear operator.
    const auto& Zfield = m_Z.getArrayVec();
    const amrex::IntVect curl_ng = amrex::IntVect(1);
    amrex::MultiFab Zscratch_x(Zfield[lev][0]->boxArray(), Zfield[lev][0]->DistributionMap(),
                               Zfield[lev][0]->nComp(), curl_ng);
    amrex::MultiFab Zscratch_y(Zfield[lev][1]->boxArray(), Zfield[lev][1]->DistributionMap(),
                               Zfield[lev][1]->nComp(), curl_ng);
    amrex::MultiFab Zscratch_z(Zfield[lev][2]->boxArray(), Zfield[lev][2]->DistributionMap(),
                               Zfield[lev][2]->nComp(), curl_ng);
    ablastr::fields::VectorField Zscratch = {&Zscratch_x, &Zscratch_y, &Zscratch_z};
    for (int ii = 0; ii < 3; ii++)
    {
        amrex::MultiFab::Copy(*Zscratch[ii], *Zfield[lev][ii], 0, 0, 1, 0);
        // Z's transverse components are nodal, so transverse end points are
        // *valid* cells possibly representing the same periodic-wrapped point.
        // Plain FillBoundary only reconciles true ghost cells, not two
        // overlapping valid cells - use FillBoundaryAndSync instead.
        Zscratch[ii]->FillBoundaryAndSync(m_services.geom(lev).periodicity());
    }

    // Calculate dA = curl(Z)
    m_services.fdtd_solver(lev)->ComputeCurlB(
        dAfield[lev], Zscratch, m_services.eb_update_e_flag()[lev], lev
    );
    for (int ii = 0; ii < 3; ii++)
    {
        dAfield[lev][ii]->FillBoundaryAndSync(m_services.geom(lev).periodicity());
    }

    const auto prefac = -1.0_rt / m_dt;
    for (int ii = 0; ii < 3; ii++)
    {
        // Copy dA values to E-field then scale by -1/dt
        amrex::MultiFab::Copy( *Efield[lev][ii], *dAfield[lev][ii], 0, 0, 1,
                                dAfield[lev][ii]->nGrowVect() );
        Efield[lev][ii]->mult(prefac, 0); // use zero ghost cells since FillBoundary is called below
    }

    // Apply E-field boundary
    m_services.apply_efield_boundary(0, PatchType::fine, astep*m_dt);
    m_services.fill_boundary_e(m_services.ng_eb(), true);
}



void SemiImplicitDarwin::ApplyScaledMassMatrices (
    ablastr::fields::MultiLevelVectorField& rhs,
    const ablastr::fields::MultiLevelVectorField& dA )
{
    BL_PROFILE("SemiImplicitDarwin::ApplyScaledMassMatrices()");
    using namespace amrex::literals;

    const amrex::Real scale = 2._prt * PhysConst::mu0 / m_dt;

    // The mass matrices were summed over their guard cells (SyncMassMatrices),
    // so that the stencil of a valid cell reaches one cell beyond the guard
    // cells of J (see guardCellManager::Init). Check that dA has enough guard
    // cells for the valid cells of rhs not to be clipped.
    AssertMassMatricesStencilNotClipped(rhs, dA);

    // No guard-cell exchange is needed on the result: ApplyMassMatrices()
    // already fills as many guard cells of `rhs` as `rhs` and the mass matrices
    // have in common, computing them from the same wide-stencil read of `dA`
    // that the valid region uses. The caller sizes `rhs` so that this covers
    // whatever it goes on to read.
    ApplyMassMatrices(
        /* a_out           = */ rhs,
        /* a_in            = */ dA,
        /* a_in_ref        = */ nullptr,
        /* a_baseline      = */ nullptr,
        /* a_scale         = */ scale,
        /* a_zero_out_first = */ false);
}

void SemiImplicitDarwin::ComputeScaledMassMatrixCC ( amrex::MultiFab& a_chi_xx_cc,
                                                     amrex::MultiFab& a_chi_yy_cc,
                                                     amrex::MultiFab& a_chi_zz_cc ) const
{
    BL_PROFILE("SemiImplicitDarwin::ComputeScaledMassMatrixCC()");

    using ablastr::fields::Direction;

    const int lev = 0;
    const amrex::MultiFab* Sdiag[3] = {
        m_services.fields->get(FieldType::MassMatrices_X, Direction{0}, lev),
        m_services.fields->get(FieldType::MassMatrices_Y, Direction{1}, lev),
        m_services.fields->get(FieldType::MassMatrices_Z, Direction{2}, lev)};
    amrex::MultiFab* chi_cc[3] = {&a_chi_xx_cc, &a_chi_yy_cc, &a_chi_zz_cc};

    // Scale by the same 2 mu0/dt prefactor the operator applies to the
    // mass-matrix product.
    const amrex::Real fac = 2.0_rt * PhysConst::mu0 / m_dt;

    for (int d = 0; d < 3; ++d) {
        const int nc = Sdiag[d]->nComp();

#ifdef AMREX_USE_OMP
#pragma omp parallel if (amrex::Gpu::notInLaunchRegion())
#endif
        for (amrex::MFIter mfi(*chi_cc[d], amrex::TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            const amrex::Box& tbx = mfi.tilebox();
            amrex::Array4<amrex::Real> const& chi = chi_cc[d]->array(mfi);
            amrex::Array4<const amrex::Real> const& S = Sdiag[d]->const_array(mfi);
            amrex::ParallelFor(tbx, [=] AMREX_GPU_DEVICE (int i, int j, int k)
            {
                // Row-sum this block's stencil-band components at (i,j,k),
                // then collocate onto the cell center by index identification
                // (same convention DarwinMLMGPC uses for Z): a plain (i,j,k)
                // fetch already reads the point at S's own native staggering
                // that shares (i,j,k) with the target cell-centered array.
                // This keeps chi_dd collocated with where curl(Z)_d itself
                // would land under the same convention, and avoids zeroing the
                // nodal-dimension Nyquist mode the way pair-averaging would.
                amrex::Real s = 0.0;
                for (int c = 0; c < nc; ++c) {
                    s += S(i,j,k,c);
                }
                chi(i,j,k) = fac*s;
            });
        }
    }
}

void SemiImplicitDarwin::ClearParticleVelocities ()
{
    m_services.clear_particle_velocities();
}

void SemiImplicitDarwin::FinishVelocityUpdate ()
{
    m_services.finish_velocity_update();
}

void SemiImplicitDarwin::PrepareVelocitiesForCurrentDeposition ()
{
    m_services.prepare_velocities_for_deposition();
}
