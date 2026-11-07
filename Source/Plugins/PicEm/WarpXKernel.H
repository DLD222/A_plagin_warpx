#ifndef WARPX_PLUGIN_WARPXKERNEL_H_
#define WARPX_PLUGIN_WARPXKERNEL_H_
#include "Runtime/IPicKernel.H"
#include "Runtime/RuntimeConfig.H"
#include <memory>
class WarpX; class CollisionHandler;
namespace warpx::runtime {
class WarpXRuntimeData; class WarpXHostServices;
class WarpXKernel final : public IPicKernel {
public:
    explicit WarpXKernel (int argc, char* argv[]);
    ~WarpXKernel () override;
    void initialize () override;
    void evolve (int) override;
    void finalize () override;
    bool verbose () const override;
    TimeState time_state (bool) const override;
    FrameState frame_state () const override;
    RuntimeConfigSnapshot config_snapshot () const override;
    ablastr::fields::MultiFabRegister& fields () override;
    const ablastr::fields::MultiFabRegister& fields () const override;
    FieldRegistry& schema () override;
    RuntimeData& data () override;
    HostServices& host () override;
    void set_plugin_host (PluginHost*) override;
    void set_runtime_context (RuntimeContext*) override;
    void register_plugins (PluginHost&, bool disabled) override;
    void register_species (DataBus&) override;
    void allocate_collision_data () override;
    void clear () override;
private:
    std::unique_ptr<WarpX> m_warpx;
    std::unique_ptr<WarpXRuntimeData> m_data;
    std::unique_ptr<WarpXHostServices> m_host;
    RuntimeConfig m_config;
    std::unique_ptr<CollisionHandler> m_collision;
};
}
#endif
