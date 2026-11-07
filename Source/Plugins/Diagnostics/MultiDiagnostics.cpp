#include "MultiDiagnostics.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/ISpeciesMetadata.H"
#include "Runtime/IParticleContainerAccess.H"

#include "Diagnostics/BTDiagnostics.H"
#include "Diagnostics/FullDiagnostics.H"
#include "Diagnostics/BoundaryScrapingDiagnostics.H"
#include "Utils/TextMsg.H"
#include <ablastr/warn_manager/WarnManager.H>
#include <AMReX_ParmParse.H>
#include <AMReX.H>
#include <AMReX_REAL.H>

using namespace amrex;

MultiDiagnostics::MultiDiagnostics (const warpx::runtime::RuntimeContext* ctx)
    : m_ctx(ctx)
{
    ReadParameters();
    /** Resize alldiags and initialize each element to a pointer to a
     * diagnostics. Calls the corresponding diagnostics constructor.
     */
    alldiags.resize( ndiags );
    for (int i=0; i<ndiags; i++){
        if ( diags_types[i] == DiagTypes::Full || diags_types[i] == DiagTypes::TimeAveraged ){
            alldiags[i] = std::make_unique<FullDiagnostics>(i, diags_names[i], diags_types[i], m_ctx);
        } else if ( diags_types[i] == DiagTypes::BackTransformed ){
            alldiags[i] = std::make_unique<BTDiagnostics>(i, diags_names[i], diags_types[i], m_ctx);
        } else if ( diags_types[i] == DiagTypes::BoundaryScraping ){
            alldiags[i] = std::make_unique<BoundaryScrapingDiagnostics>(i, diags_names[i], diags_types[i], m_ctx);
        } else {
            WARPX_ABORT_WITH_MESSAGE("Unknown diagnostic type");
        }
    }
}

void
MultiDiagnostics::InitData (const warpx::runtime::IParticleContainerAccess& mpc)
{
    for( auto& diag : alldiags ){
        diag->InitData(mpc);
    }
}

void
MultiDiagnostics::RemakeLevel ( int lev )
{
    for (auto& diag : alldiags) { diag->RemakeLevel(lev); }
}

void
MultiDiagnostics::ResizeLevels ( int new_finest )
{
    for (auto& diag : alldiags) { diag->ResizeLevels(new_finest); }
}

void
MultiDiagnostics::InitializeFieldFunctors ( int lev )
{
    for( auto& diag : alldiags ){
        // Initialize functors to store pointers to fields.
        diag->InitializeFieldFunctors( lev );
    }
}

void
MultiDiagnostics::ReadParameters ()
{
    const ParmParse pp_diagnostics("diagnostics");

    int enable_diags = 1;
    pp_diagnostics.query("enable", enable_diags);
    if (enable_diags == 1) {
        pp_diagnostics.queryarr("diags_names", diags_names);
        ndiags = static_cast<int>(diags_names.size());
    }

    diags_types.resize( ndiags );
    for (int i=0; i<ndiags; i++){
        const ParmParse pp_diag_name(diags_names[i]);
        std::string diag_type_str;
        pp_diag_name.get("diag_type", diag_type_str);
        WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
            diag_type_str == "Full" || diag_type_str == "TimeAveraged" || diag_type_str == "BackTransformed" || diag_type_str == "BoundaryScraping",
            "<diag>.diag_type must be Full, TimeAveraged, BackTransformed or BoundaryScraping");
        if (diag_type_str == "Full") { diags_types[i] = DiagTypes::Full; }
        if (diag_type_str == "TimeAveraged") { diags_types[i] = DiagTypes::TimeAveraged; }
        if (diag_type_str == "BackTransformed") { diags_types[i] = DiagTypes::BackTransformed; }
        if (diag_type_str == "BoundaryScraping") { diags_types[i] = DiagTypes::BoundaryScraping; }
    }
}

bool
MultiDiagnostics::DoComputeAndPack (int step, bool force_flush)
{
    bool result = false;
    for( auto& diag : alldiags ){
        result = result || diag->DoComputeAndPack(step, force_flush);
    }
    return result;
}

void
MultiDiagnostics::FilterComputePackFlush (int step, bool force_flush, bool BackTransform)
{
    int i = 0;
    for (auto& diag : alldiags){
        if (BackTransform) {
            if (diags_types[i] == DiagTypes::BackTransformed) {
                diag->FilterComputePackFlush (step, force_flush);
            }
        } else {
            if (diags_types[i] != DiagTypes::BackTransformed) {
                diag->FilterComputePackFlush (step, force_flush);
            }
        }
        ++i;
    }
}

void
MultiDiagnostics::FilterComputePackFlushLastTimestep (int step)
{
    for (auto& diag : alldiags){
        if (diag->DoDumpLastTimestep()){
            constexpr bool force_flush = true;
            diag->FilterComputePackFlush (step, force_flush);
        }
    }
}

void
MultiDiagnostics::NewIteration ()
{
    for( auto& diag : alldiags ){
        diag->NewIteration();
    }
}

void
MultiDiagnostics::ReadCheckpointData (std::istream& is,
                                      const warpx::runtime::IParticleContainerAccess& mpc)
{
    for (int idiag = 0; idiag < GetTotalDiags(); ++idiag) {
        if (diags_types[idiag] == DiagTypes::BackTransformed) {
            auto& diag = *alldiags[idiag];
            if (diag.getnumbuffers() > 0) {
                diag.InitDataBeforeRestart();
                for (int i_buffer=0; i_buffer<diag.getnumbuffers(); ++i_buffer) {
                    amrex::Real tlab; is >> tlab; diag.settlab(i_buffer, tlab);
                    int kindex_hi; is >> kindex_hi; diag.set_buffer_k_index_hi(i_buffer, kindex_hi);
                    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
                        amrex::Real lo; is >> lo; diag.setSnapshotDomainLo(i_buffer, idim, lo);
                    }
                    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
                        amrex::Real hi; is >> hi; diag.setSnapshotDomainHi(i_buffer, idim, hi);
                    }
                    int flush_counter; is >> flush_counter; diag.set_flush_counter(i_buffer, flush_counter);
                    int last_z; is >> last_z; diag.set_last_valid_Zslice(i_buffer, last_z);
                    int full; is >> full; diag.set_snapshot_full(i_buffer, full);
                }
                diag.InitDataAfterRestart(mpc);
            } else {
                diag.InitData(mpc);
            }
        } else {
            alldiags[idiag]->InitData(mpc);
        }
    }
}
