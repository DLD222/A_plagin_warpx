# Stellar-physics tools for the star plugins

These scripts are the analysis and validation layer for the two star plugins in
`Source/Plugins/NewtonGravity` and `Source/Plugins/FluidMoments`. They are also what the
comparisons against the standard solar model (Christensen-Dalsgaard) are run with.
The physics, the sources of every constant and the recorded failures live in
[docs/research_notes/star_plugins/PHYSICS_SPEC.md](../../Docs/research_notes/star_plugins/PHYSICS_SPEC.md).

Everything below runs with a plain `python3` plus numpy and matplotlib.

## What each file does

| File | Role |
|---|---|
| `gravity_reference.py` | Analytic potentials and fields for the gravity tests (uniform sphere, point mass, two shells). `--selftest` |
| `sod_reference.py` | Independent Toro chapter-4 star state for the Sod shock test. `--selftest` |
| `cf88_reference.py` | Independent transcription of the eleven CF88 rate fits. `--selftest`, `--table`, `--pin` |
| `ssm_compare.py` | Layer-by-layer comparison of our `epsilon` and `kappa` against the SSM profile. `--selftest` |
| `opal_opacity.py` | Reader and interpolator for the tabulated OPAL subset. `--selftest` |
| `extract_opal_subset.py` | Rebuilds that subset from an `rm-tables` archive (records provenance) |
| `eos.py` | Saha ionisation EOS: pressure, internal energy, adiabatic gradient, density inversion. `--selftest` |
| `stellar_structure.py` | Lane-Emden cross-check, the four structure equations, the mixing-length gradient and the photospheric fit. `--selftest`, `--diagnostic` |
| `solar_compare_plot.py` | The five-panel comparison figure and the model regression baseline. `--check` |

## Reproducing the comparisons

```bash
# every check that is also a ctest
python3 Tools/star/ssm_compare.py --selftest
python3 Tools/star/opal_opacity.py --selftest
python3 Tools/star/eos.py --selftest
python3 Tools/star/stellar_structure.py --selftest
python3 Tools/star/solar_compare_plot.py --check

# the figure
python3 Tools/star/solar_compare_plot.py
# -> Docs/research_notes/star_plugins/solar_comparison.png
```

The same checks run under ctest as `fluid_ssm_epsilon`, `fluid_opal_opacity`, `fluid_saha_eos`,
`fluid_lane_emden` and `fluid_solar_model`.

## Rebuilding the OPAL subset

The repository carries a 59 KiB subset (`Docs/research_notes/star_plugins/data/opal_gn93_subset.npz`),
not the 23 MB source archive. To regenerate it:

```bash
pip download rm-tables --no-deps -d /tmp/rmt
cd /tmp/rmt && unzip -o rm_tables-*.whl
python3 Tools/star/extract_opal_subset.py --archive /tmp/rmt/rm_tables/data/opal_type1.npz
```

The tables are Iglesias & Rogers 1996, ApJ 464, 943; the archive comes from Arnold Boothroyd's
CITA mirror. The extraction script records which compositions are kept and asserts the axes, so
the copy can be audited rather than trusted.

## Where the solar model stands

The record is in the specification, but in one sentence: the reaction rates reproduce the SSM's
energy generation to 5% per layer and 0.6% in the integrated luminosity, the OPAL table reproduces
the SSM's opacity column to a median of 0.986, and the structural model - started on the SSM's own
centre with our equation of state, opacity, mixing length and network - reaches L = 1.125 L_sun and
R = 0.539 R_sun. The remaining envelope discrepancy is diagnosed, not hidden: the outer layers do
not cool enough, which is recorded in sections 4.3e-4.3j together with the three numerical mistakes
found along the way.
