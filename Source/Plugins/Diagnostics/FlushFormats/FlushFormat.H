#ifndef WARPX_FLUSHFORMAT_H_
#define WARPX_FLUSHFORMAT_H_

#include "FlushFormat_fwd.H"

#include "Diagnostics/ParticleDiag/ParticleDiag.H"

#include "Runtime/RuntimeContext.H"

namespace warpx::runtime { class RuntimeData; class HostServices; struct RuntimeConfig; }

class FlushFormat
{
public:
    /** Flush fields and particles to file */
    virtual void WriteToFile (
        const amrex::Vector<std::string>& varnames,
        const amrex::Vector<amrex::MultiFab>& mf,
        amrex::Vector<amrex::Geometry>& geom,
        amrex::Vector<int> iteration, double time,
        const amrex::Vector<ParticleDiag>& particle_diags, int nlev,
        std::string prefix, int file_min_digits,
        bool plot_raw_fields,
        bool plot_raw_fields_guards,
        int verbose = 2,
        bool use_pinned_pc = false,
        bool isBTD = false, int snapshotID = -1,
        int bufferID = 1, int numBuffers = 1,
        const amrex::Geometry& full_BTD_snapshot = amrex::Geometry(),
        bool isLastBTDFlush = false) const = 0;

    FlushFormat () = default;
    virtual ~FlushFormat() = default;

    void SetRuntimeContext (const warpx::runtime::RuntimeContext* ctx)
    {
        m_runtime_context = ctx;
    }

    FlushFormat ( FlushFormat const &)             = default;
    FlushFormat& operator= ( FlushFormat const & ) = default;
    FlushFormat ( FlushFormat&& )                  = default;
    FlushFormat& operator= ( FlushFormat&& )       = default;

protected:
    [[nodiscard]] const warpx::runtime::RuntimeData& data () const;
    [[nodiscard]] const warpx::runtime::RuntimeConfig& config () const;
    //! Mutable view for checkpoint writers (VisMF::Write takes MultiFab&).
    [[nodiscard]] ablastr::fields::MultiFabRegister& fields () const;
    [[nodiscard]] warpx::runtime::HostServices& host () const;

    const warpx::runtime::RuntimeContext* m_runtime_context = nullptr;
};

#endif // WARPX_FLUSHFORMAT_H_
