#include "WarpXKernel.H"

#include "Modules/PicEm/PicEmPackage.H"
#include "Particles/Collision/CollisionHandler.H"
#include "Particles/Collision/CollisionSpeciesTable.H"
#include "Particles/MultiParticleContainer.H"
#include "Runtime/CollisionPackage.H"
#include "Runtime/DataBus.H"
#include "Runtime/DiagnosticsPackage.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/RuntimeConfigSnapshot.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/WarpXHostServices.H"
#include "Runtime/WarpXRuntimeData.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include "WarpX.H"
#include "Initialization/WarpXInit.H"

#include <set>
#include <sstream>
#include <string>

namespace warpx::runtime
{
namespace
{
std::string field_solver_module_name (const RuntimeConfig& config)
{
    if (config.electrostatic_solver_id() != ElectrostaticSolverAlgo::None) {
        return "field_solve.electrostatic";
    }
    switch (config.electromagnetic_solver_id()) {
        case ElectromagneticSolverAlgo::Yee: return "field_solve.yee";
        case ElectromagneticSolverAlgo::CKC: return "field_solve.ckc";
        case ElectromagneticSolverAlgo::PSATD: return "field_solve.psatd";
        case ElectromagneticSolverAlgo::ECT: return "field_solve.ect";
        case ElectromagneticSolverAlgo::HybridPIC: return "field_solve.hybrid_pic";
        case ElectromagneticSolverAlgo::None: return "field_solve.none";
        default: return "field_solve.unknown";
    }
}
TimeState make_time (const WarpX& warpx, bool valid)
{
    TimeState t;
    t.step = warpx.getistep(0);
    t.finest_level = warpx.finestLevel();
    t.time = warpx.gett_new(0);
    t.istep = warpx.getistep();
    t.nsubsteps = warpx.getnsubsteps();
    t.dt = warpx.getdt();
    t.t_new = warpx.gett_new();
    t.t_old = warpx.gett_old();
    t.valid = valid;
    return t;
}
}

WarpXKernel::WarpXKernel (int, char*[])
{
    m_warpx = WarpX::Create();
    auto& warpx = *m_warpx;
    m_data = std::make_unique<WarpXRuntimeData>(warpx);
    m_host = std::make_unique<WarpXHostServices>(warpx);

    auto& mpc = warpx.GetPartContainer();
    auto* host = m_host.get();
    CollisionSpeciesTable species(mpc, [host](const std::string& name) {
        host->particleServices().calculateNuei(name);
    });
    m_config = RuntimeConfig::FromLegacy();
    m_collision = std::make_unique<CollisionHandler>(
        species, *m_data, m_config, host);
    warpx.SetCollisionHandler(m_collision.get());
}

WarpXKernel::~WarpXKernel () { clear(); }

void WarpXKernel::initialize () { m_warpx->InitData(); }
void WarpXKernel::evolve (int n) { m_warpx->Evolve(n); }
bool WarpXKernel::verbose () const { return m_warpx->Verbose(); }
TimeState WarpXKernel::time_state (bool valid) const { return make_time(*m_warpx, valid); }
FrameState WarpXKernel::frame_state () const { return FrameState::FromLegacy(); }
RuntimeConfigSnapshot WarpXKernel::config_snapshot () const { return RuntimeConfigSnapshot::FromLegacy(); }
ablastr::fields::MultiFabRegister& WarpXKernel::fields () { return m_warpx->GetMultiFabRegister(); }
const ablastr::fields::MultiFabRegister& WarpXKernel::fields () const { return m_warpx->GetMultiFabRegister(); }
FieldRegistry& WarpXKernel::schema () { return m_warpx->fieldSchema(); }
RuntimeData& WarpXKernel::data () { return *m_data; }
HostServices& WarpXKernel::host () { return *m_host; }
void WarpXKernel::set_plugin_host (PluginHost* host) { m_warpx->SetPluginHost(host); }
void WarpXKernel::set_runtime_context (RuntimeContext* ctx) { m_warpx->SetRuntimeContext(ctx); }

void WarpXKernel::register_plugins (PluginHost& host, bool disabled)
{
    if (!disabled) {
        PicEmPackage::register_handlers(host, field_solver_module_name(m_config));
        DiagnosticsPackage::register_handlers(host);
        CollisionPackage::register_handler(host, m_collision.get());
    }
}

void WarpXKernel::register_species (DataBus& bus)
{
    auto& mpc = m_warpx->GetPartContainer();
    for (const auto& name : mpc.GetSpeciesNames()) {
        auto& pc = mpc.GetParticleContainerFromName(name);
        SpeciesSpec spec;
        spec.name = name;
        spec.charge = pc.getCharge();
        spec.mass = pc.getMass();
        bus.register_species(spec, &pc);
    }
}

void WarpXKernel::allocate_collision_data () { m_collision->AllocData(); }

void WarpXKernel::clear ()
{
    if (m_warpx) { m_warpx->SetRuntimeContext(nullptr); }
    m_host.reset();
    if (m_warpx) { m_warpx->SetCollisionHandler(nullptr); }
    m_collision.reset();
    m_data.reset();
    m_warpx.reset();
    WarpX::ResetInstance();
}

void WarpXKernel::finalize () {}

std::unique_ptr<IPicKernel> create_pic_kernel (int argc, char* argv[])
{
    return std::make_unique<WarpXKernel>(argc, argv);
}

void initialize_runtime_libraries (int argc, char* argv[])
{
    warpx::initialization::initialize_external_libraries(argc, argv);
}

void finalize_runtime_libraries ()
{
    warpx::initialization::finalize_external_libraries();
}

} // namespace warpx::runtime
