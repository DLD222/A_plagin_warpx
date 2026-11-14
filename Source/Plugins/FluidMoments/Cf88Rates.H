/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_CF88RATES_H_
#define WARPX_CF88RATES_H_

#include <AMReX_Extension.H>
#include <AMReX_GpuQualifiers.H>
#include <AMReX_REAL.H>

#include <cmath>

/** Caughlan & Fowler (1988) analytic reaction rates, as transcribed in PHYSICS_SPEC section 3.3.
 *
 * Every function here is a *verbatim* transcription of one row of that table; the entry number in
 * the name (f2, f4, ...) is the CF88 entry number.  The fits return
 *
 *     f [ (cm^3/mol)^(N-1) / s ],
 *
 * so a two-body rate becomes an astrophysical S-factor average through
 *
 *     <sigma v> [cm^3/s] = f / N_A,     N_A = 6.02214076e23 /mol        (PHYSICS_SPEC 3.3)
 *
 * with T9 = T / 10^9 K.  The tabulated T9 ceilings are not enforced here: the caller decides
 * where a fit is valid (the network does so through the temperature it is asked about).
 *
 * Source: G. R. Caughlan & W. A. Fowler, Atomic Data and Nuclear Data Tables 40, 283 (1988),
 * table 2, transcribed from <http://www.nuclear.csdb.cn/data/CF88/analyt_rates.html>.
 */
