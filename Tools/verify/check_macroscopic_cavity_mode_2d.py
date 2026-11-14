import sys, numpy as np, yt
yt.funcs.mylog.setLevel(50)
c = 2.99792458e8; pi = np.pi
L = 1.0; m = 1; p = 1; epsr = 1.5
kx, kz = m*pi/L, p*pi/L
omega = c*np.sqrt(kx*kx + kz*kz)/np.sqrt(epsr)
def amplitude(path, n=64):
    ds = yt.load(path)
    ad = ds.all_data()
    by = np.asarray(ad[('boxlib','By')]).reshape(n, n)     # (x, z) for 2D
    xs = (np.arange(n)+0.5)/n; zs = (np.arange(n)+0.5)/n
    X, Z = np.meshgrid(xs, zs, indexing='ij')
    weight = 4.0*np.cos(kx*X)*np.cos(kz*Z)
    return ds.current_time.to('s').value, np.abs(np.sum(by*weight)/(n*n))
t0, a0 = amplitude(sys.argv[1]); t1, a1 = amplitude(sys.argv[2])
expected = abs(np.cos(omega*t1)/np.cos(omega*t0))
print(f"t0={t0:.4e} a0={a0:.6f}  t1={t1:.4e} a1={a1:.6f}")
print(f"amplitude ratio: measured {a1/a0:.6f}   analytic {expected:.6f}   (sigma = 0)")
