/* Copyright 2026 The WarpX Community
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */
#include "PETScReinitialize.H"

namespace warpx::petsc
{

namespace
{
    std::function<void ()>& reinitializer ()
    {
        static std::function<void ()> fn;
        return fn;
    }
}

void set_reinitializer (std::function<void ()> a_reinitializer)
{
    reinitializer() = std::move(a_reinitializer);
}

void reinitialize ()
{
    if (reinitializer()) { reinitializer()(); }
}

} // namespace warpx::petsc
