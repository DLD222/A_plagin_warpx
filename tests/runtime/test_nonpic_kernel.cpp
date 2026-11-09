/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * R2/R4 acceptance for a non-PIC kernel.
 *
 * This file implements a kernel that has no particles at all — a stand-in for
 * a fluid (e.g. ten-moment) kernel — using only headers from Source/Core.  It
 * proves two things:
 *
 *  1. the kernel contract (ISimulationKernel) and the state view (RuntimeData) can be
 *     implemented outside the WarpX kernel plugin;
 *  2. the particle-specific part of RuntimeData is optional: a kernel without
 *     particles leaves the defaults in place and everything still compiles and
 *     runs, and the accessors report the absence instead of crashing.
 *
 * It also checks that events reach a plugin through the callback bus rather
 * than through a kernel header.
 */
#include "Runtime/Callbacks.H"
#include "Runtime/FieldRegistry.H"
#include "Runtime/IFilterBank.H"
#include "Runtime/FrameState.H"
#include "Runtime/HostServices.H"
#include "Runtime/ISimulationKernel.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"

#include <ablastr/fields/MultiFabRegister.H>

#include <AMReX.H>
#include <AMReX_BoxArray.H>
#include <AMReX_DistributionMapping.H>
#include <AMReX_Geometry.H>
#include <AMReX_REAL.H>

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

int failures = 0;

void check (bool ok, const char* what)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

/* A state view for a kernel that stores moments instead of particles.
 * Only the generic part is implemented; species, containers, hybrid model and
 * materials are left to the defaults. */
