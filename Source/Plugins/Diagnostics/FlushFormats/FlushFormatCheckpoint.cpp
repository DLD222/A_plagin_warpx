#include "FlushFormatCheckpoint.H"
#include "Diagnostics/DiagnosticsDependencies.H"
#include "Runtime/IParticleContainerAccess.H"
#include "Runtime/ISpeciesMetadata.H"

#include "Runtime/IPML.H"
#if (defined WARPX_DIM_RZ) && (defined WARPX_USE_FFT)
#   include "BoundaryConditions/PML_RZ.H"
#endif
#include "Diagnostics/ParticleDiag/ParticleDiag.H"
#include "Diagnostics/ReducedDiags/MultiReducedDiags.H"
#include "Fields.H"
#include "Modules/Particles/ParticleTileIter.H"
#include "Utils/TextMsg.H"
#include "Runtime/HostServices.H"
#include "Runtime/RuntimeConfig.H"
#include "Runtime/RuntimeData.H"

#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/profiler/ProfilerWrapper.H>

#include <AMReX_MultiFab.H>
#include <AMReX_ParticleIO.H>
#include <AMReX_PlotFileUtil.H>
#include <AMReX_Print.H>
#include <AMReX_REAL.H>
#include <AMReX_Utility.H>
#include <AMReX_VisMF.H>

#include <fstream>
#include <map>
#include <optional>

#ifndef WARPX_UNITY_ID
#define WARPX_UNITY_ID
#endif

using namespace amrex;
using warpx::fields::FieldType;

namespace
{

struct CheckpointFieldInfo
{
    std::string file;
    FieldType type;
    int dir;
};

std::optional<CheckpointFieldInfo>
checkpointFieldInfo (const std::string& key)
{
    static const std::map<std::string, CheckpointFieldInfo> fields = {
        {"Efield_fp_x", {"Ex_fp", FieldType::Efield_fp, 0}},
        {"Efield_fp_y", {"Ey_fp", FieldType::Efield_fp, 1}},
        {"Efield_fp_z", {"Ez_fp", FieldType::Efield_fp, 2}},
        {"Bfield_fp_x", {"Bx_fp", FieldType::Bfield_fp, 0}},
        {"Bfield_fp_y", {"By_fp", FieldType::Bfield_fp, 1}},
        {"Bfield_fp_z", {"Bz_fp", FieldType::Bfield_fp, 2}},
        {"E_old_x", {"Ex_old", FieldType::E_old, 0}},
        {"E_old_y", {"Ey_old", FieldType::E_old, 1}},
        {"E_old_z", {"Ez_old", FieldType::E_old, 2}},
        {"Efield_avg_fp_x", {"Ex_avg_fp", FieldType::Efield_avg_fp, 0}},
        {"Efield_avg_fp_y", {"Ey_avg_fp", FieldType::Efield_avg_fp, 1}},
        {"Efield_avg_fp_z", {"Ez_avg_fp", FieldType::Efield_avg_fp, 2}},
        {"Bfield_avg_fp_x", {"Bx_avg_fp", FieldType::Bfield_avg_fp, 0}},
        {"Bfield_avg_fp_y", {"By_avg_fp", FieldType::Bfield_avg_fp, 1}},
        {"Bfield_avg_fp_z", {"Bz_avg_fp", FieldType::Bfield_avg_fp, 2}},
        {"current_fp_x", {"jx_fp", FieldType::current_fp, 0}},
        {"current_fp_y", {"jy_fp", FieldType::current_fp, 1}},
        {"current_fp_z", {"jz_fp", FieldType::current_fp, 2}},
        {"Efield_cp_x", {"Ex_cp", FieldType::Efield_cp, 0}},
        {"Efield_cp_y", {"Ey_cp", FieldType::Efield_cp, 1}},
        {"Efield_cp_z", {"Ez_cp", FieldType::Efield_cp, 2}},
        {"Bfield_cp_x", {"Bx_cp", FieldType::Bfield_cp, 0}},
        {"Bfield_cp_y", {"By_cp", FieldType::Bfield_cp, 1}},
        {"Bfield_cp_z", {"Bz_cp", FieldType::Bfield_cp, 2}},
        {"Efield_avg_cp_x", {"Ex_avg_cp", FieldType::Efield_avg_cp, 0}},
        {"Efield_avg_cp_y", {"Ey_avg_cp", FieldType::Efield_avg_cp, 1}},
        {"Efield_avg_cp_z", {"Ez_avg_cp", FieldType::Efield_avg_cp, 2}},
        {"Bfield_avg_cp_x", {"Bx_avg_cp", FieldType::Bfield_avg_cp, 0}},
        {"Bfield_avg_cp_y", {"By_avg_cp", FieldType::Bfield_avg_cp, 1}},
        {"Bfield_avg_cp_z", {"Bz_avg_cp", FieldType::Bfield_avg_cp, 2}},
        {"current_cp_x", {"jx_cp", FieldType::current_cp, 0}},
        {"current_cp_y", {"jy_cp", FieldType::current_cp, 1}},
        {"current_cp_z", {"jz_cp", FieldType::current_cp, 2}}
    };
    const auto it = fields.find(key);
    if (it == fields.end()) { return std::nullopt; }
    return it->second;
}

namespace WARPX_UNITY_ID
{
    const std::string default_level_prefix {"Level_"};
}
}

