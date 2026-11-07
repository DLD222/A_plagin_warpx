#ifndef WARPX_RUNTIME_IDIAGNOSTICSSERVICE_H_
#define WARPX_RUNTIME_IDIAGNOSTICSSERVICE_H_
#include <AMReX_REAL.H>
#include <memory>
#include <string>
namespace warpx::runtime {
class IParticleContainerAccess;
class RuntimeContext;
class IDiagnosticsService {
public:
    virtual ~IDiagnosticsService () = default;
    virtual void InitData (const IParticleContainerAccess&) = 0;
    virtual bool DoComputeAndPack (int step, bool force_flush = false) = 0;
    virtual void FilterComputePackFlush (int step, bool force_flush = false,
                                         bool back_transform = false) = 0;
    virtual void FilterComputePackFlushLastTimestep (int step) = 0;
    virtual void InitializeFieldFunctors (int lev) = 0;
    virtual void ResizeLevels (int new_finest) = 0;
    virtual void RemakeLevel (int lev) = 0;
    virtual void NewIteration () = 0;
    [[nodiscard]] virtual int GetTotalDiags () const = 0;
    [[nodiscard]] virtual std::string diagName (int i) const = 0;
    [[nodiscard]] virtual bool isDiagBackTransformed (int i) const = 0;
    [[nodiscard]] virtual int diagNumBuffers (int i) const = 0;
    [[nodiscard]] virtual amrex::Real diagTlab (int i, int b) const = 0;
    [[nodiscard]] virtual int diagBufferKIndexHi (int i, int b) const = 0;
    [[nodiscard]] virtual amrex::Real diagSnapshotDomainLo (int i, int b, int d) const = 0;
    [[nodiscard]] virtual amrex::Real diagSnapshotDomainHi (int i, int b, int d) const = 0;
    [[nodiscard]] virtual int diagFlushCounter (int i, int b) const = 0;
    [[nodiscard]] virtual int diagLastValidZslice (int i, int b) const = 0;
    [[nodiscard]] virtual int diagSnapshotFullFlag (int i, int b) const = 0;
};
std::unique_ptr<IDiagnosticsService> create_diagnostics_service (const RuntimeContext*);
}
#endif
