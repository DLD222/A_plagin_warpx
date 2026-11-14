# F-06 verification attempt: a periodic deck with a material interface

F-06 was the missing `FillBoundary` on the node-centred eps/sigma arrays, which made the
`rho_medium` seed and the Gauss-residual diagnostic read zero ghosts on a periodic seam.  Verifying
it needs a deck that is periodic *and* has a material interface - and the repository has none, so I
built one from the 1D cavity:

```
boundary.field_lo = periodic
boundary.field_hi = periodic
macroscopic.epsilon2 = 1.0*epsilon0
macroscopic.interface_function(x,y,z) = x - 0.5
algo.macroscopic_subpixel = 1        # without this the interface is ignored (and the code warns)
max_step = 24
```

Two results, one of which is a finding:

1. With `subpixel = 0` the material model warns that `interface_function` is set but the subpixel
   rows are not built, and with the deck's original `abort_on_warning_threshold = medium` it aborts.
   That is the intended behaviour, not a defect.
2. With `subpixel = 1` the run completes, but the solution **diverges**: the Gauss diagnostic at
   step 20 reports `max|divD| = 1.7e267`.

```
[GAUSS] step 20  max|divD - (rho_p + rho_m)| = 0.6829695367
       max|divD| = 1.733188866e+267  max|rho_m| = 1.733188866e+267
```

So the periodic-interface deck does not give a clean F-06 test: the fields blow up long before the
diagnostic can say anything about ghost filling.  Two candidate reasons, neither investigated:

* the initial condition is a PEC cavity mode, which is not a mode of the periodic domain - a periodic
  box does not support the standing wave the deck sets up, so the run is simply not a valid test;
* or the interface plus periodic combination is genuinely unstable in this solver.

I stopped rather than guessing.  F-06's fix therefore remains verified by construction (the arrays
are filled where they are built, and the main solve never depended on the missing ghosts), and this
note records why the intended end-to-end verification did not happen.