void
FlushFormatCheckpoint::WriteToFile (
        const amrex::Vector<std::string>& /*varnames*/,
        const amrex::Vector<amrex::MultiFab>& /*mf*/,
        amrex::Vector<amrex::Geometry>& geom,
        const amrex::Vector<int> iteration, const double /*time*/,
        const amrex::Vector<ParticleDiag>& particle_diags, int nlev,
        const std::string prefix, int file_min_digits,
        bool /*plot_raw_fields*/,
        bool /*plot_raw_fields_guards*/,
        int verbose,
        const bool /*use_pinned_pc*/,
        bool /*isBTD*/, int /*snapshotID*/,
        int /*bufferID*/, int /*numBuffers*/,
        const amrex::Geometry& /*full_BTD_snapshot*/,
        bool /*isLastBTDFlush*/) const
{
    using ablastr::fields::Direction;
    using WARPX_UNITY_ID::default_level_prefix;

    ABLASTR_PROFILE("FlushFormatCheckpoint::WriteToFile()");

    const VisMF::Header::Version current_version = VisMF::GetHeaderVersion();
    VisMF::SetHeaderVersion(amrex::VisMF::Header::NoFabHeader_v1);

    const std::string& checkpointname = amrex::Concatenate(prefix, iteration[0], file_min_digits);

    if (verbose > 0) {
        amrex::Print() << Utils::TextMsg::Info(
            "Writing checkpoint " + checkpointname);
    }

    // const int nlevels = finestLevel()+1;
    amrex::PreBuildDirectorHierarchy(checkpointname, default_level_prefix, nlev, true);

    WriteWarpXHeader(checkpointname, geom, /*is_checkpoint=*/true);

    WriteJobInfo(checkpointname);

    // Additive schema manifest: old readers ignore it, new readers can
    // validate the checkpoint field list against the live FieldRegistry.
    {
        std::ofstream schema_file(checkpointname + "/field_schema.txt");
        for (const auto& name : data().fieldRegistry().checkpointNames()) {
            schema_file << name << "\n";
        }
    }

    for (int lev = 0; lev < nlev; ++lev)
    {
        for (const auto& key : data().fieldRegistry().checkpointNames()) {
            const auto info = checkpointFieldInfo(key);
            if (!info.has_value()) { continue; }
            const auto dir = ablastr::fields::Direction{info->dir};
            if (!fields().has(info->type, dir, lev)) { continue; }
            if (key.rfind("current_", 0) == 0 && !data().isSynchronized()) { continue; }
            VisMF::Write(*fields().get(info->type, dir, lev),
                amrex::MultiFabFileFullPrefix(lev, checkpointname, default_level_prefix, info->file));
        }

        if (host().pml().doPML()) {
            host().pml().checkpointPML(
                lev,
                amrex::MultiFabFileFullPrefix(lev, checkpointname, default_level_prefix, "pml"),
                fields());
        }

        fields().write_checkpoints(lev, amrex::MultiFabFileFullPrefix(lev, checkpointname, default_level_prefix, ""));

    }

    CheckpointParticles(checkpointname, particle_diags);

    WriteDMaps(checkpointname, nlev);

    WriteReducedDiagsData(checkpointname);

    VisMF::SetHeaderVersion(current_version);

}

