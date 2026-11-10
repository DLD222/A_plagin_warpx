#include "Modules/Particles/Gather/GetExternalFields.H"

#include "AcceleratorLattice/AcceleratorLattice.H"

#include "Particles/MultiParticleContainer.H"
#include "WarpX.H"
#include "Particles/WarpXParticleContainer.H"
#include "Utils/TextMsg.H"
#include "Runtime/FrameState.H"
#include "Runtime/RuntimeContext.H"
#include "Runtime/RuntimeData.H"
#include "Runtime/WarpXRuntimeData.H"
#include "Runtime/LegacyServices.H"

#include <AMReX_Vector.H>

#include <string>

using namespace amrex::literals;

template <typename ParIterType>
GetExternalEBField::GetExternalEBField (const ParIterType& a_pti, long a_offset,
                                        const warpx::runtime::RuntimeContext* ctx) noexcept
{
    static const warpx::runtime::WarpXRuntimeData legacy(WarpX::GetInstance());
    const warpx::runtime::RuntimeData& d =
        (ctx != nullptr && ctx->data != nullptr) ? *ctx->data : static_cast<const warpx::runtime::RuntimeData&>(legacy);
    const warpx::runtime::RuntimeConfig& cfg =
        (ctx != nullptr && ctx->config != nullptr) ? *ctx->config : warpx::runtime::legacy_config();
    const auto& mypc = WarpX::GetInstance().GetPartContainer();

    const int lev = a_pti.GetLevel();

    const auto& dts = d.dtAll();

    AcceleratorLattice const & accelerator_lattice = WarpX::GetInstance().get_accelerator_lattice(lev);
    if (accelerator_lattice.m_lattice_defined) {
        d_lattice_element_finder = accelerator_lattice.GetFinderDeviceInstance(a_pti, static_cast<int>(a_offset), dts);
    }

    const warpx::runtime::FrameState& fr =
        (ctx != nullptr && ctx->frame != nullptr) ? *ctx->frame : warpx::runtime::FrameState::FromLegacy();
    m_gamma_boost = fr.gamma_boost;
    m_uz_boost = std::sqrt(fr.gamma_boost*fr.gamma_boost - 1._prt)*PhysConst::c;

    m_Etype = Unknown;
    m_Btype = Unknown;

    if (cfg.E_ext_particle_s() == "none") { m_Etype = None; }
    if (cfg.B_ext_particle_s() == "none") { m_Btype = None; }

    // These lines will be removed once the user interface is redefined and the CI tests updated
    if (cfg.E_ext_particle_s() == "constant") { m_Etype = None; }
    if (cfg.B_ext_particle_s() == "constant") { m_Btype = None; }

    if (cfg.E_ext_particle_s() == "parse_e_ext_particle_function" ||
        cfg.B_ext_particle_s() == "parse_b_ext_particle_function" ||
        cfg.E_ext_particle_s() == "repeated_plasma_lens" ||
        cfg.B_ext_particle_s() == "repeated_plasma_lens")
    {
        m_time = d.t_new(a_pti.GetLevel());
        m_get_position = GetParticlePosition<PIdx>(a_pti, a_offset);
    }

    if (cfg.E_ext_particle_s() == "parse_e_ext_particle_function")
    {
        m_Etype = ExternalFieldInitType::Parser;
        m_Exfield_partparser = mypc.m_Ex_particle_parser->compile<4>();
        m_Eyfield_partparser = mypc.m_Ey_particle_parser->compile<4>();
        m_Ezfield_partparser = mypc.m_Ez_particle_parser->compile<4>();
    }

    if (cfg.B_ext_particle_s() == "parse_b_ext_particle_function")
    {
        m_Btype = ExternalFieldInitType::Parser;
        m_Bxfield_partparser = mypc.m_Bx_particle_parser->compile<4>();
        m_Byfield_partparser = mypc.m_By_particle_parser->compile<4>();
        m_Bzfield_partparser = mypc.m_Bz_particle_parser->compile<4>();
    }

    if (cfg.E_ext_particle_s() == "repeated_plasma_lens" ||
        cfg.B_ext_particle_s() == "repeated_plasma_lens")
    {
        if (cfg.E_ext_particle_s() == "repeated_plasma_lens") { m_Etype = RepeatedPlasmaLens; }
        if (cfg.B_ext_particle_s() == "repeated_plasma_lens") { m_Btype = RepeatedPlasmaLens; }
        m_dt = d.dt(a_pti.GetLevel());
        const auto& attribs = a_pti.GetAttribs();
        m_ux = attribs[PIdx::ux].dataPtr() + a_offset;
        m_uy = attribs[PIdx::uy].dataPtr() + a_offset;
        m_uz = attribs[PIdx::uz].dataPtr() + a_offset;
        m_repeated_plasma_lens_period = mypc.m_repeated_plasma_lens_period;
        m_n_lenses = static_cast<int>(mypc.h_repeated_plasma_lens_starts.size());
        m_repeated_plasma_lens_starts = mypc.d_repeated_plasma_lens_starts.data();
        m_repeated_plasma_lens_lengths = mypc.d_repeated_plasma_lens_lengths.data();
        m_repeated_plasma_lens_strengths_E = mypc.d_repeated_plasma_lens_strengths_E.data();
        m_repeated_plasma_lens_strengths_B = mypc.d_repeated_plasma_lens_strengths_B.data();
    }

    // When the external particle fields are read from file,
    // the external fields are not added directly inside the gather kernel.
    // (Hence of `None`, which ensures that the gather kernel is compiled without support
    // for external fields.) Instead, the external fields are added to the MultiFab
    // Efield_aux and Bfield_aux before the particles gather from these MultiFab.
    if (cfg.E_ext_particle_s() == "read_from_file") {
        m_Etype = ExternalFieldInitType::None;
    }
    if (cfg.B_ext_particle_s() == "read_from_file") {
        m_Btype = ExternalFieldInitType::None;
    }

    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_Etype != Unknown, "Unknown E_ext_particle_init_style");
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(m_Btype != Unknown, "Unknown B_ext_particle_init_style");

}

// The core only knows the generic iterator; this is the one concrete
// instantiation the built-in plugins use.
template GetExternalEBField::GetExternalEBField<ParticleTileIter> (
    const ParticleTileIter&, long, const warpx::runtime::RuntimeContext*) noexcept;
