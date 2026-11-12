/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "ImplicitKernelServicesBuilder.H"

#include "WarpX.H"

#include "Particles/MultiParticleContainer.H"
#include "Python/callbacks.H"

#include "Runtime/ImplicitKernelServices.H"

warpx::runtime::ImplicitKernelServices
BuildImplicitKernelServices (WarpX& warpx)
{
    warpx::runtime::ImplicitKernelServices s;
    s.fields = &warpx.m_fields;
    s.geom = [&warpx] (int lev) -> const amrex::Geometry& { return warpx.Geom(lev); };
    s.ng_eb = [&warpx] { return warpx.getngEB(); };
    s.pec_insulator_is_e = [&warpx] (int dir, int side) {
        return warpx.GetPECInsulator_IsESet(dir, side);
    };
    s.field_boundary_lo = [&warpx] () -> amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> const& {
        return warpx.GetFieldBoundaryLo();
    };
    s.field_boundary_hi = [&warpx] () -> amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> const& {
        return warpx.GetFieldBoundaryHi();
    };
    s.verbose = [&warpx] { return warpx.Verbose(); };
    s.finest_level = [&warpx] { return warpx.finestLevel(); };
    s.field_dot_mask = [&warpx] (warpx::fields::FieldType type, int lev, int dir) {
        return warpx.getFieldDotMaskPointer(type, lev, ablastr::fields::Direction{dir});
    };
    s.apply_filter_mf = [&warpx] (ablastr::fields::MultiLevelVectorField const& field, int lev) {
        warpx.ApplyFilterMF(field, lev);
    };
    s.eb_update_e_flag = [&warpx] () -> const auto& { return warpx.GetEBUpdateEFlag_fp(); };
    s.eb_update_b_flag = [&warpx] () -> const auto& { return warpx.GetEBUpdateBFlag_fp(); };
    s.fdtd_solver = [&warpx] (int lev) { return warpx.get_pointer_fdtd_solver_fp(lev); };
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
    s.apply_inverse_volume_scaling_current =
        [&warpx] (ablastr::fields::MultiFabRegister& fields, int lev) {
            warpx.ApplyInverseVolumeScalingToCurrentDensity(
                fields.get(warpx::fields::FieldType::current_fp, ablastr::fields::Direction{0}, lev),
                fields.get(warpx::fields::FieldType::current_fp, ablastr::fields::Direction{1}, lev),
                fields.get(warpx::fields::FieldType::current_fp, ablastr::fields::Direction{2}, lev),
                lev);
        };
    s.apply_inverse_volume_scaling_mass_matrices_pc =
        [&warpx] (ablastr::fields::MultiFabRegister& fields, int lev) {
            warpx.ApplyInverseVolumeScalingToMassMatricesPC(
                fields.get(warpx::fields::FieldType::MassMatrices_PC, ablastr::fields::Direction{0}, lev),
                fields.get(warpx::fields::FieldType::MassMatrices_PC, ablastr::fields::Direction{1}, lev),
                fields.get(warpx::fields::FieldType::MassMatrices_PC, ablastr::fields::Direction{2}, lev),
                lev);
        };

#endif
    s.evolve_b = [&warpx] (amrex::Real dt, SubcyclingHalf half, amrex::Real start_time) {
        warpx.EvolveB(dt, half, start_time);
    };
    s.fill_boundary_b = [&warpx] (amrex::IntVect const& ng, bool nodal_sync) {
        warpx.FillBoundaryB(ng, nodal_sync);
    };
    s.fill_boundary_e = [&warpx] (amrex::IntVect const& ng, bool nodal_sync) {
        warpx.FillBoundaryE(ng, nodal_sync);
    };
    s.apply_efield_boundary = [&warpx] (int lev, ablastr::utils::enums::PatchType patch_type, amrex::Real t) {
        warpx.ApplyEfieldBoundary(lev, patch_type, t);
    };
    s.apply_jfield_boundary = [&warpx] (int lev, amrex::MultiFab* jx, amrex::MultiFab* jy,
                                        amrex::MultiFab* jz,
                                        ablastr::utils::enums::PatchType patch_type) {
        warpx.ApplyJfieldBoundary(lev, jx, jy, jz, patch_type);
    };
    s.finalize_j = [&warpx] { warpx.FinalizeJ(); };
    s.finalize_rho = [&warpx] { warpx.FinalizeRho(); };
    s.sync_current = [&warpx] (char const* which) { warpx.SyncCurrent(which); };
    s.sync_current_and_rho = [&warpx] { warpx.SyncCurrentAndRho(); };
    s.sync_mass_matrices = [&warpx] { warpx.SyncMassMatrices(); };
    s.sync_mass_matrices_pc = [&warpx] { warpx.SyncMassMatricesPC(); };
    s.deposit_mass_matrices = [&warpx] (amrex::Real dt,
                                        ImplicitOptions const& options) {
        warpx.DepositMassMatrices(dt, options);
    };
    s.implicit_compute_rhse = [&warpx] (amrex::Real dt, WarpXSolverVec& rhse) {
        warpx.ImplicitComputeRHSE(dt, rhse);
    };
    s.push_particles_and_deposit =
        [&warpx] (amrex::Real cur_time, bool skip_deposition,
                  PositionPushType position_push, MomentumPushType momentum_push,
                  amrex::Real dt_scale, ImplicitOptions const* options) {
            warpx.PushParticlesandDeposit(cur_time, skip_deposition, position_push,
                                          momentum_push, dt_scale, options);
        };
    s.save_particles_at_implicit_step_start = [&warpx] (amrex::Real time) {
        warpx.SaveParticlesAtImplicitStepStart();
        amrex::ignore_unused(time);
    };
    s.reset_implicit_particle_data = [&warpx] { warpx.ResetImplicitParticleData(); };
    s.finish_implicit_particle_update = [&warpx] (amrex::Real time) {
        warpx.FinishImplicitParticleUpdate(time);
    };
    s.handle_particles_at_boundaries = [&warpx] (int step, amrex::Real cur_time, int num_moved) {
        warpx.HandleParticlesAtBoundaries(step, cur_time, num_moved);
    };
    s.set_electric_field_and_apply_bcs = [&warpx] (WarpXSolverVec const& e, amrex::Real time) {
        warpx.SetElectricFieldAndApplyBCs(e, time);
    };
    s.finish_electric_field_and_apply_bcs = [&warpx] (amrex::Real theta, amrex::Real time) {
        warpx.FinishElectricFieldAndApplyBCs(theta, time);
    };
    s.finish_magnetic_field_and_apply_bcs = [&warpx] (amrex::Real theta, amrex::Real time) {
        warpx.FinishMagneticFieldAndApplyBCs(theta, time);
    };
    s.update_magnetic_field_and_apply_bcs =
        [&warpx] (ablastr::fields::MultiLevelVectorField const& Bn,
                  amrex::Real theta_dt, amrex::Real start_time) {
            warpx.UpdateMagneticFieldAndApplyBCs(Bn, theta_dt, start_time);
        };
    s.spectral_source_free_field_advance = [&warpx] (amrex::Real start_time) {
        warpx.SpectralSourceFreeFieldAdvance(start_time);
    };
    s.compute_diags_mid_step = [&warpx] (int step, amrex::Real dt) {
        if (warpx.reduced_diags) { warpx.reduced_diags->ComputeDiagsMidStep(step, dt); }
    };
    s.push_particles_momentum = [&warpx] (int lev, amrex::Real dt,
                                          amrex::MultiFab const& Ex, amrex::MultiFab const& Ey,
                                          amrex::MultiFab const& Ez, amrex::MultiFab const& Bx,
                                          amrex::MultiFab const& By, amrex::MultiFab const& Bz,
                                          MomentumPushType push_type,
                                          std::optional<warpx::particles::FieldGatherOrders> gather) {
        warpx.GetPartContainer().PushP(lev, dt, Ex, Ey, Ez, Bx, By, Bz, push_type, gather);
    };
    s.deposit_current_all_species =
        [&warpx] (ablastr::fields::MultiLevelVectorField const& J, amrex::Real dt,
                  amrex::Real time_dilation, PushType push_type, int particle_shape) {
            warpx.GetPartContainer().DepositCurrent(J, dt, time_dilation, push_type, particle_shape);
        };
    s.push_particles_position = [&warpx] (amrex::Real dt) {
        warpx.GetPartContainer().PushX(dt);
    };
    s.run_python_callback = [&warpx] (char const* name) {
        amrex::ignore_unused(warpx);
        ExecutePythonCallback(name);
    };
    s.clear_particle_velocities = [&warpx] { warpx.ClearImplicitParticleVelocities(); };
    s.finish_velocity_update = [&warpx] { warpx.FinishImplicitVelocityUpdate(); };
    s.prepare_velocities_for_deposition = [&warpx] {
        warpx.PrepareImplicitVelocitiesForDeposition();
    };
    s.add_implicit_particle_attributes = [&warpx] (bool particle_suborbits) {
        warpx.AddImplicitParticleAttributes(particle_suborbits);
    };
    s.num_species = [&warpx] { return warpx.GetPartContainer().nSpecies(); };
    s.species_name = [&warpx] (int ispecies) {
        return warpx.GetPartContainer().GetParticleContainer(ispecies).getName();
    };
    s.species_has_attrib = [&warpx] (int ispecies, std::string const& attrib) {
        auto& pc = dynamic_cast<WarpXParticleContainer&>(
            warpx.GetPartContainer().GetParticleContainer(ispecies));
        return pc.HasiAttrib(attrib);
    };
    s.count_suborbits = [&warpx] (int ispecies, int threshold, amrex::Long* counter) {
        auto& pc = dynamic_cast<WarpXParticleContainer&>(
            warpx.GetPartContainer().GetParticleContainer(ispecies));
        for (int lev = 0; lev <= warpx.finestLevel(); ++lev) {
#ifdef AMREX_USE_OMP
#pragma omp parallel
#endif
            {
                for (ParticleTileIter pti(pc, lev); pti.isValid(); ++pti) {
                    int const* const nsuborbits = pti.GetiAttribs("nsuborbits").dataPtr();
                    long const np = pti.numParticles();
                    amrex::ParallelFor(np, [=] AMREX_GPU_DEVICE (long ip) {
                        if (nsuborbits[ip] >= threshold) {
                            amrex::Gpu::Atomic::Add(counter, amrex::Long(1));
                        }
                    });
                }
            }
        }
    };
    return s;
}
