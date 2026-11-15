/* Copyright 2026 DLD222
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WARPX_RUNTIME_FACTORYCONFLICT_H_
#define WARPX_RUNTIME_FACTORYCONFLICT_H_

#include <stdexcept>
#include <string>

namespace warpx::runtime::detail
{

/** Assign one factory slot, recording (not silently accepting) a second, different registration.
 *
 * The registries hold a single slot per kind, so two plugins of the same kind cannot coexist.
 * Before this, the last registration simply won and which one that was depended on
 * static-initialisation and link order - the same input deck could take a different code path
 * depending on how the binary was linked (CODE_REVIEW F-09).  A conflict is recorded here and
 * reported by the create_*() entry point, which runs at a point where aborting is safe.
 *
 * Shared by every factory so the behaviour cannot drift between them (it used to live only in
 * ServiceFactory, leaving the kernel / spectral / magnetostatic registries last-one-wins).
 */
template <class F>
inline void assign_factory (F& slot, F factory, char const* what, std::string& conflict)
{
    if (slot != nullptr && slot != factory && conflict.empty()) {
        conflict = std::string("two plugins registered a ") + what
                 + " factory. The registry holds one slot per kind, so which one takes "
                   "effect would depend on static-initialisation and link order (CODE_REVIEW "
                   "F-09); make the build link only one of them, or give the second one a "
                   "distinct kind.";
    }
    slot = factory;
}

inline void require_no_conflict (std::string const& conflict)
{
    if (!conflict.empty()) { throw std::runtime_error(conflict); }
}

} // namespace warpx::runtime::detail

#endif // WARPX_RUNTIME_FACTORYCONFLICT_H_
