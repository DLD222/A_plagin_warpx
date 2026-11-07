#ifndef WARPX_RUNTIME_IREDUCEDDIAGSSERVICE_H_
#define WARPX_RUNTIME_IREDUCEDDIAGSSERVICE_H_
#include <AMReX_REAL.H>
#include <memory>
#include <string>
namespace warpx::runtime {
class RuntimeContext;
class IReducedDiagsService {
public:
    virtual ~IReducedDiagsService () = default;
    virtual void InitData () = 0;
    virtual bool DoDiags (int step) = 0;
    virtual void LoadBalance () = 0;
    virtual void ComputeDiags (int step) = 0;
    virtual void ComputeDiagsMidStep (int step, amrex::Real dt) = 0;
    virtual void WriteToFile (int step) = 0;
    virtual void WriteCheckpointData (const std::string& dir) = 0;
    virtual void ReadCheckpointData (const std::string& dir) = 0;
    [[nodiscard]] virtual int plotReducedDiags () const = 0;
};
std::unique_ptr<IReducedDiagsService> create_reduced_diags_service (const RuntimeContext*);
}
#endif
