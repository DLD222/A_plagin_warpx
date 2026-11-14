/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Acceptance test: the CF88 reaction rates.
 *
 * Three things are checked, in increasing order of strength:
 *
 * 1. Positivity over the fits' temperature range, and monotone growth inside the solar window
 *    (the exceptions are properties of the published fits, not of this code: the 7Be electron
 *    capture decreases with T9, and the Gaussian factors exp(-(T9/T0)^2) bend several fits over
 *    above T0).
 * 2. Agreement with the independent transcription in Tools/star/cf88_reference.py to 1e-12 at
 *    three temperatures.  The two were written separately from the same table in
 *    PHYSICS_SPEC section 3.3, so this catches a typo in either.
 * 3. A magnitude anchor that no typo survives: the energy generation implied by the p+p rate at
 *    the standard solar centre (T, rho and X taken from the SSM profile file, so nothing is
 *    quoted from memory) must be within a factor of two of the tabulated epsilon there.
 *
 * Specification: PHYSICS_SPEC.md sections 3.3 and 3.3b.
 */
#include "Cf88Rates.H"
#include "FluidTestReference.H"

#include <AMReX.H>
#include <AMReX_REAL.H>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace amrex;
namespace cf88 = warpx::plugins::fluid::cf88;

int main (int argc, char* argv[])
{
    Initialize(argc, argv);

    // Column order of every pinned row below: f2, f4, f18, f24, f41, f42, f61, f64, f71, f76,
    // f78 — the order of the table in PHYSICS_SPEC section 3.3.
    std::vector<Real> const t9_pinned{Real(0.02), Real(0.2), Real(1.0)};
    std::vector<std::vector<Real>> const pinned{
        {2.4097236110183356e-19, 0.045259741996354158, 1.8182145536964669e-08,
         2.2566522612109262e-13, 1.2239466700170206e-09, 1.6094350535585492e-10,
         3.8512746643699451e-14, 1.3417419448334509e-13, 2.7278183702255913e-16,
         5.6116498845815412e-15, 7.2295238788488717e-12},
        {5.9132454445794486e-17, 31.131005186833924, 117.80870532930685,
         0.0045189319823818117, 6.0517880623363672e-10, 0.021769042980652548,
         0.0071823620030163297, 0.024141751306466255, 0.0082040465580455312,
         0.030926707923109791, 50.123296951687131},
        {4.3020695477357735e-16, 387.69695631279404, 268649.63415170193, 11.770184978107961,
         5.7964471107219292e-10, 12.563906264273037, 868.22170793508781, 4342.9934389058044,
         122.09444469762892, 618.01863398445903, 3209870.0966713545}};

    std::vector<const char*> const names{"f2", "f4", "f18", "f24", "f41", "f42", "f61", "f64",
                                        "f71", "f76", "f78"};
    using RateFn = Real (*) (Real);
    std::vector<RateFn> const rates{&cf88::f2, &cf88::f4, &cf88::f18, &cf88::f24, &cf88::f41,
                                    &cf88::f42, &cf88::f61, &cf88::f64, &cf88::f71, &cf88::f76,
                                    &cf88::f78};

    int exit_code = 0;

    // 1. Positivity and monotonicity.
    {
        Real const t_lo = Real(0.005);
        Real const t_hi = Real(3.0);
        for (std::size_t r = 0; r < rates.size(); ++r)
        {
            Real previous = rates[r](Real(0.02));
            for (int k = 0; k <= 1000; ++k)
            {
                Real const t9 = t_lo + (t_hi - t_lo) * Real(k) / Real(1000);
                Real const value = rates[r](t9);
                if (!(value > 0.0)) {
                    std::printf(" FAILED: %s is not positive at T9 = %.4f (%.3e)\n",
                                names[r], (double)t9, (double)value);
                    exit_code = 1;
                }
                if (r != 4 && t9 > Real(0.02) && t9 < Real(0.30)) {
                    if (!(value > previous)) {
                        std::printf(" FAILED: %s is not increasing at T9 = %.4f\n",
                                    names[r], (double)t9);
                        exit_code = 1;
                    }
                }
                previous = value;
            }
        }
        if (!(cf88::f41(Real(0.02)) > cf88::f41(Real(0.05)))) {
            std::printf(" FAILED: 7Be electron capture should decrease with T9\n");
            exit_code = 1;
        }
    }

    // 2. Agreement with the independent transcription.
    std::printf("\n CF88 fits against Tools/star/cf88_reference.py\n");
    std::printf(" %6s", "T9");
    for (auto const* n : names) { std::printf(" %9s", n); }
    std::printf("\n");
    for (std::size_t p = 0; p < pinned.size(); ++p)
    {
        std::printf(" %6.2f", (double)t9_pinned[p]);
        for (std::size_t r = 0; r < rates.size(); ++r)
        {
            Real const value = rates[r](t9_pinned[p]);
            Real const reference = pinned[p][r];
            Real const rel = std::abs(value - reference) / std::abs(reference);
            std::printf(" %9.2e", (double)rel);
            if (rel > Real(1.e-12)) {
                std::printf("\n FAILED: %s at T9 = %.2f differs by %.2e\n", names[r],
                            (double)t9_pinned[p], (double)rel);
                exit_code = 1;
            }
        }
        std::printf("\n");
    }

    // 3. Magnitude anchor at the solar centre.  Everything in this block comes from the SSM
    //    profile file Docs/research_notes/star_plugins/data/ssm_S_profile.csv (centre record):
    //    T = 1.563376e7 K, rho = 152.699 g/cm^3, X = 0.3407625.
    {
        Real const t9 = Real(1.563376e7) / Real(1.e9);
        Real const rho = Real(152.699);              // g/cm^3
        Real const x_h = Real(0.3407625);
        Real const m_p = Real(1.67262192e-24);       // g
        Real const n_p = rho * x_h / m_p;
        Real const sigma_v = cf88::f2(t9) / cf88::N_A;
        Real const rate = Real(0.5) * n_p * n_p * sigma_v;      // reactions cm^-3 s^-1
        // 4 p -> 4He + 2 e+ + 2 nu releases 26.73 MeV for two p+p reactions.
        Real const energy_per_pp = Real(26.73) / Real(2.0);     // MeV
        Real const epsilon = energy_per_pp * Real(1.602176634e-6) * rate / rho;
        Real const ssm_epsilon = Real(16.93);        // same point, from the SSM profile file
        std::printf("\n solar centre: T9 = %.6f, rho = %.3f, X = %.5f\n", (double)t9,
                    (double)rho, (double)x_h);
        std::printf("   <sigma v>(p+p) = %.4e cm^3/s\n", (double)sigma_v);
        std::printf("   epsilon from p+p = %.2f erg/g/s   SSM table = %.2f erg/g/s   ratio %.3f\n",
                    (double)epsilon, (double)ssm_epsilon, (double)(epsilon / ssm_epsilon));
        std::printf("   (the SSM value also contains the CNO cycle, electron screening and the\n"
                    "    pp-chain branch ratios; none of them is in this module yet, so the check\n"
                    "    is a factor-of-two magnitude anchor, not a precision test)\n");
        Real const ratio = epsilon / ssm_epsilon;
        if (ratio < Real(0.5) || ratio > Real(2.0)) {
            std::printf(" FAILED: epsilon is %.2f times the tabulated value (allowed 0.5-2)\n",
                        (double)ratio);
            exit_code = 1;
        }
    }

    if (exit_code == 0) { std::printf("test_cf88_rates: ok\n"); }
    Finalize();
    return exit_code;
}
