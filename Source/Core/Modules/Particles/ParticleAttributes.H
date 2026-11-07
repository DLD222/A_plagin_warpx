#ifndef WARPX_CORE_PARTICLEATTRIBUTES_H_
#define WARPX_CORE_PARTICLEATTRIBUTES_H_
#include <array>
/** Particle real-attribute indices (project module, dimension dependent). */
struct PIdx
{
#if defined(WARPX_DIM_1D_Z)
    enum { z, w, ux, uy, uz, nattribs };
    static constexpr auto names = {"z", "w", "ux", "uy", "uz"};
#elif defined(WARPX_DIM_XZ)
    enum { x, z, w, ux, uy, uz, nattribs };
    static constexpr auto names = {"x", "z", "w", "ux", "uy", "uz"};
#elif defined(WARPX_DIM_RZ)
    enum { r, z, w, ux, uy, uz, theta, nattribs };
    static constexpr auto names = {"r", "z", "w", "ux", "uy", "uz", "theta"};
#elif defined(WARPX_DIM_RCYLINDER)
    enum { r, w, ux, uy, uz, theta, nattribs };
    static constexpr auto names = {"r", "w", "ux", "uy", "uz", "theta"};
#elif defined(WARPX_DIM_RSPHERE)
    enum { r, w, ux, uy, uz, theta, phi, nattribs };
    static constexpr auto names = {"r", "w", "ux", "uy", "uz", "theta", "phi"};
#elif defined(WARPX_DIM_3D)
    enum { x, y, z, w, ux, uy, uz, nattribs };
    static constexpr auto names = {"x", "y", "z", "w", "ux", "uy", "uz"};
#else
#error WarpX dimension is not correctly set!
#endif
    static_assert(names.size() == nattribs);
};
#endif
