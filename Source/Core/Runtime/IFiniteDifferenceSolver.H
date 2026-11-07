#ifndef WARPX_RUNTIME_IFINITEDIFFERENCESOLVER_H_
#define WARPX_RUNTIME_IFINITEDIFFERENCESOLVER_H_
#include "Utils/WarpXAlgorithmSelection.H"
#include <ablastr/fields/MultiFabRegister.H>
#include <ablastr/utils/Enums.H>
#include <AMReX_Array.H>
#include <AMReX_Box.H>
#include <AMReX_IntVect.H>
#include <AMReX_MultiFab.H>
#include <AMReX_iMultiFab.H>
#include <array>
#include <memory>

class FaceInfoBox;
class MultiSigmaBox;
class MacroscopicProperties;
class HybridPICModel;

namespace warpx::runtime {

/** Field-solver plugin contract for finite-difference Maxwell updates (A7.3.1b). */
class IFiniteDifferenceSolver
{
public:
    virtual ~IFiniteDifferenceSolver () = default;

    virtual void EvolveB (ablastr::fields::MultiFabRegister& fields, int lev,
                         ablastr::utils::enums::PatchType patch_type,
                         std::array<std::unique_ptr<amrex::iMultiFab>,3>& flag_info_cell,
                         std::array<std::unique_ptr<amrex::LayoutData<FaceInfoBox>>,3>& borrowing,
                         amrex::Real dt) = 0;
    virtual void EvolveE (ablastr::fields::MultiFabRegister& fields, int lev,
                         ablastr::utils::enums::PatchType patch_type,
                         ablastr::fields::VectorField const& Efield,
                         std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E,
                         amrex::Real dt) = 0;
    virtual void EvolveF (amrex::MultiFab* Ffield, ablastr::fields::VectorField const& Efield,
                         amrex::MultiFab* rhofield, int rho_comp, amrex::Real dt) = 0;
    virtual void EvolveG (amrex::MultiFab* Gfield, ablastr::fields::VectorField const& Bfield,
                         amrex::Real dt) = 0;
    virtual void EvolveECTRho (ablastr::fields::VectorField const& Efield,
                              ablastr::fields::VectorField const& edge_lengths,
                              ablastr::fields::VectorField const& face_areas,
                              ablastr::fields::VectorField const& ECTRhofield, int lev) = 0;
    virtual void ApplySilverMuellerBoundary (
        ablastr::fields::VectorField& Efield, ablastr::fields::VectorField& Bfield,
        amrex::Box domain_box, amrex::Real dt,
        amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> lo,
        amrex::Array<FieldBoundaryType, AMREX_SPACEDIM> hi) = 0;
    virtual void ComputeDivE (ablastr::fields::VectorField const& Efield,
                             amrex::MultiFab& divE) = 0;
    virtual void MacroscopicEvolveE (
        ablastr::fields::MultiFabRegister& fields, MacroscopicSolverAlgo algo,
        ablastr::fields::VectorField const& Efield,
        ablastr::fields::VectorField const& Bfield,
        ablastr::fields::VectorField const& Jfield,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E,
        amrex::Real dt, std::unique_ptr<MacroscopicProperties> const& props) = 0;
    virtual void EvolveBPML (ablastr::fields::MultiFabRegister& fields,
                            ablastr::utils::enums::PatchType patch_type,
                            int level, amrex::Real dt, bool dive_cleaning) = 0;
    virtual void EvolveEPML (ablastr::fields::MultiFabRegister& fields,
                            ablastr::utils::enums::PatchType patch_type, int level,
                            std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E,
                            MultiSigmaBox const& sigba, amrex::Real dt,
                            bool pml_has_particles) = 0;
    virtual void EvolveFPML (amrex::MultiFab* Ffield, ablastr::fields::VectorField Efield,
                            amrex::Real dt) = 0;
    virtual void HybridPICSolveE (
        ablastr::fields::VectorField const& Efield, ablastr::fields::VectorField& Jfield,
        ablastr::fields::VectorField const& Jifield,
        ablastr::fields::VectorField const& Bfield,
        amrex::MultiFab const& rhofield, amrex::MultiFab const& Pefield,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E,
        int lev, HybridPICModel const* hybrid_model, bool solve_for_Faraday) = 0;
    virtual void CalculateCurrentAmpere (
        ablastr::fields::VectorField& Jfield, ablastr::fields::VectorField const& Bfield,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev) = 0;
    virtual void ComputeCurlA (
        ablastr::fields::VectorField& Bfield, ablastr::fields::VectorField const& Afield,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_B, int lev,
        amrex::IntVect const& ngrow = amrex::IntVect::TheZeroVector()) = 0;
    virtual void ComputeCurlB (
        ablastr::fields::VectorField& Efield, ablastr::fields::VectorField const& Bfield,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update_E, int lev,
        amrex::IntVect const& ngrow = amrex::IntVect::TheZeroVector()) = 0;
    virtual void ComputeGradient (ablastr::fields::VectorField& out_field,
                                 ablastr::fields::ScalarField const& in_field,
                                 std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update,
                                 int lev) = 0;
    virtual void ComputeLaplacian (ablastr::fields::ScalarField& out_field,
                                  ablastr::fields::ScalarField const& in_field,
                                  std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update,
                                  int lev) = 0;
    virtual void ComputeVectorLaplacian (
        ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev) = 0;
    virtual void ComputeVectorBiLaplacian (
        ablastr::fields::VectorField& out_field, ablastr::fields::VectorField const& in_field,
        std::array<std::unique_ptr<amrex::iMultiFab>,3> const& eb_update, int lev) = 0;
};

} // namespace warpx::runtime
#endif