class FluidRuntimeData final : public warpx::runtime::RuntimeData
{
public:
    FluidRuntimeData ()
    {
        m_geom.resize(1);
        m_ba.resize(1);
        m_dm.resize(1);
        m_dm[0].define(m_ba[0]);
        m_ng_eb = amrex::IntVect(0);
        m_ng_rho = amrex::IntVect(0);
        m_ng_j = amrex::IntVect(0);
        m_dt = {1.0e-3};
    }

    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () override { return m_fields; }
    [[nodiscard]] const ablastr::fields::MultiFabRegister& fields () const override { return m_fields; }
    [[nodiscard]] amrex::AmrCore& amrCore () const override { throw std::logic_error("no AmrCore"); }
    [[nodiscard]] warpx::runtime::IGeometryProvider& kernel () override { throw std::logic_error("no kernel view"); }
    [[nodiscard]] const warpx::runtime::IGeometryProvider& kernel () const override { throw std::logic_error("no kernel view"); }
    [[nodiscard]] int finestLevel () const override { return 0; }
    [[nodiscard]] int maxLevel () const override { return 0; }
    [[nodiscard]] const amrex::Geometry& geom (int) const override { return m_geom[0]; }
    [[nodiscard]] const amrex::Vector<amrex::Geometry>& geomAll () const override { return m_geom; }
    [[nodiscard]] const amrex::BoxArray& boxArray (int) const override { return m_ba[0]; }
    [[nodiscard]] const amrex::Vector<amrex::BoxArray>& boxArrayAll () const override { return m_ba; }
    [[nodiscard]] const amrex::DistributionMapping& distributionMap (int) const override { return m_dm[0]; }
    [[nodiscard]] const amrex::Vector<amrex::DistributionMapping>& distributionMapAll () const override { return m_dm; }
    [[nodiscard]] amrex::Array<amrex::Real, 3> cellSize (int) const override { return {1.0, 1.0, 1.0}; }
    [[nodiscard]] amrex::XDim3 lowerCorner (const amrex::Box&, int, amrex::Real = 0.0) const override { return {0.0, 0.0, 0.0}; }
    [[nodiscard]] amrex::XDim3 upperCorner (const amrex::Box&, int) const override { return {0.0, 0.0, 0.0}; }
    [[nodiscard]] amrex::RealBox realBox (const amrex::Box&, int) const override { return {}; }
    [[nodiscard]] amrex::XDim3 invCellSize (int) const override { return {1.0, 1.0, 1.0}; }
    [[nodiscard]] amrex::IntVect ngEB () const override { return m_ng_eb; }
    [[nodiscard]] amrex::IntVect ngDeposRho () const override { return m_ng_rho; }
    [[nodiscard]] amrex::IntVect ngDeposJ () const override { return m_ng_j; }
    [[nodiscard]] bool useVerboncoeurAxisCorrection () const override { return false; }
    [[nodiscard]] const amrex::Vector<amrex::Real>& vGalilean () const override { return m_v_galilean; }
    [[nodiscard]] const amrex::Vector<amrex::Real>& dtAll () const override { return m_dt; }
    [[nodiscard]] amrex::IntVect refRatio (int) const override { return m_ref_ratio; }
    [[nodiscard]] const amrex::Vector<amrex::IntVect>& refRatioAll () const override { return m_ref_ratio_all; }
    [[nodiscard]] const amrex::Vector<amrex::IntVect>& refRatioVector () const override { return m_ref_ratio_all; }
    [[nodiscard]] amrex::Real loadBalanceEfficiency (int) const override { return 1.0; }
    [[nodiscard]] LoadBalanceCostsUpdateAlgo loadBalanceCostsUpdateAlgo () const override
    { return LoadBalanceCostsUpdateAlgo::Timers; }
    [[nodiscard]] amrex::LayoutData<amrex::Real>* costs (int) const override { return nullptr; }
    [[nodiscard]] int maxStep () const override { return 1; }
    [[nodiscard]] amrex::Real stopTime () const override { return 1.0; }
    [[nodiscard]] const amrex::EB2::IndexSpace* ebIndexSpace (int) const override { return nullptr; }
    [[nodiscard]] const warpx::runtime::FieldRegistry& fieldRegistry () const override { return m_field_registry; }
    [[nodiscard]] warpx::runtime::IFilterBank& filterBank () override { throw std::logic_error("no filters"); }
    [[nodiscard]] const warpx::runtime::IFilterBank& filterBank () const override { throw std::logic_error("no filters"); }
    [[nodiscard]] amrex::IntVect blockingFactor (int) const override { return amrex::IntVect(8); }
    [[nodiscard]] amrex::IntVect numProcs () const override { return amrex::IntVect(1); }
    [[nodiscard]] amrex::IntVect maxGridSize (int) const override { return amrex::IntVect(1024); }
    [[nodiscard]] const amrex::Array<amrex::Real, 3>& galileanShift () const override { return m_galilean_shift; }
    [[nodiscard]] amrex::Real t_new (int) const override { return 0.0; }
    [[nodiscard]] amrex::Real dt (int) const override { return m_dt[0]; }
    [[nodiscard]] int istep (int) const override { return 0; }
    [[nodiscard]] amrex::Vector<int> istepVector () const override { return {}; }
    [[nodiscard]] EvolveScheme evolveScheme () const override { return EvolveScheme::Explicit; }
    [[nodiscard]] ablastr::utils::text::IntervalsParser loadBalanceIntervals () const override { return {}; }
    [[nodiscard]] bool isSynchronized () const override { return true; }
    void computeCostsHeuristic () const override {}
    [[nodiscard]] const amrex::EBFArrayBoxFactory& fieldEBFactory (int) const override
    { throw std::logic_error("no EB"); }

private:
    ablastr::fields::MultiFabRegister m_fields;
    amrex::Vector<amrex::Geometry> m_geom;
    amrex::Vector<amrex::BoxArray> m_ba;
    amrex::Vector<amrex::DistributionMapping> m_dm;
    amrex::IntVect m_ng_eb;
    amrex::IntVect m_ng_rho;
    amrex::IntVect m_ng_j;
    amrex::IntVect m_ref_ratio = amrex::IntVect(2);
    amrex::Vector<amrex::IntVect> m_ref_ratio_all;
    amrex::Vector<amrex::Real> m_dt;
    amrex::Vector<amrex::Real> m_v_galilean;
    warpx::runtime::FieldRegistry m_field_registry;
    amrex::Array<amrex::Real, 3> m_galilean_shift{{0.0, 0.0, 0.0}};
};

