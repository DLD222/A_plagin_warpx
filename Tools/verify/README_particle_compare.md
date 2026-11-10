# Particle-level comparison of two runs

`compare_particles_openpmd.py` compares the particle dumps of two WarpX runs that wrote
openPMD output.  It exists because the field comparison used for the A/B work reads
plotfiles through yt, and several QED cases write **particles only**.

```bash
python3 Tools/verify/compare_particles_openpmd.py \
    <run_a>/diags/diag1 <run_b>/diags/diag1
```

It compares, per species: the particle count and the *sorted* positions, momenta and
weights.  Sorting makes the check independent of storage order, which is the strongest
statement that can be made about a particle dump without a common particle id.

What it does **not** claim: bit-identical output.  A real physics difference shows up as a
difference in the sorted arrays; a mere reordering does not.

Status 2026-11-11: used to compare two QED cases against the upstream binary -
`test_3d_linear_breit_wheeler_angular` (electron + positron, 174 860 each, OK) and
`test_3d_linear_compton_two_particles` (four species, OK).
