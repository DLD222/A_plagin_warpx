/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_LEVELRESOURCES_H_
#define WARPX_RUNTIME_LEVELRESOURCES_H_

#include <ablastr/fields/MultiFabRegister.H>

namespace warpx::runtime
{

/**
 * \brief Per-level resource registry (A2.1).
 *
 * A2.1a only establishes the ownership invariant: the field register is
 * borrowed, never owned, and the number of registered levels tracks the AMR
 * hierarchy. Resource members (factory/PML/costs/dotMask) move here in
 * A2.1b-e.
 */
class LevelResources
{
public:
    explicit LevelResources (int nlevs = 0);

    void resize (int nlevs);
    [[nodiscard]] int size () const noexcept { return m_size; }

    /** Borrow the authoritative field register. Does not take ownership. */
    void bind_fields (ablastr::fields::MultiFabRegister& fields) noexcept
    {
        m_fields = &fields;
    }

    [[nodiscard]] ablastr::fields::MultiFabRegister* fields () const noexcept
    {
        return m_fields;
    }

    [[nodiscard]] bool has_fields () const noexcept { return m_fields != nullptr; }

private:
    int m_size = 0;
    ablastr::fields::MultiFabRegister* m_fields = nullptr;
};

} // namespace warpx::runtime

#endif // WARPX_RUNTIME_LEVELRESOURCES_H_
