/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WARPX_GRAVITYTESTREFERENCE_H_
#define WARPX_GRAVITYTESTREFERENCE_H_

#include <AMReX_Array.H>
#include <AMReX_BoxArray.H>
#include <AMReX_Geometry.H>
#include <AMReX_MultiFab.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

/** Analytic reference for the Newton-gravity acceptance tests.
 *
 * Every expression below is the C++ transcription of Tools/star/gravity_reference.py, which
 * carries the same formulas plus self-checks against identities that hold independently of this
 * plugin (Gauss's law at the sphere surface, the shell theorem, continuity of phi at a shell
 * boundary, and the point-mass limit).  Specification: PHYSICS_SPEC.md section 1.4.
 *
 * The formulas are for three dimensions only, which is why the tests using this header are
 * registered for WARPX_DIMS=3.
 */
namespace warpx::plugins::gravity::test
{

constexpr amrex::Real G_newton = 6.67430e-11;   // CODATA 2018
constexpr amrex::Real pi = 3.14159265358979323846;

AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real massUniformSphere (amrex::Real radius, amrex::Real density)
{
    return amrex::Real(4.0 / 3.0) * pi * density * radius * radius * radius;
}

/** phi = -GM/(2R) (3 - (r/R)^2) inside, -GM/r outside; zero at infinity. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real phiUniformSphere (amrex::Real r, amrex::Real radius, amrex::Real density)
{
    amrex::Real const mass = massUniformSphere(radius, density);
    if (r <= radius) {
        return -G_newton * mass / (amrex::Real(2.0) * radius)
             * (amrex::Real(3.0) - (r / radius) * (r / radius));
    }
    return -G_newton * mass / r;
}

/** |g| inside g = (4/3) pi G rho r, outside g = G M / r^2. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real gUniformSphere (amrex::Real r, amrex::Real radius, amrex::Real density)
{
    if (r <= radius) {
        return amrex::Real(4.0 / 3.0) * pi * G_newton * density * r;
    }
    return G_newton * massUniformSphere(radius, density) / (r * r);
}

/** phi = -G M / r, the r -> inf limit of the sphere. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real phiPointMass (amrex::Real r, amrex::Real mass)
{
    return -G_newton * mass / r;
}

/** Potential of one uniform shell r_in <= r <= r_out, zero at infinity.
 *
 * phi = -G [ integral_r^inf M_enc(s)/s^2 ds ] with M_enc the mass interior to s.  The three
 * branches below are that integral evaluated analytically (see gravity_reference.py). */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real phiShell (amrex::Real r, amrex::Real r_in, amrex::Real r_out, amrex::Real density)
{
    amrex::Real const mass = amrex::Real(4.0 / 3.0) * pi * density
                           * (r_out * r_out * r_out - r_in * r_in * r_in);
    if (r >= r_out) {
        return -G_newton * mass / r;
    }
    if (r >= r_in) {
        amrex::Real const partial = amrex::Real(4.0 / 3.0) * pi * density
            * (amrex::Real(0.5) * (r_out * r_out - r * r)
               - r_in * r_in * r_in * (r_out - r) / (r * r_out));
        return -G_newton * (partial + mass / r_out);
    }
    amrex::Real const partial = amrex::Real(4.0 / 3.0) * pi * density
        * (amrex::Real(0.5) * (r_out * r_out - r_in * r_in)
           - r_in * r_in * r_in * (r_out - r_in) / (r_in * r_out));
    return -G_newton * (partial + mass / r_out);
}

/** |g| of one uniform shell: zero inside, G M_enc(r)/r^2 in the shell, G M/r^2 outside. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real gShell (amrex::Real r, amrex::Real r_in, amrex::Real r_out, amrex::Real density)
{
    if (r >= r_out) {
        amrex::Real const mass = amrex::Real(4.0 / 3.0) * pi * density
                               * (r_out * r_out * r_out - r_in * r_in * r_in);
        return G_newton * mass / (r * r);
    }
    if (r >= r_in) {
        return G_newton * amrex::Real(4.0 / 3.0) * pi * density
             * (r * r * r - r_in * r_in * r_in) / (r * r);
    }
    return amrex::Real(0.0);
}

/** Fraction of a cube of side dx centred at (x,y,z) that lies inside a sphere of radius R.
 *
 * The uniform density is represented with these fractions so that the discrete mass (and the
 * total monopole) is exact to a few parts in 10^-5; a cell-centre indicator, by contrast, is
 * wrong by O(dx/R) and would swamp the solver error the test is meant to measure.
 * Midpoint sampling with 6^3 sub-points per cell was measured (Tools/star/gravity_reference.py
 * notes) to give a relative mass error of 5e-5 for R = 24 dx; the volume error contributes far
 * below the second-order solver error. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE
amrex::Real sphereVolumeFraction (amrex::Real x, amrex::Real y, amrex::Real z,
                                  amrex::Real dx, amrex::Real cx, amrex::Real cy,
                                  amrex::Real cz, amrex::Real radius)
{
    // Cells farther than half a diagonal from the surface are wholly inside or outside; that
    // shortcut matters because the 12^3 sub-sampling is only needed in the boundary layer.
    amrex::Real const half_diagonal = amrex::Real(0.5) * std::sqrt(amrex::Real(3.0)) * dx;
    amrex::Real const dxc = x - cx;
    amrex::Real const dyc = y - cy;
    amrex::Real const dzc = z - cz;
    amrex::Real const r2 = dxc * dxc + dyc * dyc + dzc * dzc;
    if (r2 > (radius + half_diagonal) * (radius + half_diagonal)) { return amrex::Real(0.0); }
    if (r2 < (radius - half_diagonal) * (radius - half_diagonal)) { return amrex::Real(1.0); }

    constexpr int n_sub = 12;
    amrex::Real const h = dx / amrex::Real(n_sub);
    int inside = 0;
    for (int a = 0; a < n_sub; ++a) {
        for (int b = 0; b < n_sub; ++b) {
            for (int c = 0; c < n_sub; ++c) {
                // The cell centre x is at the middle of the cell, so the sub-cell samples
                // start half a cell below it; leaving that out shifts the digitised mass by
                // half a cell and adds a dipole the solver then reproduces faithfully.
                amrex::Real const ox = x - cx - amrex::Real(0.5) * dx;
                amrex::Real const oy = y - cy - amrex::Real(0.5) * dx;
                amrex::Real const oz = z - cz - amrex::Real(0.5) * dx;
                amrex::Real const px = ox + (amrex::Real(a) + amrex::Real(0.5)) * h;
                amrex::Real const py = oy + (amrex::Real(b) + amrex::Real(0.5)) * h;
                amrex::Real const pz = oz + (amrex::Real(c) + amrex::Real(0.5)) * h;
                inside += (px * px + py * py + pz * pz <= radius * radius) ? 1 : 0;
            }
        }
    }
    return amrex::Real(inside) / amrex::Real(n_sub * n_sub * n_sub);
}

/** A periodic-free cube domain [-L/2, L/2]^3 with n cells per direction, cell-centred. */
inline amrex::Geometry makeCube (amrex::Real side, int n_cell,
                                 amrex::BoxArray& ba, amrex::DistributionMapping& dm)
{
    amrex::IntVect const lo(0);
    amrex::IntVect const hi(n_cell - 1);
    ba.define(amrex::Box(lo, hi));
    dm.define(ba);
    amrex::Real const half = amrex::Real(0.5) * side;
    amrex::RealBox rb({AMREX_D_DECL(-half, -half, -half)}, {AMREX_D_DECL(half, half, half)});
    return amrex::Geometry(amrex::Box(lo, hi), rb, amrex::CoordSys::cartesian,
                           {AMREX_D_DECL(0, 0, 0)});
}

struct ErrorStats
{
    amrex::Real max_rel = 0.0;   // largest relative error over the sampled cells
    amrex::Real rms_rel = 0.0;   // root-mean-square relative error
    amrex::Real max_abs = 0.0;   // largest absolute error (useful where the reference vanishes)
    long count = 0;
};

/** Comparison geometry: sample cells whose centre radius falls in one of `bands`, skip cells
 *  within `surface_margin` of any radius in `surfaces` (the density is discontinuous there and
 *  the pointwise field error is dominated by the interface, not by the solver), and skip cells
 *  within `boundary_margin` of the domain face (the Dirichlet data fixes the answer there). */
struct CompareRegion
{
    std::vector<std::pair<amrex::Real, amrex::Real>> bands;
    std::vector<amrex::Real> surfaces;
    amrex::Real surface_margin = 0.0;
    amrex::Real boundary_margin = 0.0;
};

inline bool inRegion (amrex::Real x, amrex::Real y, amrex::Real z, amrex::Real half_side,
                      CompareRegion const& region)
{
    // Keep away from the domain faces: there the Dirichlet data, not the solver, fixes phi.
    amrex::Real const face_distance = half_side
        - std::max(std::abs(x), std::max(std::abs(y), std::abs(z)));
    if (face_distance < region.boundary_margin) { return false; }
    amrex::Real const r = std::sqrt(x * x + y * y + z * z);
    for (amrex::Real s : region.surfaces) {
        if (std::abs(r - s) < region.surface_margin) { return false; }
    }
    for (auto const& band : region.bands) {
        if (r >= band.first && r <= band.second) { return true; }
    }
    return false;
}

/** Relative error of the numerical |g| against g_ref(r) over the compare region. */
inline ErrorStats compareAcceleration (amrex::MultiFab const& gx, amrex::MultiFab const& gy,
                                       amrex::MultiFab const& gz, amrex::Geometry const& geom,
                                       std::function<amrex::Real(amrex::Real)> const& g_ref,
                                       CompareRegion const& region)
{
    ErrorStats stats;
    amrex::Real sum_sq = 0.0;
    auto const dx = geom.CellSizeArray();
    auto const prob_lo = geom.ProbLoArray();
    auto const prob_hi = geom.ProbHiArray();
    amrex::Real const half_side = amrex::Real(0.5) * (prob_hi[0] - prob_lo[0]);

    for (amrex::MFIter mfi(gx); mfi.isValid(); ++mfi)
    {
        amrex::Box const& bx = mfi.validbox();
        auto const gxa = gx.const_array(mfi);
        auto const gya = gy.const_array(mfi);
        auto const gza = gz.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    amrex::Real const x = prob_lo[0] + (amrex::Real(i) + amrex::Real(0.5)) * dx[0];
                    amrex::Real const y = prob_lo[1] + (amrex::Real(j) + amrex::Real(0.5)) * dx[1];
                    amrex::Real const z = prob_lo[2] + (amrex::Real(k) + amrex::Real(0.5)) * dx[2];
                    if (!inRegion(x, y, z, half_side, region)) { continue; }
                    amrex::Real const r = std::sqrt(x * x + y * y + z * z);
                    amrex::Real const g_num = std::sqrt(
                        gxa(i, j, k) * gxa(i, j, k) + gya(i, j, k) * gya(i, j, k)
                        + gza(i, j, k) * gza(i, j, k));
                    amrex::Real const g_exact = g_ref(r);
                    amrex::Real const err = std::abs(g_num - g_exact);
                    amrex::Real const denom = std::max(std::abs(g_exact), amrex::Real(1.e-300));
                    stats.max_rel = std::max(stats.max_rel, err / denom);
                    stats.max_abs = std::max(stats.max_abs, err);
                    sum_sq += (err / denom) * (err / denom);
                    ++stats.count;
                }
            }
        }
    }
    if (stats.count > 0) {
        stats.rms_rel = std::sqrt(sum_sq / amrex::Real(stats.count));
    }
    return stats;
}

