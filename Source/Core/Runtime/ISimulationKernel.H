#ifndef WARPX_RUNTIME_IPICKERNEL_H_
#define WARPX_RUNTIME_IPICKERNEL_H_
#include "Runtime/FrameState.H"
#include "Runtime/IParticleCapability.H"
#include "Runtime/RuntimeConfigSnapshot.H"
#include "Runtime/TimeState.H"
#include <ablastr/fields/MultiFabRegister.H>
#include <memory>
#include <string>
#include <vector>
namespace warpx::runtime {
class DataBus; class FieldRegistry; class HostServices; class PluginHost;
class IParticleCapability;
class RuntimeContext; class RuntimeData;
/**
 * \brief The kernel contract: what the runtime needs from a simulation kernel.
 *
 * Nothing here is PIC-specific: a fluid kernel implements the same interface
 * (see tests/runtime/test_nonpic_kernel.cpp).  The particle-specific setup lives
 * behind the optional IParticleCapability a kernel may return.
 */
class ISimulationKernel {
public:
    virtual ~ISimulationKernel () = default;
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
    /**
     * \brief The particle capability, or nullptr for a kernel without particles.
     *
     * A fluid kernel returns nullptr and the runtime simply skips the particle
     * setup; nothing else in this contract mentions particles.
     */
    [[nodiscard]] virtual IParticleCapability* particle_capability () { return nullptr; }
    virtual void clear () = 0;

    //! Features this run actually uses ("amr", "eb", "qed", "psatd", ...).
    //! Used to check plugin capability declarations before the first step.
    [[nodiscard]] virtual std::vector<std::string> active_features () const = 0;
};
// The factory and the library hooks live in Runtime/KernelFactory.H: Core
// declares them, a kernel plugin (PicEm) registers them at static-init time.
}
#endif
