/* Copyright 2019 Andrew Myers, Aurore Blelly, Axel Huebl
 * Maxence Thevenet, Remi Lehe, Weiqun Zhang
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#ifndef WARPX_CORE_MULTISIGMABOX_H_
#define WARPX_CORE_MULTISIGMABOX_H_

#include <AMReX_Box.H>
#include <AMReX_BoxArray.H>
#include <AMReX_Config.H>
#include <AMReX_FabArray.H>
#include <AMReX_FabFactory.H>
#include <AMReX_GpuContainers.H>
#include <AMReX_IntVect.H>
#include <AMReX_REAL.H>

#include <array>

namespace warpx::pml
{
struct Sigma : amrex::Gpu::DeviceVector<amrex::Real>
{
    [[nodiscard]] int lo() const { return m_lo; }
    [[nodiscard]] int hi() const { return m_hi; }
    int m_lo, m_hi;
};

struct SigmaBox
{
    SigmaBox (const amrex::Box& box, const amrex::BoxArray& grids,
              const amrex::Real* dx, const amrex::IntVect& ncell, const amrex::IntVect& delta,
              const amrex::Box& regdomain, amrex::Real v_sigma);

    void define_single (const amrex::Box& regdomain, const amrex::IntVect& ncell,
                        const amrex::Array<amrex::Real,AMREX_SPACEDIM>& fac,
                        amrex::Real v_sigma);
    void define_multiple (const amrex::Box& box, const amrex::BoxArray& grids,
                          const amrex::IntVect& ncell,
                          const amrex::Array<amrex::Real,AMREX_SPACEDIM>& fac,
                          amrex::Real v_sigma);

    void ComputePMLFactorsB (const amrex::Real* dx, amrex::Real dt);
    void ComputePMLFactorsE (const amrex::Real* dx, amrex::Real dt);

    using SigmaVect = std::array<Sigma,AMREX_SPACEDIM>;

    using value_type = void; // needed by amrex::FabArray

    SigmaVect sigma;
    SigmaVect sigma_cumsum;
    SigmaVect sigma_star;
    SigmaVect sigma_star_cumsum;
    SigmaVect sigma_fac;
    SigmaVect sigma_cumsum_fac;
    SigmaVect sigma_star_fac;
    SigmaVect sigma_star_cumsum_fac;
    amrex::Real v_sigma;

};

class SigmaBoxFactory
    : public amrex::FabFactory<SigmaBox>
{
public:
    SigmaBoxFactory (const amrex::BoxArray* grid_ba, const amrex::Real* dx,
                     const amrex::IntVect& ncell, const amrex::IntVect& delta,
                     const amrex::Box& regular_domain, const amrex::Real v_sigma_sb)
        : m_grids{grid_ba}, m_dx(dx), m_ncell(ncell), m_delta(delta), m_regdomain(regular_domain), m_v_sigma_sb(v_sigma_sb) {}
    ~SigmaBoxFactory () override = default;

    SigmaBoxFactory (const SigmaBoxFactory&) = default;
    SigmaBoxFactory (SigmaBoxFactory&&) noexcept = default;

    SigmaBoxFactory () = delete;
    SigmaBoxFactory& operator= (const SigmaBoxFactory&) = delete;
    SigmaBoxFactory& operator= (SigmaBoxFactory&&) = delete;

    [[nodiscard]] SigmaBox* create (const amrex::Box& box, int /*ncomps*/,
        const amrex::FabInfo& /*info*/, int /*box_index*/) const final
    {
        return new SigmaBox(box, *m_grids, m_dx, m_ncell, m_delta, m_regdomain, m_v_sigma_sb);
    }

    void destroy (SigmaBox* fab) const final
    {
        delete fab;
    }

    [[nodiscard]] SigmaBoxFactory*
    clone () const final
    {
        return new SigmaBoxFactory(*this);
    }

private:
    const amrex::BoxArray* m_grids;
    const amrex::Real* m_dx;
    amrex::IntVect m_ncell;
    amrex::IntVect m_delta;
    amrex::Box m_regdomain;
    amrex::Real m_v_sigma_sb;
};

class MultiSigmaBox
    : public amrex::FabArray<SigmaBox>
{
public:
    MultiSigmaBox(const amrex::BoxArray& ba, const amrex::DistributionMapping& dm,
                  const amrex::BoxArray* grid_ba, const amrex::Real* dx,
                  const amrex::IntVect& ncell, const amrex::IntVect& delta,
                  const amrex::Box& regular_domain, amrex::Real v_sigma_sb);
    void ComputePMLFactorsB (const amrex::Real* dx, amrex::Real dt);
    void ComputePMLFactorsE (const amrex::Real* dx, amrex::Real dt);
private:
    amrex::Real dt_B = -1.e10;
    amrex::Real dt_E = -1.e10;
};

} // namespace warpx::pml

#endif // WARPX_CORE_MULTISIGMABOX_H_