/** Relative error of the numerical phi against phi_ref(r) over the compare region. */
inline ErrorStats comparePotential (amrex::MultiFab const& phi, amrex::Geometry const& geom,
                                    std::function<amrex::Real(amrex::Real)> const& phi_ref,
                                    CompareRegion const& region)
{
    ErrorStats stats;
    amrex::Real sum_sq = 0.0;
    auto const dx = geom.CellSizeArray();
    auto const prob_lo = geom.ProbLoArray();
    auto const prob_hi = geom.ProbHiArray();
    amrex::Real const half_side = amrex::Real(0.5) * (prob_hi[0] - prob_lo[0]);

    for (amrex::MFIter mfi(phi); mfi.isValid(); ++mfi)
    {
        amrex::Box const& bx = mfi.validbox();
        auto const pa = phi.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    amrex::Real const x = prob_lo[0] + (amrex::Real(i) + amrex::Real(0.5)) * dx[0];
                    amrex::Real const y = prob_lo[1] + (amrex::Real(j) + amrex::Real(0.5)) * dx[1];
                    amrex::Real const z = prob_lo[2] + (amrex::Real(k) + amrex::Real(0.5)) * dx[2];
                    if (!inRegion(x, y, z, half_side, region)) { continue; }
                    amrex::Real const r = std::sqrt(x * x + y * y + z * z);
                    amrex::Real const exact = phi_ref(r);
                    amrex::Real const err = std::abs(pa(i, j, k) - exact);
                    amrex::Real const denom = std::max(std::abs(exact), amrex::Real(1.e-300));
                    stats.max_rel = std::max(stats.max_rel, err / denom);
                    stats.max_abs = std::max(stats.max_abs, err);
                    sum_sq += (err / denom) * (err / denom);
                    ++stats.count;
                }
            }
        }
    }
    if (stats.count > 0) {
        stats.rms_rel = std::sqrt(sum_sq / amrex::Real(stats.count));
    }
    return stats;
}

