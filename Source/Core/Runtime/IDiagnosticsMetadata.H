#ifndef WARPX_RUNTIME_IDIAGNOSTICSMETADATA_H_
#define WARPX_RUNTIME_IDIAGNOSTICSMETADATA_H_
#include <AMReX_REAL.H>
#include <AMReX_Vector.H>
#include <AMReX_iMultiFab.H>
#include <memory>
#include <vector>
#include <string>
namespace warpx::runtime {
class IDiagnosticsMetadata {
public:
    virtual ~IDiagnosticsMetadata () = default;
    virtual int totalDiags () const = 0;
    virtual std::string diagName (int) const = 0;
    virtual bool isDiagBackTransformed (int) const = 0;
    virtual int diagNumBuffers (int) const = 0;
    virtual amrex::Real diagTlab (int, int) const = 0;
    virtual int diagBufferKIndexHi (int, int) const = 0;
    virtual amrex::Real diagSnapshotDomainLo (int, int, int) const = 0;
    virtual amrex::Real diagSnapshotDomainHi (int, int, int) const = 0;
    virtual int diagFlushCounter (int, int) const = 0;
    virtual int diagLastValidZslice (int, int) const = 0;
    virtual int diagSnapshotFullFlag (int, int) const = 0;
    virtual amrex::Vector<int> nsubsteps () const = 0;
    virtual amrex::Vector<amrex::Real> tNew () const = 0;
    virtual amrex::Vector<amrex::Real> tOld () const = 0;
    virtual amrex::Vector<amrex::Real> dt () const = 0;
    virtual void prepareFieldsForDiagnostics () = 0;
    virtual const amrex::Vector<std::unique_ptr<amrex::iMultiFab>>& ebReduceParticleShapeFlag () const = 0;
    virtual int hybridPicSubsteps () const = 0;
    virtual bool hasHybridPICModel () const = 0;
    virtual std::string authors () const = 0;
    virtual void computeDiagnostics (int) = 0;
};
}
#endif
