/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Runtime/DataBus.H"
#include "Runtime/Runtime.H"
#include "Runtime/CollisionPackage.H"
#include "Runtime/HandlerSpec.H"
#include "Runtime/FieldRegistry.H"
#include "Runtime/PicEmPackage.H"
#include "Runtime/PicFieldSolveHandler.H"
#include "Runtime/PicFinalizeHandler.H"
#include "Runtime/PicPushHandler.H"
#include "Runtime/StagePoint.H"
#include "Utils/WarpXConst.H"
#include "WarpX.H"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

int failures = 0;

void check (bool ok, const std::string& what)
{
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

template <typename T>
void check_eq (const T& a, const T& b, const std::string& what)
{
    check(a == b, what);
}

void check_real (amrex::Real a, amrex::Real b, const std::string& what)
{
    check(std::abs(a - b) <= 1.0e-15 * std::max<amrex::Real>(1.0, std::abs(b)), what);
}

} // namespace

int main (int argc, char* argv[])
{
    const bool particles_mode = (argc > 2 && std::string(argv[2]) == "--particles");
    const bool psatd_mode = (argc > 2 && std::string(argv[2]) == "--psatd");
    char* rt_argv[3] = {argv[0], argv[1], nullptr};
    warpx::runtime::Runtime runtime(2, rt_argv);

    // The context must be installed before InitData (diagnostics are built there).
    auto* installed = WarpX::GetInstance().GetRuntimeContext();
    check(installed != nullptr, "WarpX has a runtime context before InitData");
    check(installed == &runtime.context(), "installed context is Runtime::context()");
    check(runtime.context().config == &runtime.config(), "context.config points at RuntimeConfig");
    check(runtime.context().time == &runtime.time(), "context.time points at TimeState");
    check(runtime.context().frame == &runtime.frame(), "context.frame points at FrameState");
    check(&runtime.fields() == &WarpX::GetInstance().GetMultiFabRegister(),
          "Runtime::fields() is the authoritative MultiFabRegister (no DataBus copy)");
    check(&runtime.bus().fields() == &runtime.fields(), "DataBus forwards to the same Register");
    check(runtime.context().bus == &runtime.bus(), "context.bus points at Runtime::bus()");
    check(WarpX::GetInstance().levelResources().fields() == &runtime.fields(),
          "LevelResources borrows the same field registry");

    {
        struct FakeHandler final : warpx::runtime::IHandler {
            warpx::runtime::HandlerSpec s;
            explicit FakeHandler (warpx::runtime::HandlerSpec spec) : s(std::move(spec)) {}
            [[nodiscard]] warpx::runtime::HandlerSpec spec () const override { return s; }
            void apply (warpx::runtime::StageContext&) override {}
        };

        warpx::runtime::PluginHost host;
        host.add(std::make_shared<FakeHandler>(warpx::runtime::HandlerSpec{
            "diag", {warpx::runtime::Stage::Diagnostics, 0}, {}, {}, {}, {}, 0}));
        host.add(std::make_shared<FakeHandler>(warpx::runtime::HandlerSpec{
            "push", {warpx::runtime::Stage::Push, 0}, {}, {}, {}, {}, 0}));
        const auto ordered = host.ordered();
        check_eq(ordered.size(), std::size_t(2), "PluginHost stores handlers");
        check_eq(ordered.front()->spec().name, std::string("push"),
                 "PluginHost orders Push before Diagnostics");
        check_eq(host.specs().size(), std::size_t(2),
                 "PluginHost records specs on add");

        warpx::runtime::PluginHost conflict;
        conflict.add(std::make_shared<FakeHandler>(warpx::runtime::HandlerSpec{
            "writer_a", {warpx::runtime::Stage::FieldSolve, 0}, {}, {"Efield_fp"}, {}, {}, 0}));
        bool threw = false;
        try {
            conflict.add(std::make_shared<FakeHandler>(warpx::runtime::HandlerSpec{
                "writer_b", {warpx::runtime::Stage::FieldSolve, 0}, {}, {"Efield_fp"}, {}, {}, 1}));
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "PluginHost rejects two writers of the same field at one StagePoint");
    }

    {
        warpx::runtime::PluginHost host;
        warpx::runtime::PicEmPackage::register_specs(host);
        warpx::runtime::PicPushHandler push_handler;
        check_eq(push_handler.spec().name, std::string("pic_push"),
                 "PicPushHandler declares pic_push");
        warpx::runtime::PicFinalizeHandler finalize_handler;
        check_eq(finalize_handler.spec().name, std::string("pic_finalize_deposition"),
                 "PicFinalizeHandler declares pic_finalize_deposition");
        warpx::runtime::PicFieldSolveHandler field_handler;
        check_eq(field_handler.spec().name, std::string("pic_field_solve"),
                 "PicFieldSolveHandler declares pic_field_solve");

        warpx::runtime::CollisionPackage::register_specs(host);
        const auto specs = host.ordered_specs();
        check_eq(specs.size(), std::size_t(6), "PIC + collision packages declare six specs");
        check_eq(specs.front().name, std::string("pic_push"),
                 "PicEmPackage Push comes first");
        bool has_binary = false;
        for (const auto& spec : specs) {
            has_binary |= (spec.name == "binary_collision" &&
                           spec.point.stage == warpx::runtime::Stage::Collisions);
        }
        check(has_binary, "CollisionPackage declares binary collision at Collisions");
    }

    {
        warpx::runtime::StagePlan plan;
        plan.add({warpx::runtime::Stage::Diagnostics, 0}, "diag");
        plan.add({warpx::runtime::Stage::Init, 1}, "init1");
        plan.add({warpx::runtime::Stage::Init, 0}, "init0");
        const auto ordered = plan.sorted();
        check_eq(ordered.front().second, std::string("init0"), "StagePlan sorts Init/0 first");
        check_eq(ordered.back().second, std::string("diag"), "StagePlan sorts Diagnostics last");
    }

    runtime.initialize();
    check(WarpX::GetInstance().levelResources().size() ==
              WarpX::GetInstance().maxLevel() + 1,
          "LevelResources tracks the AMR hierarchy");
    check(WarpX::GetInstance().fieldSchema().contains("Efield_fp_x"),
          "FieldSchema declares base E field");
    check(WarpX::GetInstance().fieldSchema().get("Efield_fp_x").ncomp ==
              WarpX::GetInstance().ncomps,
          "FieldSchema ncomp matches register");
    check(WarpX::GetInstance().fieldSchema().contains("current_fp_z"),
          "FieldSchema declares base J field");
    check_eq(runtime.bus().registry().size(),
             WarpX::GetInstance().fieldSchema().size(),
             "DataBus and FieldSchema share one FieldRegistry");

    {
        warpx::runtime::FieldRegistry registry;
        warpx::runtime::FieldSpec spec;
        spec.name = "probe";
        spec.checkpoint = false;
        registry.declare(spec);
        check(registry.contains("probe"), "FieldRegistry stores schema by name");
        spec.checkpoint = true;
        bool threw = false;
        try {
            registry.declare(spec);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        check(threw, "FieldRegistry rejects conflicting checkpoint metadata");
    }

    if (particles_mode) {
        check(runtime.bus().has_species("electrons"), "DataBus registers electrons");
        const auto e = runtime.bus().species("electrons");
        check_eq(e.name(), std::string("electrons"), "SpeciesHandle keeps name");
        check(e.numberOfParticles() > 0, "SpeciesHandle reports particles");
        check_real(e.charge(), -PhysConst::q_e, "SpeciesHandle charge");
        check_real(e.mass(), PhysConst::m_e, "SpeciesHandle mass");
    }
    check(WarpX::GetInstance().GetRuntimeContext() == installed, "context survives InitData");

    // RuntimeConfig must be a live view, not a value snapshot (audit D4).
    {
        const int nox_before = WarpX::nox;
        WarpX::nox = nox_before + 1;
        check_eq(runtime.config().nox(), nox_before + 1, "RuntimeConfig is live (nox)");
        WarpX::nox = nox_before;
    }

    const auto& c = runtime.config();
    check_eq(c.current_deposition_algo(), WarpX::current_deposition_algo, "current_deposition_algo");
    check_eq(c.charge_deposition_algo(), WarpX::charge_deposition_algo, "charge_deposition_algo");
    check_eq(c.field_gathering_algo(), WarpX::field_gathering_algo, "field_gathering_algo");
    check_eq(c.particle_pusher_algo(), WarpX::particle_pusher_algo, "particle_pusher_algo");
    check_eq(c.electromagnetic_solver_id(), WarpX::electromagnetic_solver_id, "electromagnetic_solver_id");
    check_eq(c.electrostatic_solver_id(), WarpX::electrostatic_solver_id, "electrostatic_solver_id");
    check_eq(c.poisson_solver_id(), WarpX::poisson_solver_id, "poisson_solver_id");
    check_eq(c.grid_type(), WarpX::grid_type, "grid_type");
    check_eq(c.ect_conformal_E(), WarpX::ect_conformal_E, "ect_conformal_E");
    check_real(c.ect_conformal_E_Astab(), WarpX::ect_conformal_E_Astab, "ect_conformal_E_Astab");
    check_eq(c.macroscopic_subpixel(), WarpX::macroscopic_subpixel, "macroscopic_subpixel");
    check_eq(c.ect_te_borrowing(), WarpX::ect_te_borrowing, "ect_te_borrowing");
    check_eq(c.field_boundary_lo(), WarpX::field_boundary_lo, "field_boundary_lo");
    check_eq(c.field_boundary_hi(), WarpX::field_boundary_hi, "field_boundary_hi");
    check_eq(c.particle_boundary_lo(), WarpX::particle_boundary_lo, "particle_boundary_lo");
    check_eq(c.particle_boundary_hi(), WarpX::particle_boundary_hi, "particle_boundary_hi");
    check_eq(c.eb_particle_boundary(), WarpX::eb_particle_boundary, "eb_particle_boundary");
    check_eq(c.time_dependency_J(), WarpX::time_dependency_J, "time_dependency_J");
    check_eq(c.time_dependency_rho(), WarpX::time_dependency_rho, "time_dependency_rho");
    check_eq(c.do_single_precision_comms(), WarpX::do_single_precision_comms, "do_single_precision_comms");
    check_eq(c.do_shared_mem_charge_deposition(), WarpX::do_shared_mem_charge_deposition, "do_shared_mem_charge_deposition");
    check_eq(c.do_shared_mem_current_deposition(), WarpX::do_shared_mem_current_deposition, "do_shared_mem_current_deposition");
    check_eq(c.do_dive_cleaning(), WarpX::do_dive_cleaning, "do_dive_cleaning");
    check_eq(c.do_divb_cleaning(), WarpX::do_divb_cleaning, "do_divb_cleaning");
    check_eq(c.use_fdtd_nci_corr(), WarpX::use_fdtd_nci_corr, "use_fdtd_nci_corr");
    check_eq(c.galerkin_interpolation(), WarpX::galerkin_interpolation, "galerkin_interpolation");
    check_eq(c.use_filter(), WarpX::use_filter, "use_filter");
    check_eq(c.use_kspace_filter(), WarpX::use_kspace_filter, "use_kspace_filter");
    check_eq(c.use_filter_compensation(), WarpX::use_filter_compensation, "use_filter_compensation");
    check_eq(c.serialize_initial_conditions(), WarpX::serialize_initial_conditions, "serialize_initial_conditions");
    check_eq(c.compute_max_step_from_btd(), WarpX::compute_max_step_from_btd, "compute_max_step_from_btd");
    check_eq(c.do_dynamic_scheduling(), WarpX::do_dynamic_scheduling, "do_dynamic_scheduling");
    check_eq(c.refine_plasma(), WarpX::refine_plasma, "refine_plasma");
    check_eq(c.filter_npass_each_dir(), WarpX::filter_npass_each_dir, "filter_npass_each_dir");
    check_eq(c.load_balance_costs_update_algo(), WarpX::load_balance_costs_update_algo, "load_balance_costs_update_algo");
    check_eq(c.start_moving_window_step(), WarpX::start_moving_window_step, "start_moving_window_step");
    check_eq(c.end_moving_window_step(), WarpX::end_moving_window_step, "end_moving_window_step");

    const auto& t = runtime.time();
    check_eq(t.step, WarpX::GetInstance().getistep(0), "time.step");
    check_eq(t.finest_level, WarpX::GetInstance().finestLevel(), "time.finest_level");
    check_eq(t.istep, WarpX::GetInstance().getistep(), "time.istep");
    check_eq(t.nsubsteps, WarpX::GetInstance().getnsubsteps(), "time.nsubsteps");
    check_eq(t.dt, WarpX::GetInstance().getdt(), "time.dt");
    check_eq(t.t_new, WarpX::GetInstance().gett_new(), "time.t_new");
    check_eq(t.t_old, WarpX::GetInstance().gett_old(), "time.t_old");
    check_real(t.time, WarpX::GetInstance().gett_new(0), "time.time");

    const auto& f = runtime.frame();
    check_real(f.gamma_boost, WarpX::gamma_boost, "frame.gamma_boost");
    check_real(f.beta_boost, WarpX::beta_boost, "frame.beta_boost");
    check_eq(f.boost_direction, WarpX::boost_direction, "frame.boost_direction");
    check_eq(f.do_moving_window, WarpX::do_moving_window != 0, "frame.do_moving_window");
    check_eq(f.moving_window_dir, WarpX::moving_window_dir, "frame.moving_window_dir");
    check_real(f.moving_window_v, WarpX::moving_window_v, "frame.moving_window_v");
    check_eq(f.start_moving_window_step, WarpX::start_moving_window_step, "frame.start_moving_window_step");
    check_eq(f.end_moving_window_step, WarpX::end_moving_window_step, "frame.end_moving_window_step");

    // A0.6: DataBus only declares/forwards; alias must not copy a MultiFab.
    {
        warpx::runtime::FieldSpec spec;
        spec.name = "A0_6_probe";
        spec.ncomp = 1;
        spec.ngrow = amrex::IntVect{AMREX_D_DECL(1, 1, 1)};
        spec.unit = "V/m";
        runtime.bus().declare(spec);
        check(runtime.bus().spec("A0_6_probe") != nullptr, "DataBus stores FieldSpec");
        check(!runtime.bus().has("A0_6_probe", 0), "declare() does not allocate");
    }
    {
        auto* owner = runtime.bus().field("Efield_fp", 0, 0);
        auto* alias = runtime.bus().alias("A0_6_alias", "Efield_fp", 0, 0);
        check(owner != nullptr, "Efield_fp exists");
        check(alias->atLocalIdx(0).dataPtr(0) == owner->atLocalIdx(0).dataPtr(0), "alias_init shares the same data (no copy)");
    }

    runtime.evolve(1);
    check_eq(runtime.time().step, 1, "time.step after evolve");
    check_eq(runtime.time().istep, WarpX::GetInstance().getistep(), "time.istep after evolve");

    auto last_line = [](const std::string& path) {
        std::ifstream in(path);
        std::string line, last;
        while (std::getline(in, line)) {
            if (!line.empty() && (std::isdigit(static_cast<unsigned char>(line[0])) || line[0] == '-')) {
                last = line;
            }
        }
        return last;
    };

    if (!particles_mode && !psatd_mode) {
    // Numeric verification of the migrated reduced diagnostics:
    // E = 1 V/m uniform in [-1,1]^2 -> U = 0.5*eps0*E^2*volume = 1.7708...e-11 J.
    {
        std::istringstream is(last_line("diags/reducedfiles/field_energy.txt"));
        double step = 0, time = 0, total = 0, e = 0, b = 0;
        is >> step >> time >> total >> e >> b;
        constexpr double eps0 = 8.8541878188e-12;
        constexpr double mu0 = 1.2566370612685e-6;
        const double expected_e = 0.5 * eps0 * 4.0;
        const double expected_b = 0.5 / mu0 * 4.0;
        check_eq(static_cast<int>(step), 1, "field_energy step");
        check(std::abs(total - (expected_e + expected_b)) <= 1.0e-12 * (expected_e + expected_b),
              "field_energy total");
        check(std::abs(e - expected_e) <= 1.0e-12 * expected_e, "field_energy E component");
        check(std::abs(b - expected_b) <= 1.0e-12 * expected_b, "field_energy B component");
    }
    {
        std::istringstream is(last_line("diags/reducedfiles/field_max.txt"));
        double step = 0, time = 0, ex = 0, ey = 0, ez = 0, emag = 0;
        double bx = 0, by = 0, bz = 0, bmag = 0;
        is >> step >> time >> ex >> ey >> ez >> emag >> bx >> by >> bz >> bmag;
        check_eq(static_cast<int>(step), 1, "field_max step");
        check(std::abs(ex - 1.0) <= 1.0e-14, "field_max max_Ex");
        check(std::abs(emag - 1.0) <= 1.0e-14, "field_max max_|E|");
        check(std::abs(by - 1.0) <= 1.0e-14, "field_max max_By");
        check(std::abs(bmag - 1.0) <= 1.0e-14, "field_max max_|B|");
    }
    {
        std::istringstream is(last_line("diags/reducedfiles/field_mom.txt"));
        double step = 0, time = 0, px = 0, py = 0, pz = 0;
        is >> step >> time >> px >> py >> pz;
        constexpr double eps0 = 8.8541878188e-12;
        const double expected_pz = eps0 * 4.0;
        check_eq(static_cast<int>(step), 1, "field_mom step");
        check(std::abs(px) <= 1.0e-30 && std::abs(py) <= 1.0e-30, "field_mom x/y are zero");
        check(std::abs(pz - expected_pz) <= 1.0e-12 * expected_pz, "field_mom pz = eps0*E*B*V");
    }

    {
        std::istringstream is(last_line("diags/reducedfiles/field_poynting.txt"));
        double step = 0, time = 0;
        double plox = 0, ploz = 0, phix = 0, phiz = 0;
        double elox = 0, eloz = 0, ehix = 0, ehiz = 0;
        is >> step >> time >> plox >> ploz >> phix >> phiz >> elox >> eloz >> ehix >> ehiz;
        constexpr double mu0 = 1.2566370612685e-6;
        const double expected = 2.0 / mu0;   // area 2 (z faces), E=1, B=1
        check_eq(static_cast<int>(step), 1, "field_poynting step");
        check(std::abs(plox) <= 1.0e-30 && std::abs(phix) <= 1.0e-30, "poynting x faces are zero");
        check(std::abs(ploz + expected) <= 1.0e-12 * expected, "poynting lo_z");
        check(std::abs(phiz - expected) <= 1.0e-12 * expected, "poynting hi_z");
    }

    {
        std::istringstream is(last_line("diags/reducedfiles/field_reduce.txt"));
        double step = 0, time = 0, value = 0;
        is >> step >> time >> value;
        check_eq(static_cast<int>(step), 1, "field_reduce step");
        check(std::abs(value - 1.0) <= 1.0e-14, "field_reduce Maximum(Ex)");
    }

    }

    if (psatd_mode) {
        std::istringstream is(last_line("diags/reducedfiles/field_energy.txt"));
        double step = 0, time = 0, total = 0, e = 0, b = 0;
        is >> step >> time >> total >> e >> b;
        constexpr double expected = 0.5 * 8.8541878188e-12 * 4.0;
        check_eq(static_cast<int>(step), 1, "psatd step");
        check(std::abs(total - expected) <= 1.0e-12 * expected, "psatd energy conservation");
        check(std::abs(e - expected) <= 1.0e-12 * expected, "psatd E energy");
    }

    if (particles_mode) {
        {
            std::istringstream is(last_line("diags/reducedfiles/part_num.txt"));
            double step = 0, time = 0, total = 0, electrons = 0, weight = 0;
            is >> step >> time >> total >> electrons >> weight;
            check_eq(static_cast<int>(step), 1, "part_num step");
            check(std::abs(total - 256.0) <= 1.0e-12, "part_num macroparticles");
            check(std::abs(weight - 4.0e18) <= 1.0e-12 * 4.0e18, "part_num weight");
        }
        {
            std::istringstream is(last_line("diags/reducedfiles/part_energy.txt"));
            double step = 0, time = 0, total = 0;
            is >> step >> time >> total;
            check_eq(static_cast<int>(step), 1, "part_energy step");
            check(total > 0.0, "part_energy is positive after acceleration");
        }
        {
            std::istringstream is(last_line("diags/reducedfiles/rho_max.txt"));
            double step = 0, time = 0, rmax = 0, rmin = 0, absmax = 0;
            is >> step >> time >> rmax >> rmin >> absmax;
            const double expected = 1.602176634e-19 * 1.0e18;
            check_eq(static_cast<int>(step), 1, "rho_max step");
            check(std::abs(rmax + expected) <= 1.0e-12 * expected, "rho_max max");
            check(std::abs(rmin + expected) <= 1.0e-12 * expected, "rho_max min");
            check(std::abs(absmax - expected) <= 1.0e-12 * expected, "rho_max |rho|");
        }
        {
            std::istringstream is(last_line("diags/reducedfiles/extrema.txt"));
            double step = 0, time = 0;
            double xmin = 0, xmax = 0, ymin = 0, ymax = 0, zmin = 0, zmax = 0;
            is >> step >> time >> xmin >> xmax >> ymin >> ymax >> zmin >> zmax;
            check_eq(static_cast<int>(step), 1, "extrema step");
            check(std::abs(xmin + 0.9375) <= 1.0e-9, "extrema xmin");
            check(std::abs(xmax - 0.9375) <= 1.0e-9, "extrema xmax");
            check(std::abs(zmin + 0.9375) <= 1.0e-9, "extrema zmin");
            check(std::abs(zmax - 0.9375) <= 1.0e-9, "extrema zmax");
        }
        {
            std::istringstream is(last_line("diags/reducedfiles/part_mom.txt"));
            double step = 0, time = 0, px = 0, py = 0, pz = 0;
            is >> step >> time >> px >> py >> pz;
            check_eq(static_cast<int>(step), 1, "part_mom step");
            check(std::abs(pz) > 0.0, "part_mom pz is nonzero after acceleration");
        }
    }

    {
        // field_probe is only configured on the fields-only input file;
        // particles/psatd modes do not configure it, so skip.
        if (!particles_mode && !psatd_mode) {
            std::istringstream is(last_line("diags/reducedfiles/field_probe.txt"));
            double step = 0, time = 0, x = 0, y = 0, z = 0;
            double ex = 0, ey = 0, ez = 0, bx = 0, by = 0, bz = 0, s = 0;
            is >> step >> time >> x >> y >> z >> ex >> ey >> ez >> bx >> by >> bz >> s;
            constexpr double mu0 = 1.2566370612685e-6;
            check_eq(static_cast<int>(step), 1, "field_probe step");
            check(std::abs(ex - 1.0) <= 1.0e-14, "field_probe Ex");
            check(std::abs(by - 1.0) <= 1.0e-14, "field_probe By");
            check(std::abs(s - 1.0 / mu0) <= 1.0e-12 / mu0, "field_probe Poynting S");
        }
    }

    if (failures == 0) {
        std::cout << "PASS: runtime snapshots match legacy WarpX state\n";
    }
    return failures == 0 ? 0 : 1;
}