/** Largest |g| over the compare region.  Used where the reference field is exactly zero (the
 *  hollow core of a shell), so that a relative error is meaningless and an absolute scale is
 *  compared instead. */
inline amrex::Real maxAccelerationMagnitude (amrex::MultiFab const& gx, amrex::MultiFab const& gy,
                                             amrex::MultiFab const& gz, amrex::Geometry const& geom,
                                             CompareRegion const& region)
{
    amrex::Real max_g = 0.0;
    auto const dx = geom.CellSizeArray();
    auto const prob_lo = geom.ProbLoArray();
    auto const prob_hi = geom.ProbHiArray();
    amrex::Real const half_side = amrex::Real(0.5) * (prob_hi[0] - prob_lo[0]);

    for (amrex::MFIter mfi(gx); mfi.isValid(); ++mfi)
    {
        amrex::Box const& bx = mfi.validbox();
        auto const gxa = gx.const_array(mfi);
        auto const gya = gy.const_array(mfi);
        auto const gza = gz.const_array(mfi);
        for (int i = bx.smallEnd(0); i <= bx.bigEnd(0); ++i) {
            for (int j = bx.smallEnd(1); j <= bx.bigEnd(1); ++j) {
                for (int k = bx.smallEnd(2); k <= bx.bigEnd(2); ++k) {
                    amrex::Real const x = prob_lo[0] + (amrex::Real(i) + amrex::Real(0.5)) * dx[0];
                    amrex::Real const y = prob_lo[1] + (amrex::Real(j) + amrex::Real(0.5)) * dx[1];
                    amrex::Real const z = prob_lo[2] + (amrex::Real(k) + amrex::Real(0.5)) * dx[2];
                    if (!inRegion(x, y, z, half_side, region)) { continue; }
                    max_g = std::max(max_g, std::sqrt(
                        gxa(i, j, k) * gxa(i, j, k) + gya(i, j, k) * gya(i, j, k)
                        + gza(i, j, k) * gza(i, j, k)));
                }
            }
        }
    }
    return max_g;
}

/** Observed convergence order between two grids, given errors e1 (coarse) and e2 (fine) and the
 *  cell-size ratio. Returns 0 when the fine error is zero. */
inline amrex::Real convergenceOrder (amrex::Real e_coarse, amrex::Real e_fine, amrex::Real ratio)
{
    if (e_fine <= 0.0) { return 0.0; }
    return std::log(e_coarse / e_fine) / std::log(ratio);
}

} // namespace warpx::plugins::gravity::test

#endif // WARPX_GRAVITYTESTREFERENCE_H_
