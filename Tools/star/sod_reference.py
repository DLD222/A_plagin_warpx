#!/usr/bin/env python3
"""Exact Riemann solution of the one-dimensional Euler equations (Toro, 3rd ed., chapter 4).

This is the independent reference for the fluid plugin's Sod acceptance test: the C++ test
carries the same algorithm in FluidTestReference.H, and the star state printed here must be the
one the test prints (p* = 0.30313018, u* = 0.92745262 for the standard Sod data).

The star pressure is the root of f_L(p) + f_R(p) = 0 with

    f_K(p) = (p - p_K) sqrt(A_K / (p + B_K)),   A_K = 2/((gamma+1) rho_K),  B_K = (gamma-1)/(gamma+1) p_K
             for p > p_K (shock),
    f_K(p) = 2 c_K/(gamma-1) ((p/p_K)^((gamma-1)/(2 gamma)) - 1)   for p <= p_K (rarefaction),

(Toro 4.39-4.47), and then u* = (u_L + u_R)/2 + (f_R(p*) - f_L(p*))/2 (Toro 4.57).

Run `python3 Tools/star/sod_reference.py` to print the Sod star state; `--selftest` adds
consistency checks that hold independently of the numbers.
"""
import argparse
import math


def c_sound(gamma, p, rho):
    return math.sqrt(gamma * p / rho)


def pressure_function(p, gamma, rho_k, p_k, c_k):
    if p > p_k:
        a = 2.0 / ((gamma + 1.0) * rho_k)
        b = (gamma - 1.0) / (gamma + 1.0) * p_k
        return (p - p_k) * math.sqrt(a / (p + b))
    expo = (gamma - 1.0) / (2.0 * gamma)
    return 2.0 * c_k / (gamma - 1.0) * ((p / p_k) ** expo - 1.0)


def pressure_derivative(p, gamma, rho_k, p_k, c_k):
    if p > p_k:
        a = 2.0 / ((gamma + 1.0) * rho_k)
        b = (gamma - 1.0) / (gamma + 1.0) * p_k
        return math.sqrt(a / (p + b)) * (1.0 - 0.5 * (p - p_k) / (p + b))
    expo = (gamma - 1.0) / (2.0 * gamma)
    return (p / p_k) ** (-expo - 1.0) / (rho_k * c_k)


def star_state(gamma, rho_l, u_l, p_l, rho_r, u_r, p_r, tol=1e-15):
    c_l = c_sound(gamma, p_l, rho_l)
    c_r = c_sound(gamma, p_r, rho_r)

    def f(p):
        return pressure_function(p, gamma, rho_l, p_l, c_l) + \
               pressure_function(p, gamma, rho_r, p_r, c_r)

    def df(p):
        return pressure_derivative(p, gamma, rho_l, p_l, c_l) + \
               pressure_derivative(p, gamma, rho_r, p_r, c_r)

    lo, hi = 1e-12, 2.0 * max(p_l, p_r) + 1e3
    while f(hi) < 0.0:
        hi *= 2.0
    p = 0.5 * (p_l + p_r)
    for _ in range(200):
        value = f(p)
        if abs(value) < tol * max(p_l, p_r):
            break
        if value < 0.0:
            lo = p
        else:
            hi = p
        nxt = p - value / df(p)
        if not (lo < nxt < hi):
            nxt = 0.5 * (lo + hi)
        p = nxt
    u = 0.5 * (u_l + u_r) + 0.5 * (
        pressure_function(p, gamma, rho_r, p_r, c_r)
        - pressure_function(p, gamma, rho_l, p_l, c_l))
    return p, u, c_l, c_r


def star_density(gamma, p_star, p_k, rho_k):
    if p_star > p_k:
        r = p_star / p_k
        gm = (gamma - 1.0) / (gamma + 1.0)
        return rho_k * (r + gm) / (gm * r + 1.0)
    return rho_k * (p_star / p_k) ** (1.0 / gamma)


def selftest():
    gamma = 1.4
    rho_l, u_l, p_l = 1.0, 0.0, 1.0
    rho_r, u_r, p_r = 0.125, 0.0, 0.1
    p_star, u_star, c_l, c_r = star_state(gamma, rho_l, u_l, p_l, rho_r, u_r, p_r)

    # 1. The star pressure is a root of the pressure function.
    f_l = pressure_function(p_star, gamma, rho_l, p_l, c_l)
    f_r = pressure_function(p_star, gamma, rho_r, p_r, c_r)
    assert abs(f_l + f_r) < 1e-10, (f_l, f_r)

    # 2. The same star velocity follows from either side.
    assert abs((u_l - f_l) - (u_r + f_r)) < 1e-10

    # 3. Sod's data give a shock on the left and a rarefaction on the right, and p* lies between
    #    the two initial pressures.
    assert p_r < p_star < p_l

    # 4. Sod's left wave is a rarefaction (p* < p_L) and the right wave is a shock
    #    (p* > p_R); each star density must follow its own branch: the isentrope on the left,
    #    the Rankine-Hugoniot relation on the right.
    rho_star_l = star_density(gamma, p_star, p_l, rho_l)
    assert abs(rho_star_l - rho_l * (p_star / p_l) ** (1.0 / gamma)) < 1e-15
    rho_star_r = star_density(gamma, p_star, p_r, rho_r)
    gm = (gamma - 1.0) / (gamma + 1.0)
    r = p_star / p_r
    assert abs(rho_star_r - rho_r * (r + gm) / (gm * r + 1.0)) < 1e-15

    # 5. A symmetric problem (identical states) has p* = p and u* = u.
    p_sym, u_sym, _, _ = star_state(gamma, 1.0, 0.3, 1.0, 1.0, 0.3, 1.0)
    assert abs(p_sym - 1.0) < 1e-12 and abs(u_sym - 0.3) < 1e-12

    print("sod_reference: all self-checks passed")
    print(f"  p* = {p_star:.8f}   u* = {u_star:.8f}")
    print(f"  rho*_L = {rho_star_l:.6f}   rho*_R = "
          f"{star_density(gamma, p_star, p_r, rho_r):.6f}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        selftest()
        return 0
    gamma = 1.4
    p_star, u_star, _, _ = star_state(gamma, 1.0, 0.0, 1.0, 0.125, 0.0, 0.1)
    print(f"Sod star state: p* = {p_star:.8f}, u* = {u_star:.8f}, "
          f"rho*_L = {star_density(gamma, p_star, 1.0, 1.0):.6f}, "
          f"rho*_R = {star_density(gamma, p_star, 0.1, 0.125):.6f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
