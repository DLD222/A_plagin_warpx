#ifndef WARPX_RUNTIME_NULLDIAGNOSTICSSERVICE_H_
#define WARPX_RUNTIME_NULLDIAGNOSTICSSERVICE_H_
#include "Runtime/IDiagnosticsService.H"
#include "Runtime/IReducedDiagsService.H"
namespace warpx::runtime {
struct NullDiagnosticsService final : IDiagnosticsService {
    void InitData (const IParticleContainerAccess&) override {}
    bool DoComputeAndPack (int, bool = false) override { return false; }
    void FilterComputePackFlush (int, bool = false, bool = false) override {}
    void FilterComputePackFlushLastTimestep (int) override {}
    void InitializeFieldFunctors (int) override {}
    void ResizeLevels (int) override {}
    void RemakeLevel (int) override {}
    void NewIteration () override {}
    int GetTotalDiags () const override { return 0; }
    std::string diagName (int) const override { return {}; }
    bool isDiagBackTransformed (int) const override { return false; }
    int diagNumBuffers (int) const override { return 0; }
    amrex::Real diagTlab (int, int) const override { return 0; }
    int diagBufferKIndexHi (int, int) const override { return 0; }
    amrex::Real diagSnapshotDomainLo (int, int, int) const override { return 0; }
    amrex::Real diagSnapshotDomainHi (int, int, int) const override { return 0; }
    int diagFlushCounter (int, int) const override { return 0; }
    int diagLastValidZslice (int, int) const override { return 0; }
    int diagSnapshotFullFlag (int, int) const override { return 0; }
};
struct NullReducedDiagsService final : IReducedDiagsService {
    void InitData () override {}
    bool DoDiags (int) override { return false; }
    void LoadBalance () override {}
    void ComputeDiags (int) override {}
    void ComputeDiagsMidStep (int, amrex::Real) override {}
    void WriteToFile (int) override {}
    void WriteCheckpointData (const std::string&) override {}
    void ReadCheckpointData (const std::string&) override {}
    int plotReducedDiags () const override { return 0; }
};
}
#endif
