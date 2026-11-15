# Acceptance thresholds: where they come from

`expected.json` lists every acceptance threshold used by the analysis scripts in this directory,
with three fields that matter more than the number itself:

| field | meaning |
|---|---|
| `provenance` | `post-hoc` = written **after** seeing the first runs, from the observed scatter |
| `measured`   | the observation that motivated the value |
| `replacement`| the criterion that would remove the "was it tuned to pass?" doubt |

The project's own validation protocol (`benchmarks/EM_EB_MHD_VALIDATION_10CASES_v2.md:196`) asks
for thresholds to be committed **before** the first run. Most of them were not (audit finding
**Benchmarks/F13**). Recording that here is the honest fix; the alternative — rewriting history as
if the numbers had been predicted — is exactly the kind of thing this audit set out to find.

**What this means for the results:** a `post-hoc` threshold proves "the agreement is at least this
good", not "the code was expected to reach this". Where agreement is much better than the band
(e.g. the Mie convergence study, `p1_tensor_reference.py`, `analyze_v2.py`), the threshold is not
what carries the result — the residual is. Those residuals are reported and asserted on their own
(`selfcheck_*` in the scripts, `RESULT: PASS/FAIL` + `sys.exit` in the verifiers).

Replacing the remaining post-hoc bands with resolution-convergence criteria (error must fall
monotonically under refinement) is the right long-term fix and is listed per case in
`expected.json`.