namespace warpx::plugins::fluid::cf88
{

//! Avogadro constant, in the units the fits use (1/mol).
constexpr amrex::Real N_A = 6.02214076e23;

//! Caughlan-Fowler coefficient for the 15N(p,alpha)12C branch (the source's "rc78", default 0.1).
constexpr amrex::Real rc78 = 0.1;

/** Helpers shared by the fits: T9^(1/3), T9^(2/3), T9^(3/2), T9^(4/3), T9^(5/3), T9^(6/5),
 *  T9^(1/2), exactly as the source defines its abbreviations. */
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t913 (amrex::Real t9) noexcept
{ return std::cbrt(t9); }
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t923 (amrex::Real t9) noexcept
{ return t913(t9) * t913(t9); }
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t932 (amrex::Real t9) noexcept
{ return t9 * std::sqrt(t9); }
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t943 (amrex::Real t9) noexcept
{ return t9 * t913(t9); }
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t953 (amrex::Real t9) noexcept
{ return t9 * t923(t9); }
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t965 (amrex::Real t9) noexcept
{ return std::pow(t9, amrex::Real(1.2)); }
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real t912 (amrex::Real t9) noexcept
{ return std::sqrt(t9); }

//! 2: p(p,e+nu)d, Q = 1.442 MeV, fit valid up to T9 = 3.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f2 (amrex::Real t9) noexcept
{
    return amrex::Real(4.01e-15) / t923(t9) * std::exp(amrex::Real(-3.380) / t913(t9))
         * (amrex::Real(1.0) + amrex::Real(0.123) * t913(t9) + amrex::Real(1.09) * t923(t9)
            + amrex::Real(0.938) * t9);
}

//! 4: d(p,gamma)3He, Q = 5.494 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f4 (amrex::Real t9) noexcept
{
    return amrex::Real(2.24e+03) / t923(t9) * std::exp(amrex::Real(-3.720) / t913(t9))
         * (amrex::Real(1.0) + amrex::Real(0.112) * t913(t9) + amrex::Real(3.38) * t923(t9)
            + amrex::Real(2.65) * t9);
}

//! 18: 3He(3He,2p)4He, Q = 12.860 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f18 (amrex::Real t9) noexcept
{
    return amrex::Real(6.04e+10) / t923(t9) * std::exp(amrex::Real(-12.276) / t913(t9))
         * (amrex::Real(1.0) + amrex::Real(0.034) * t913(t9) - amrex::Real(0.522) * t923(t9)
            - amrex::Real(0.124) * t9 + amrex::Real(0.353) * t943(t9)
            + amrex::Real(0.213) * t953(t9));
}

//! 24: 3He(alpha,gamma)7Be, Q = 1.588 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f24 (amrex::Real t9) noexcept
{
    amrex::Real const t9a = t9 / (amrex::Real(1.0) + amrex::Real(4.95e-2) * t9);
    return amrex::Real(5.61e+06) * std::pow(t9a, amrex::Real(5.0) / amrex::Real(6.0)) / t932(t9)
         * std::exp(amrex::Real(-12.826) / std::cbrt(t9a));
}

//! 41: 7Be(e-,nu)7Li, Q = 0.862 MeV, fit valid up to T9 = 3.
//! The source adds two restrictions, both the caller's business: T9 below 0.001 is clamped to
//! 0.001, and the rate must not exceed 1.51e-07/(rho (1+X)/2).
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f41 (amrex::Real t9) noexcept
{
    return amrex::Real(1.34e-10) / t912(t9)
         * (amrex::Real(1.0) - amrex::Real(0.537) * t913(t9) + amrex::Real(3.86) * t923(t9)
            + amrex::Real(0.0027) / t9 * std::exp(amrex::Real(2.515e-3) / t9));
}

//! 42: 7Be(p,gamma)8B, Q = 0.137 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f42 (amrex::Real t9) noexcept
{
    return amrex::Real(3.11e+05) / t923(t9) * std::exp(amrex::Real(-10.262) / t913(t9))
         + amrex::Real(2.53e+03) / t932(t9) * std::exp(amrex::Real(-7.306) / t9);
}

//! 61: 12C(p,gamma)13N, Q = 1.944 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f61 (amrex::Real t9) noexcept
{
    return amrex::Real(2.04e+07) / t923(t9)
         * std::exp(amrex::Real(-13.690) / t913(t9) - std::pow(t9 / amrex::Real(1.500), 2))
         * (amrex::Real(1.00) + amrex::Real(0.030) * t913(t9) + amrex::Real(1.19) * t923(t9)
            + amrex::Real(0.254) * t9 + amrex::Real(2.06) * t943(t9)
            + amrex::Real(1.12) * t953(t9))
         + amrex::Real(1.08e+05) / t932(t9) * std::exp(amrex::Real(-4.925) / t9)
         + amrex::Real(2.15e+05) / t932(t9) * std::exp(amrex::Real(-18.179) / t9);
}

//! 64: 13C(p,gamma)14N, Q = 7.551 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f64 (amrex::Real t9) noexcept
{
    return amrex::Real(8.01e+07) / t923(t9)
         * std::exp(amrex::Real(-13.717) / t913(t9) - std::pow(t9 / amrex::Real(2.000), 2))
         * (amrex::Real(1.0) + amrex::Real(0.030) * t913(t9) + amrex::Real(0.958) * t923(t9)
            + amrex::Real(0.204) * t9 + amrex::Real(1.39) * t943(t9)
            + amrex::Real(0.753) * t953(t9))
         + amrex::Real(1.21e+06) / t965(t9) * std::exp(amrex::Real(-5.701) / t9);
}

//! 71: 14N(p,gamma)15O, Q = 7.297 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f71 (amrex::Real t9) noexcept
{
    return amrex::Real(4.90e+07) / t923(t9)
         * std::exp(amrex::Real(-15.228) / t913(t9) - std::pow(t9 / amrex::Real(3.294), 2))
         * (amrex::Real(1.0) + amrex::Real(0.027) * t913(t9) - amrex::Real(0.778) * t923(t9)
            - amrex::Real(0.149) * t9 + amrex::Real(0.261) * t943(t9)
            + amrex::Real(0.127) * t953(t9))
         + amrex::Real(2.37e+03) / t932(t9) * std::exp(amrex::Real(-3.011) / t9)
         + amrex::Real(2.19e+04) * std::exp(amrex::Real(-12.530) / t9);
}

//! 76: 15N(p,gamma)16O, Q = 12.128 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f76 (amrex::Real t9) noexcept
{
    return amrex::Real(9.78e+08) / t923(t9)
         * std::exp(amrex::Real(-15.251) / t913(t9) - std::pow(t9 / amrex::Real(0.450), 2))
         * (amrex::Real(1.00) + amrex::Real(0.027) * t913(t9) + amrex::Real(0.219) * t923(t9)
            + amrex::Real(0.042) * t9 + amrex::Real(6.83) * t943(t9)
            + amrex::Real(3.32) * t953(t9))
         + amrex::Real(1.11e+04) / t932(t9) * std::exp(amrex::Real(-3.328) / t9)
         + amrex::Real(1.49e+04) / t932(t9) * std::exp(amrex::Real(-4.665) / t9)
         + amrex::Real(3.80e+06) / t932(t9) * std::exp(amrex::Real(-11.048) / t9);
}

//! 78: 15N(p,alpha)12C, Q = 4.966 MeV.
AMREX_GPU_HOST_DEVICE AMREX_FORCE_INLINE amrex::Real f78 (amrex::Real t9) noexcept
{
    return amrex::Real(1.08e+12) / t923(t9)
         * std::exp(amrex::Real(-15.251) / t913(t9) - std::pow(t9 / amrex::Real(0.522), 2))
         * (amrex::Real(1.00) + amrex::Real(0.027) * t913(t9) + amrex::Real(2.62) * t923(t9)
            + amrex::Real(0.501) * t9 + amrex::Real(5.36) * t943(t9)
            + amrex::Real(2.60) * t953(t9))
         + amrex::Real(1.19e+08) / t932(t9) * std::exp(amrex::Real(-3.676) / t9)
         + amrex::Real(5.41e+08) / t912(t9) * std::exp(amrex::Real(-8.926) / t9)
         + rc78 * amrex::Real(4.72e+08) / t932(t9) * std::exp(amrex::Real(-7.721) / t9)
         + amrex::Real(2.20e+09) / t932(t9) * std::exp(amrex::Real(-11.418) / t9);
}

/** Q values [MeV] from the same table (column 3). */
namespace q_value
{
constexpr amrex::Real r2 = 1.442;
constexpr amrex::Real r4 = 5.494;
constexpr amrex::Real r18 = 12.860;
constexpr amrex::Real r24 = 1.588;
constexpr amrex::Real r41 = 0.862;
constexpr amrex::Real r42 = 0.137;
constexpr amrex::Real r61 = 1.944;
constexpr amrex::Real r64 = 7.551;
constexpr amrex::Real r71 = 7.297;
constexpr amrex::Real r76 = 12.128;
constexpr amrex::Real r78 = 4.966;
} // namespace q_value

} // namespace warpx::plugins::fluid::cf88

#endif // WARPX_CF88RATES_H_
