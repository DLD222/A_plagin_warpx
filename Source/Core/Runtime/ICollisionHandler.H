#ifndef WARPX_RUNTIME_ICOLLISIONHANDLER_H_
#define WARPX_RUNTIME_ICOLLISIONHANDLER_H_
#include <AMReX_REAL.H>

#include <memory>
#include <string>
#include <vector>

namespace warpx::runtime {

class HostServices;
class IParticleContainerAccess;
class RuntimeConfig;
class RuntimeData;
class StateRegistry;
/** Collision plugin boundary (A7.2.2). */
class ICollisionHandler
{
public:
    virtual ~ICollisionHandler () = default;
    virtual void AllocData () = 0;
    //! Declare the state this handler needs (S4): the core allocates it with
    //! the levels, so diagnostics can already see the names.
    virtual void declare_state (StateRegistry& /*state*/) {}
    virtual void doCollisions (int step, amrex::Real cur_time, amrex::Real dt) = 0;

    //! Species this handler reads / writes, for the stage graph's dependency and conflict edges.
    //!
    //! Species names come from the input file, so the adapter cannot guess them (it used to
    //! declare the fixed set electrons/ions/gas/photons, which is wrong for almost every deck and
    //! silently disabled the write-conflict check for the real names).  A handler that does not
    //! override these contributes no species edges.
    [[nodiscard]] virtual std::vector<std::string> readsSpecies () const { return {}; }
    [[nodiscard]] virtual std::vector<std::string> writesSpecies () const { return {}; }
};

/**
 * \brief Collision plugin boundary: the core asks, a plugin answers (S6).
 *
 * The kernel never constructs the concrete handler; it calls this factory,
 * which returns nullptr when no collision plugin is linked, so the rest of the
 * simulation keeps working without it.
 */
using CollisionHandlerFactory = std::unique_ptr<ICollisionHandler> (*) (
    const IParticleContainerAccess& particles, const RuntimeData& data,
    const RuntimeConfig& config, HostServices* host);

void register_collision_handler_factory (CollisionHandlerFactory factory);

[[nodiscard]] std::unique_ptr<ICollisionHandler>
create_collision_handler (const IParticleContainerAccess& particles,
                          const RuntimeData& data, const RuntimeConfig& config,
                          HostServices* host);

[[nodiscard]] bool has_collision_handler_factory ();

} // namespace warpx::runtime
#endif
