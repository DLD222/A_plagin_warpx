#ifndef WARPX_RUNTIME_IPICKERNEL_H_
#define WARPX_RUNTIME_IPICKERNEL_H_
#include "Runtime/FrameState.H"
#include "Runtime/RuntimeConfigSnapshot.H"
#include "Runtime/TimeState.H"
#include <ablastr/fields/MultiFabRegister.H>
#include <memory>
namespace warpx::runtime {
class DataBus; class FieldRegistry; class HostServices; class PluginHost;
class RuntimeContext; class RuntimeData;
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
    virtual void register_species (DataBus&) = 0;
    virtual void allocate_collision_data () = 0;
    virtual void clear () = 0;
};
std::unique_ptr<IPicKernel> create_pic_kernel (int argc, char* argv[]);
void initialize_runtime_libraries (int argc, char* argv[]);
void finalize_runtime_libraries ();
}
#endif