void
FlushFormatCheckpoint::CheckpointParticles (
    const std::string& dir,
    const amrex::Vector<ParticleDiag>& particle_diags) const
{
    for (const auto& part_diag: particle_diags) {
        warpx::runtime::IParticleContainer* pc = part_diag.getParticleContainer();

        Vector<std::string> real_names;
        Vector<std::string> int_names;
        Vector<int> write_real_comps;
        Vector<int> write_int_comps;

        // note: positions skipped here, since we reconstruct a plotfile SoA from them
        std::vector<std::string> const fixed_names = {"weight",
                                                      "momentum_x",
                                                      "momentum_y",
                                                      "momentum_z"
#if defined(WARPX_DIM_RZ) || defined(WARPX_DIM_RCYLINDER) || defined(WARPX_DIM_RSPHERE)
                                                      ,"theta"
#endif
#if defined(WARPX_DIM_RSPHERE)
                                                      ,"phi"
#endif
                                                      };

        for (auto const& name : fixed_names) {
            real_names.push_back(name);
            write_real_comps.push_back(1);
        }

        // get the names of the extra real comps
        real_names.resize(pc->NumRealComps() - AMREX_SPACEDIM);
        write_real_comps.resize(pc->NumRealComps() - AMREX_SPACEDIM);

        // note, skip the required component names here
        auto rnames = pc->GetRealSoANames();
        for (std::size_t index = PIdx::nattribs; index < rnames.size(); ++index) {
            std::size_t const i = index - AMREX_SPACEDIM;
            real_names[i] = rnames[index];
            write_real_comps[i] = pc->h_redistribute_real_comp[index];
        }

        // and the int comps
        int_names.resize(pc->NumIntComps());
        write_int_comps.resize(pc->NumIntComps());
        //   note: inames and h_redistribute_int_comp are not the same size
        auto inames = pc->GetIntSoANames();
        std::size_t const i0_redist = pc->h_redistribute_int_comp.size() - inames.size();
        for (std::size_t index = 0; index < inames.size(); ++index) {
            int_names[index] = inames[index];
            write_int_comps[index] = pc->h_redistribute_int_comp[i0_redist + index];
        }

        pc->Checkpoint(dir, part_diag.getSpeciesName(),
                       write_real_comps, write_int_comps,
                       real_names, int_names);
    }
}

void
FlushFormatCheckpoint::WriteDMaps (const std::string& dir, int nlev) const
{
    if (ParallelDescriptor::IOProcessor()) {
        for (int lev = 0; lev < nlev; ++lev) {
            std::string DMFileName = dir;
            if (!DMFileName.empty() && DMFileName[DMFileName.size()-1] != '/') {DMFileName += '/';}
            DMFileName = amrex::Concatenate(DMFileName.append("Level_"), lev, 1);
            DMFileName += "/DM";

            std::ofstream DMFile;
            DMFile.open(DMFileName.c_str(), std::ios::out|std::ios::trunc);

            if (!DMFile.good()) { amrex::FileOpenFailed(DMFileName); }

            DMFile << ParallelDescriptor::NProcs() << "\n";
            data().distributionMap(lev).writeOn(DMFile);

            DMFile.flush();
            DMFile.close();
            WARPX_ALWAYS_ASSERT_WITH_MESSAGE(
                DMFile.good(),
                "FlushFormatCheckpoint::WriteDMaps: problem writing DMFile"
            );
        }
    }
}

void
FlushFormatCheckpoint::WriteReducedDiagsData (std::string const & dir) const
{
    if (ParallelDescriptor::IOProcessor()) {
        host().checkpoint().writeReducedDiagCheckpoint(dir);
    }
}