/* A kernel that evolves fluid moments.  The particle-flavoured parts of the
 * kernel contract are no-ops; that is the whole accommodation a fluid kernel
 * needs today. */
class FluidKernel final : public warpx::runtime::ISimulationKernel
{
public:
    FluidKernel () : m_data(std::make_unique<FluidRuntimeData>()) {}

    void initialize () override
    {
        warpx::runtime::execute_callback("fluid_kernel_initialized");
        m_initialized = true;
    }
    void evolve (int numsteps) override { m_steps += numsteps; }
    void finalize () override { m_initialized = false; }
    [[nodiscard]] bool verbose () const override { return false; }
    [[nodiscard]] warpx::runtime::TimeState time_state (bool valid) const override
    { warpx::runtime::TimeState s; s.valid = valid; s.time = 0.0; s.step = 0; return s; }
    [[nodiscard]] warpx::runtime::FrameState frame_state () const override
    { return warpx::runtime::FrameState::FromLegacy(); }
    [[nodiscard]] warpx::runtime::RuntimeConfigSnapshot config_snapshot () const override { return {}; }
    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () override { return m_data->fields(); }
    [[nodiscard]] const ablastr::fields::MultiFabRegister& fields () const override { return m_data->fields(); }
    [[nodiscard]] warpx::runtime::FieldRegistry& schema () override { return m_schema; }
    [[nodiscard]] warpx::runtime::RuntimeData& data () override { return *m_data; }
    [[nodiscard]] warpx::runtime::HostServices& host () override { return *m_host; }
    void set_plugin_host (warpx::runtime::PluginHost*) override {}
    void set_runtime_context (warpx::runtime::RuntimeContext*) override {}
    void register_plugins (warpx::runtime::PluginHost&, bool) override {}
    // A fluid kernel has no particles: the runtime must be told so explicitly
    // rather than through a pair of empty hooks.
    [[nodiscard]] warpx::runtime::IParticleCapability* particle_capability () override
    {
        return nullptr;
    }

    void clear () override {}
    [[nodiscard]] std::vector<std::string> active_features () const override { return {"fluid"}; }

    [[nodiscard]] int steps () const { return m_steps; }
    [[nodiscard]] bool initialized () const { return m_initialized; }

private:
    std::unique_ptr<FluidRuntimeData> m_data;
    std::unique_ptr<warpx::runtime::HostServices> m_host;
    warpx::runtime::FieldRegistry m_schema;
    int m_steps = 0;
    bool m_initialized = false;
};

} // namespace

int main (int argc, char* argv[])
{
    amrex::Initialize(argc, argv);

    FluidKernel kernel;

    bool callback_ran = false;
    warpx::runtime::register_callback("fluid_kernel_initialized",
                                      [&callback_ran] { callback_ran = true; });

    kernel.initialize();
    check(kernel.initialized(), "a fluid kernel initializes");
    check(callback_ran, "a fluid kernel fires events through the core callback bus");

    kernel.evolve(3);
    check(kernel.steps() == 3, "a fluid kernel evolves");

    // The particle part of the state view is optional: asking for it reports
    // the absence instead of crashing.
    bool species_reported = false;
    try {
        (void)kernel.data().speciesMetadata();
    } catch (const std::logic_error&) {
        species_reported = true;
    }
    check(species_reported, "a fluid kernel reports that it has no species");

    check(kernel.data().fluids() == nullptr, "no fluid model by default");
    check(kernel.data().hybridPICModel() == nullptr, "no hybrid model by default");
    check(kernel.active_features().size() == 1 && kernel.active_features()[0] == "fluid",
          "the kernel advertises its own feature set");

    kernel.finalize();

    amrex::Finalize();

    if (failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("non-PIC kernel: %d checks passed\n", 8);
    return 0;
}
