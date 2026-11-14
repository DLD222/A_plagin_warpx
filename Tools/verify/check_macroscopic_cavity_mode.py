# License: BSD-3-Clause
# SPDX-License-Identifier: BSD-3-Clause
import sys, numpy as np, yt
yt.funcs.mylog.setLevel(50)
c = 2.99792458e8; eps0 = 8.8541878128e-12; pi = np.pi
L = 1.0; p = 1; kz = p*np.pi/L; epsr = 1.5
omega = c*kz/np.sqrt(epsr)
def amplitude(path, n=128):
    ds = yt.load(path)
    ad = ds.all_data()
    by = np.asarray(ad[('boxlib', 'By')])
    z = (np.arange(by.size) + 0.5) * (L / by.size)
    return ds.current_time.to('s').value, np.abs(2*np.trapezoid(by*np.cos(kz*z), z)/L)
t0, a0 = amplitude(sys.argv[1]); t1, a1 = amplitude(sys.argv[2])
sigma = float(sys.argv[3]); gamma = sigma/(2*epsr*eps0)
expected = np.abs(np.cos(omega*t1)/np.cos(omega*t0)) * np.exp(-gamma*(t1-t0))
print(f"t0={t0:.4e} s a0={a0:.6f}   t1={t1:.4e} s a1={a1:.6f}")
print(f"amplitude ratio: measured {a1/a0:.6f}   expected {expected:.6f}   "
      f"(sigma={sigma} S/m, gamma={gamma:.3e}/s)")
