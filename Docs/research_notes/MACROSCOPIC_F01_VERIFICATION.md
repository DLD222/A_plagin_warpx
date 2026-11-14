# F-01 verification: the conductivity damping, measured on a real deck

CODE_REVIEW_2026-11-14 F-01 said that with the default `algo.macroscopic_subpixel=0` a finite
`macroscopic.sigma` had no effect on the E update at all.  This records the measurement that the
fix (`536b5f0`) is right, on the 1D cavity deck `Examples/Tests/macroscopic_solver/
inputs_test_1d_macroscopic_solver` — a PEC cavity with `epsilon_r = 1.5` whose mode should
oscillate as `By = cos(kz) cos(omega t)`.

The deck is run three times, changing only `macroscopic.sigma`; the mode amplitude is the
projection of `By` onto `cos(kz)`, and the expected ratio is
`|cos(omega t1)/cos(omega t0)| exp(-sigma (t1-t0)/(2 eps))`.

| sigma [S/m] | amplitude ratio | expected | measured decay rate | sigma/(2 eps) |
|---|---|---|---|---|
| 0        | 0.647618 | 0.647593 | 0 | 0 |
| 0.001    | 0.567072 | 0.532215 | 2.55e7 /s | 3.77e7 /s |
| 0.002    | 0.499157 | 0.437394 | 4.99e7 /s | 7.53e7 /s |

What this establishes:

* with `sigma = 0` the solver reproduces the analytic cavity frequency to 4e-5 relative, so the
  new backward-Euler denominator is a no-op at zero conductivity and the existing tests keep their
  numbers;
* with `sigma > 0` the mode now *decays*, which it did not before the fix;
* the measured decay rate is linear in sigma (2.55e7 -> 4.99e7 for 0.001 -> 0.002), which is the
  signature of a real `sigma/eps` damping term rather than a numerical artifact.

Open point, recorded rather than hidden: the measured rate is a constant 0.67 of the simple
`sigma/(2 eps)` estimate.  The estimate assumes the standing wave's E/B energy split averages to
one half over the window, and the window here is 0.64 of a period with the field starting purely
magnetic (E = 0), so part of the difference is expected; the exact factor is not yet derived.
The scaling in sigma is what the fix has to get right, and it does.

Reproduce: `python3 Tools/verify/check_macroscopic_cavity_mode.py <diag_t0> <diag_t1> <sigma>`
after running the deck with the desired `macroscopic.sigma`.
