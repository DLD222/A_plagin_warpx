#ifndef WARPX_RUNTIME_IPICKERNEL_H_
#define WARPX_RUNTIME_IPICKERNEL_H_
#include "Runtime/FrameState.H"
#include "Runtime/RuntimeConfigSnapshot.H"
#include "Runtime/TimeState.H"
#include <ablastr/fields/MultiFabRegister.H>
#include <memory>
#include <string>
#include <vector>
namespace warpx::runtime {
class DataBus; class FieldRegistry; class HostServices; class PluginHost;
class RuntimeContext; class RuntimeData;
/**
 * \brief The kernel contract: what the runtime needs from a simulation kernel.
 *
 * Despite the historical name, nothing here is PIC-specific: a fluid kernel
 * implements the same interface (see tests/runtime/test_nonpic_kernel.cpp).
 * The particle-only parts are optional hooks with empty defaults.  A7.9 renames
 * this to ISimulationKernel once the remaining PIC-flavoured accessors in
 * RuntimeData move behind capability interfaces.
 */
class IPicKernel {
public:
    virtual ~IPicKernel () = default;
    virtual void initialize () = 0;
    virtual void evolve (int numsteps) = 0;
    virtual void finalize () = 0;
    virtual bool verbose () const = 0;
    virtual TimeState time_state (bool valid) const = 0;
    virtual FrameState frame_state () const = 0;
    virtual RuntimeConfigSnapshot config_snapshot () const = 0;
    virtual ablastr::fields::MultiFabRegister& fields () = 0;
    virtual const ablastr::fields::MultiFabRegister& fields () const = 0;
    virtual FieldRegistry& schema () = 0;
    virtual RuntimeData& data () = 0;
    virtual HostServices& host () = 0;
    virtual void set_plugin_host (PluginHost*) = 0;
    virtual void set_runtime_context (RuntimeContext*) = 0;
    virtual void register_plugins (PluginHost&, bool disabled) = 0;
    //! Particle capability hook: publish the species on the bus.  A kernel
    //! without particles (a fluid kernel) leaves the default in place.
    virtual void register_species (DataBus& /*bus*/) {}
    //! Particle capability hook: allocate collision state.  Optional for the
    //! same reason.
    virtual void allocate_collision_data () {}
    virtual void clear () = 0;

    //! Features this run actually uses ("amr", "eb", "qed", "psatd", ...).
    //! Used to check plugin capability declarations before the first step.
    [[nodiscard]] virtual std::vector<std::string> active_features () const = 0;
};
// The factory and the library hooks live in Runtime/KernelFactory.H: Core
// declares them, a kernel plugin (PicEm) registers them at static-init time.
}
#endif
