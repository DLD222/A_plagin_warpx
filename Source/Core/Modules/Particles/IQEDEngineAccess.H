#ifndef WARPX_CORE_PARTICLES_IQEDENGINEACCESS_H_
#define WARPX_CORE_PARTICLES_IQEDENGINEACCESS_H_
class BreitWheelerEngine;
class QuantumSynchrotronEngine;
namespace warpx::runtime {
/** Opaque QED engine access; concrete engine stays in QED plugin. */
class IQEDEngineAccess
{
public:
    virtual ~IQEDEngineAccess () = default;
    [[nodiscard]] virtual BreitWheelerEngine* get_breit_wheeler_engine_ptr () const = 0;
    [[nodiscard]] virtual QuantumSynchrotronEngine* get_quantum_sync_engine_ptr () const = 0;
};
}
#endif
